cmake_minimum_required(VERSION 3.7.2)

if (NOT USE_SYSTEM_LIBS)
    include(FetchContent)
    include(ExternalProject)

    include(ProcessorCount)
    ProcessorCount(nproc)
endif()

project(chaosworkshop CXX)

file(GLOB src *.cpp *.h endpoints/*.cpp endpoints/*.h)
add_executable(chaosworkshop ${src})

set(INCLUDE_DIRS ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/vendor)

if (USE_SYSTEM_LIBS)
    find_library(elzip NAMES elzip REQUIRED)
    find_library(dpp NAMES dpp REQUIRED)
    find_library(SQLiteCpp NAMES SQLiteCpp REQUIRED)
    find_library(microhttpd microhttpd REQUIRED)
    find_library(httpserver NAMES httpserver REQUIRED)
else()
    file(STRINGS vendor/elzip.txt elzip_DATA)
    list(GET elzip_DATA 0 elzip_TAG)
    FetchContent_Declare(elzip
        GIT_REPOSITORY https://github.com/Sygmei/11Zip.git
        GIT_TAG ${elzip_TAG}
    )

    file(STRINGS vendor/dpp.txt dpp_DATA)
    list(GET dpp_DATA 0 dpp_TAG)
    FetchContent_Declare(dpp
        GIT_REPOSITORY https://github.com/brainboxdotcc/DPP.git
        GIT_TAG ${dpp_TAG}
    )

    file(STRINGS vendor/sqlitecpp.txt sqlitecpp_DATA)
    list(GET sqlitecpp_DATA 0 sqlitecpp_TAG)
    FetchContent_Declare(sqlitecpp
        GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
        GIT_TAG ${sqlitecpp_TAG}
    )

    FetchContent_MakeAvailable(elzip dpp sqlitecpp)

    find_program(MAKE_EXECUTABLE NAMES make REQUIRED)

    file(STRINGS vendor/libmicrohttpd.txt libmicrohttpd_DATA)
    list(GET libmicrohttpd_DATA 0 libmicrohttpd_TAG)
    list(GET libmicrohttpd_DATA 1 libmicrohttpd_HASH)
    ExternalProject_Add(libmicrohttpd PREFIX prefix
        URL https://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-${libmicrohttpd_TAG}.tar.gz
        URL_HASH ${libmicrohttpd_HASH}
        UPDATE_DISCONNECTED true
        BUILD_IN_SOURCE true
        CONFIGURE_COMMAND ./configure --prefix=${CMAKE_CURRENT_BINARY_DIR}
        BUILD_COMMAND ${MAKE_EXECUTABLE} -j${nproc}
        INSTALL_COMMAND ${MAKE_EXECUTABLE} install
    )
    ExternalProject_Get_Property(libmicrohttpd SOURCE_DIR)
    set(INCLUDE_DIRS ${INCLUDE_DIRS} ${SOURCE_DIR}/src/include)
    add_dependencies(chaosworkshop libmicrohttpd)

    file(STRINGS vendor/libhttpserver.txt libhttpserver_DATA)
    list(GET libhttpserver_DATA 0 libhttpserver_TAG)
    ExternalProject_Add(libhttpserver PREFIX prefix
        GIT_REPOSITORY https://github.com/etr/libhttpserver.git
        GIT_TAG ${libhttpserver_TAG}
        UPDATE_DISCONNECTED true
        BUILD_IN_SOURCE true
        CONFIGURE_COMMAND ./bootstrap COMMAND mkdir -p build
            COMMAND cd build && env CXXFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/include/\ -L${CMAKE_CURRENT_BINARY_DIR}/lib ../configure --prefix=${CMAKE_CURRENT_BINARY_DIR} --disable-examples # One of the examples fails to build
        BUILD_COMMAND cd build && ${MAKE_EXECUTABLE} -j${nproc}
        INSTALL_COMMAND cd build && ${MAKE_EXECUTABLE} install
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
        ${PROJECT_SOURCE_DIR}/vendor/11zip/include
        ${PROJECT_SOURCE_DIR}/vendor/dpp/include
        ${PROJECT_SOURCE_DIR}/vendor/sqlitecpp/include)
endif()
target_include_directories(chaosworkshop BEFORE PRIVATE ${INCLUDE_DIRS})

target_link_libraries(chaosworkshop PRIVATE microhttpd httpserver ${OPENSSL_LIBRARIES} zstd elzip dpp SQLiteCpp sqlite3 pthread dl ICU::uc ICU::i18n)
