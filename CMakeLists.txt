cmake_minimum_required(VERSION 3.7.2)
include(FetchContent)
include(ExternalProject)

include(ProcessorCount)
ProcessorCount(nproc)

add_subdirectory(vendor/11zip)
add_subdirectory(vendor/dpp)
add_subdirectory(vendor/sqlitecpp)

project(chaosworkshop CXX)

file(GLOB src *.cpp *.h endpoints/*.cpp endpoints/*.h)
add_executable(chaosworkshop ${src})

find_program(MAKE_EXECUTABLE NAMES make)

ExternalProject_Add(libmicrohttpd PREFIX prefix
    URL https://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.75.tar.gz
    UPDATE_DISCONNECTED true
    BUILD_IN_SOURCE true
    CONFIGURE_COMMAND ./configure --prefix=${CMAKE_CURRENT_BINARY_DIR}
    BUILD_COMMAND ${MAKE_EXECUTABLE} -j${nproc}
    INSTALL_COMMAND ${MAKE_EXECUTABLE} install
)
add_dependencies(chaosworkshop libmicrohttpd)

ExternalProject_Add(libhttpserver PREFIX prefix
    GIT_REPOSITORY https://github.com/etr/libhttpserver.git
    GIT_TAG acc3e6a3ad0b6193286886015109839d642f1ef6
    UPDATE_DISCONNECTED true
    BUILD_IN_SOURCE true
    CONFIGURE_COMMAND ./bootstrap COMMAND mkdir -p build COMMAND cd build
        COMMAND env CXXFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/include/\ -L${CMAKE_CURRENT_BINARY_DIR}/lib ../configure --prefix=${CMAKE_CURRENT_BINARY_DIR} --disable-examples # One of the examples fails to build
    BUILD_COMMAND cd build COMMAND ${MAKE_EXECUTABLE} -j${nproc}
    INSTALL_COMMAND cd build COMMAND ${MAKE_EXECUTABLE} install
)
add_dependencies(libhttpserver libmicrohttpd)
add_dependencies(chaosworkshop libhttpserver)

find_package(ICU 61.0 COMPONENTS uc i18n REQUIRED)

set_property(TARGET chaosworkshop PROPERTY CXX_STANDARD 20)

target_link_directories(chaosworkshop BEFORE PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/lib)
target_include_directories(chaosworkshop BEFORE PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/vendor
    ${PROJECT_SOURCE_DIR}/vendor/sqlitecpp/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
    )

target_link_libraries(chaosworkshop PRIVATE microhttpd httpserver zstd elzip dpp SQLiteCpp sqlite3 pthread dl ICU::uc ICU::i18n)
