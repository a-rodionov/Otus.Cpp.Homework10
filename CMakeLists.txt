cmake_minimum_required(VERSION 3.2)

# Определение PATCH_VERSION для локальной сборки и сборки на travis
set(PATCH_VERSION 1)
if($ENV{TRAVIS_BUILD_NUMBER})
  set(PATCH_VERSION $ENV{TRAVIS_BUILD_NUMBER})
endif()

project(bulkmt VERSION 1.0.${PATCH_VERSION})

find_package(Threads)
find_package(Boost COMPONENTS unit_test_framework REQUIRED)

# Создание целей
add_executable(bulkmt main.cpp)

add_executable(bulkmt_test test.cpp)

# Настройка целей

# для всех целей
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wpedantic -Wall -Wextra)

target_link_libraries(bulkmt
  ${CMAKE_THREAD_LIBS_INIT}
)

target_compile_definitions(bulkmt_test PRIVATE BOOST_TEST_DYN_LINK)
target_link_libraries(bulkmt_test 
  Boost::unit_test_framework
  ${CMAKE_THREAD_LIBS_INIT}
)

install(TARGETS bulkmt RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_CONTACT camelot58@gmail.com)

include (CPack)

enable_testing()

add_test(test_suite_internals bulkmt_test)
add_test(fixture_test_suite_parser bulkmt_test)
add_test(test_suite_file_output bulkmt_test)
add_test(test_suite_thread_pool bulkmt_test)
add_test(test_suite_multithread_file_output bulkmt_test)
