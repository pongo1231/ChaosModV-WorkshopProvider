cmake_minimum_required(VERSION 3.7.2)

if (NOT USE_SYSTEM_LIBS)
    include(FetchContent)
    include(ExternalProject)

    include(ProcessorCount)
    ProcessorCount(nproc)
endif()

if (USE_SYSTEM_LIBS)
    find_library(elzip NAMES elzip REQUIRED)
    find_library(dpp NAMES dpp REQUIRED)
    find_library(SQLiteCpp NAMES SQLiteCpp REQUIRED)
else()
    add_subdirectory(vendor/11zip)
    add_subdirectory(vendor/dpp)
    add_subdirectory(vendor/sqlitecpp)
endif()

project(chaosworkshop CXX)

file(GLOB src *.cpp *.h endpoints/*.cpp endpoints/*.h)
add_executable(chaosworkshop ${src})

if (NOT USE_SYSTEM_LIBS)
    find_program(MAKE_EXECUTABLE NAMES make)
endif()

set(INCLUDE_DIRS ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/vendor)

if (USE_SYSTEM_LIBS)
    find_library(microhttpd microhttpd REQUIRED)
else()
    ExternalProject_Add(libmicrohttpd PREFIX prefix
        URL https://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.71.tar.gz
        UPDATE_DISCONNECTED true
        BUILD_IN_SOURCE true
        CONFIGURE_COMMAND ./configure --prefix=${CMAKE_CURRENT_BINARY_DIR}
        BUILD_COMMAND ${MAKE_EXECUTABLE} -j${nproc}
        INSTALL_COMMAND ${MAKE_EXECUTABLE} install
    )
    ExternalProject_Get_Property(libmicrohttpd SOURCE_DIR)
    set(INCLUDE_DIRS ${INCLUDE_DIRS} ${SOURCE_DIR}/src/include)
    add_dependencies(chaosworkshop libmicrohttpd)
endif()

if (USE_SYSTEM_LIBS)
    find_library(httpserver NAMES httpserver REQUIRED)
else()
    ExternalProject_Add(libhttpserver PREFIX prefix
        GIT_REPOSITORY https://github.com/etr/libhttpserver.git
        GIT_TAG 60be2d347fb3c757b2e77efc997120e47ed8fb7f
        UPDATE_DISCONNECTED true
        BUILD_IN_SOURCE true
        CONFIGURE_COMMAND ./bootstrap COMMAND mkdir -p build COMMAND cd build
            COMMAND env CXXFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/include/\ -L${CMAKE_CURRENT_BINARY_DIR}/lib ../configure --prefix=${CMAKE_CURRENT_BINARY_DIR} --disable-examples # One of the examples fails to build
        BUILD_COMMAND cd build COMMAND ${MAKE_EXECUTABLE} -j${nproc}
        INSTALL_COMMAND cd build COMMAND ${MAKE_EXECUTABLE} install
    )
    ExternalProject_Get_Property(libhttpserver SOURCE_DIR)
    set(INCLUDE_DIRS ${INCLUDE_DIRS} ${SOURCE_DIR}/src)
    add_dependencies(libhttpserver libmicrohttpd)
    add_dependencies(chaosworkshop libhttpserver)
endif()

find_package(ICU 61.0 COMPONENTS uc i18n REQUIRED)

find_package(OpenSSL REQUIRED)

set_property(TARGET chaosworkshop PROPERTY CXX_STANDARD 20)

target_link_directories(chaosworkshop BEFORE PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/lib)

if (NOT USE_SYSTEM_LIBS)
    set(INCLUDE_DIRS ${INCLUDE_DIRS}
        ${libmicrohttpd_install}/include
        ${libhttpserver_install}/include
        ${PROJECT_SOURCE_DIR}/vendor/sqlitecpp/include
        ${PROJECT_SOURCE_DIR}/vendor/sqlitecpp/include
        ${PROJECT_SOURCE_DIR}/vendor/11zip/include)
endif()
target_include_directories(chaosworkshop BEFORE PRIVATE ${INCLUDE_DIRS})

target_link_libraries(chaosworkshop PRIVATE microhttpd httpserver ${OPENSSL_LIBRARIES} zstd elzip dpp SQLiteCpp sqlite3 pthread dl ICU::uc ICU::i18n)
