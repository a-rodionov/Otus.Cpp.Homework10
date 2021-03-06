cmake_minimum_required(VERSION 3.2)

# Определение PATCH_VERSION для локальной сборки и сборки на travis
set(PATCH_VERSION 1)
if($ENV{TRAVIS_BUILD_NUMBER})
  set(PATCH_VERSION $ENV{TRAVIS_BUILD_NUMBER})
endif()

project(bulkmt VERSION 1.0.${PATCH_VERSION})

find_package(Threads)
find_package(Boost COMPONENTS unit_test_framework system thread log REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Создание целей
add_executable(bulkmt main.cpp)

# Настройка для всех целей
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wpedantic -Wall -Wextra)

target_compile_definitions(bulkmt PRIVATE BOOST_LOG_DYN_LINK)
target_link_libraries(bulkmt
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
)

install(TARGETS bulkmt RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_CONTACT camelot58@gmail.com)

include (CPack)

enable_testing()

add_subdirectory(tests)
