#include "FileOutput.h"
#include <algorithm>
#include <functional>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define BOOST_TEST_MODULE test_benchmark

#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>

using namespace std::chrono_literals;

class FileOutputWithCPUOverhead : public FileOutput
{
public:

  explicit FileOutputWithCPUOverhead(size_t threads_count = 0)
    : FileOutput(threads_count) {}

  auto StopWorkers() {
    is_end_of_infinite_loop = true;
    return FileOutput::StopWorkers();
  }

  void Output(const std::size_t timestamp, std::shared_ptr<const std::list<std::string>> data) override {
    AddTask([this, timestamp, data]() {

      std::unique_lock<std::shared_timed_mutex> lock_filenames(filenames_mutex);
      processed_filenames.push_back(MakeFilename(timestamp, 0));
      lock_filenames.unlock();

      std::shared_lock<std::shared_timed_mutex> lock_statistics(statistics_mutex);
      auto statistics = threads_statistics.find(std::this_thread::get_id());
      if(std::cend(threads_statistics) == statistics)
        throw std::runtime_error("FileOutput::Output. Statistics for this thread doesn't exist.");
      lock_statistics.unlock();

      while(!is_end_of_infinite_loop) {
        // Каждый поток модифицирует только свою статистику, поэтому безопасно ее модифицировать без блокировки.
        ++statistics->second.blocks;

        for(auto i{0}; i < 10000; ++i) {
          std::for_each(std::cbegin(*data),
                        std::cend(*data),
                        [&statistics] (const auto& str) {
                          statistics->second.commands += std::hash<std::string>()(str);
                        });
        }

        WriteBulkToFile(timestamp, *data, 0);
      }
    });
  }

private:

  std::atomic_bool is_end_of_infinite_loop{false};
};

static char buffer_file_io[1024] __attribute__ ((__aligned__ (1024)));

void benchmark_iteration(size_t threads_count,
                         std::chrono::seconds iteration_period) {
  FileOutputWithCPUOverhead thread_pool;
  std::shared_ptr<std::list<std::string>> data{ new std::list<std::string>{"cmd1", "cmd2", "cmd3"}};

  for(decltype(threads_count) i{0}; i < threads_count; ++i) {
    thread_pool.AddWorker();
    thread_pool.Output(i, data);
  }
  auto start = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(iteration_period);
  auto files_statistics = thread_pool.StopWorkers();
  auto end = std::chrono::high_resolution_clock::now();
  
  auto filenames = thread_pool.GetProcessedFilenames();
  for(const auto& filename : filenames) {
    std::remove(filename.c_str());
  }

  size_t total_blocks{0};
  for(const auto& file_statistics : files_statistics) {
    total_blocks += file_statistics.second.blocks;
  }

  std::cout << threads_count
            << " : "
            << total_blocks
            << " : "
            << (1000 * static_cast<double>(total_blocks) / (threads_count * std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count()))
            << std::endl;
}

void benchmark_iteration(size_t threads_count,
                         std::chrono::seconds iteration_period,
                         int fd,
                         size_t filesize) {
  FileOutputWithCPUOverhead thread_pool;
  std::shared_ptr<std::list<std::string>> data{ new std::list<std::string>{"cmd1", "cmd2", "cmd3"}};
  size_t read_counts{0};

  for(decltype(threads_count) i{0}; i < threads_count; ++i) {
    thread_pool.AddWorker();
    thread_pool.Output(i, data);
  }
  auto start = std::chrono::high_resolution_clock::now();
  auto file_read_end = std::chrono::high_resolution_clock::now();
  while(iteration_period > std::chrono::duration_cast<std::chrono::seconds>(file_read_end-start)) {
    BOOST_REQUIRE(0 == lseek(fd, 0, SEEK_SET));
    for(size_t i{0}; i < filesize/sizeof(buffer_file_io); ++i) {
      BOOST_REQUIRE(sizeof(buffer_file_io) == read(fd, buffer_file_io, sizeof(buffer_file_io)));
      ++read_counts;
    }
    file_read_end = std::chrono::high_resolution_clock::now();
  }
  auto files_statistics = thread_pool.StopWorkers();
  auto end = std::chrono::high_resolution_clock::now();
  
  auto filenames = thread_pool.GetProcessedFilenames();
  for(const auto& filename : filenames) {
    std::remove(filename.c_str());
  }

  size_t total_blocks{0};
  for(const auto& file_statistics : files_statistics) {
    total_blocks += file_statistics.second.blocks;
  }

  std::cout << threads_count
            << " : "
            << total_blocks
            << " : "
            << (0 == threads_count ? 0 :
               (1000 * static_cast<double>(total_blocks) / (threads_count * std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count())))
            << " : "
            << (1000 * static_cast<double>(read_counts) / std::chrono::duration_cast<std::chrono::milliseconds>(file_read_end-start).count())
            << std::endl;
}


BOOST_AUTO_TEST_SUITE(test_suite_main)

BOOST_AUTO_TEST_CASE(benchmark_without_file_reading)
{
  const size_t max_threads_count = 3 * std::thread::hardware_concurrency();
  const std::chrono::seconds iteration_period{10};
  
  std::cout << "Тест производительности многопоточной записи блоков в файлы.\n"
            << "Внимание! Данный тест будет выполняться не менее " << max_threads_count * iteration_period.count() << " секунд.\n"
            << "Одновременно возможно выполнение " << std::thread::hardware_concurrency() << " потоков.\n"
            << "Значения столбцов:\n"
            << "1. Количество потоков записи в файл.\n"
            << "2. Общее количество записанных блоков.\n"
            << "3. Среднее количество блоков, записываемых потоками в секунду.\n";
  for(size_t i{1}; i <= max_threads_count; ++i) {
    benchmark_iteration(i, iteration_period);
  }
}

BOOST_AUTO_TEST_CASE(benchmark_with_file_reading)
{
  const size_t max_threads_count = 3 * std::thread::hardware_concurrency();
  const std::chrono::seconds iteration_period{10};
  const std::string filename {"benchmark.dat"};
  const size_t filesize = 512*sizeof(buffer_file_io);
  
  std::fill(std::begin(buffer_file_io), std::end(buffer_file_io), 0xAC);
  auto fd = open(filename.c_str(), O_CREAT | O_RDWR | O_DIRECT | O_SYNC, S_IRWXU | S_IRWXG | S_IRWXO);
  BOOST_REQUIRE(-1 != fd);
  for(size_t i{0}; i < filesize/sizeof(buffer_file_io); ++i) {
    if(sizeof(buffer_file_io) != write(fd, buffer_file_io, sizeof(buffer_file_io))) {
      close(fd);
      std::remove(filename.c_str());
      BOOST_FAIL("Ошибка создания файла для чтения.");
    }
  }
  BOOST_REQUIRE(0 == lseek(fd, 0, SEEK_SET));

  std::cout << "Тест производительности многопоточной записи блоков в файлы и одновременного чтения из файла.\n"
            << "Внимание! Данный тест будет выполняться не менее " << max_threads_count * iteration_period.count() << " секунд.\n"
            << "Одновременно возможно выполнение " << std::thread::hardware_concurrency() << " потоков.\n"
            << "Значения столбцов:\n"
            << "1. Количество потоков записи в файл.\n"
            << "2. Общее количество записанных блоков.\n"
            << "3. Среднее количество блоков, записываемых потоками в секунду.\n"
            << "4. Скорость чтения файла основным потом в КБ/с.\n";
  for(size_t i{0}; i <= max_threads_count; ++i) {
    benchmark_iteration(i, iteration_period, fd, filesize);
  }
  BOOST_REQUIRE(0 == close(fd));
  std::remove(filename.c_str());
}

BOOST_AUTO_TEST_SUITE_END()
