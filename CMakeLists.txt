cmake_minimum_required(VERSION 3.14)

project(rssani LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ExternalProject)

find_package(Qt6 COMPONENTS Core LinguistTools Network Core5Compat REQUIRED)

# --- xmlrpc-c (system) ---
find_program(XMLRPC_C_CONFIG xmlrpc-c-config)
if(XMLRPC_C_CONFIG)
  execute_process(COMMAND ${XMLRPC_C_CONFIG} c++2 abyss-server --cflags
    OUTPUT_VARIABLE XMLRPC_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${XMLRPC_C_CONFIG} c++2 abyss-server --libs
    OUTPUT_VARIABLE XMLRPC_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(XMLRPC REQUIRED xmlrpc_server_abyss++)
  set(XMLRPC_CFLAGS "${XMLRPC_CFLAGS_OTHER}")
  set(XMLRPC_LIBS "${XMLRPC_LINK_LIBRARIES}")
endif()

set(DEPS_PREFIX ${CMAKE_BINARY_DIR}/deps)

# --- libcommuni ---
ExternalProject_Add(libcommuni_ext
  GIT_REPOSITORY https://github.com/communi/libcommuni.git
  GIT_TAG v3.7.0
  GIT_SHALLOW ON
  PREFIX ${DEPS_PREFIX}/libcommuni
  CONFIGURE_COMMAND qmake6 <SOURCE_DIR>/libcommuni.pro IRC_INSTALL_LIBS=<INSTALL_DIR>/lib IRC_INSTALL_HEADERS=<INSTALL_DIR>/include IRC_INSTALL_FEATURES=<INSTALL_DIR>/features CONFIG+=no_tests CONFIG+=no_examples
  PATCH_COMMAND sed -i "s/QT = core network/QT = core network core5compat/" <SOURCE_DIR>/src/module_build.pri
  BUILD_COMMAND make -j4
  INSTALL_COMMAND bash -c "make -C src/core install && make -C src/model install && make -C src/util install"
  BUILD_IN_SOURCE ON
)
ExternalProject_Get_Property(libcommuni_ext INSTALL_DIR)
set(LIBCOMMUNI_INSTALL_DIR ${INSTALL_DIR})

# --- rssani ---
set(TS_FILES rssani_en_US.ts)

add_definitions(-DIRC_STATIC)

add_executable(rssani
  main.cpp
  mailsender.cpp
  myircsession.cpp
  rss_lite.cpp
  rssani_lite.cpp
  xmlrpc.cpp
  ${TS_FILES}
)

add_dependencies(rssani libcommuni_ext)

target_include_directories(rssani PRIVATE
  ${LIBCOMMUNI_INSTALL_DIR}/include/IrcCore
)

target_link_directories(rssani PRIVATE
  ${LIBCOMMUNI_INSTALL_DIR}/lib
)

separate_arguments(XMLRPC_CFLAGS_LIST UNIX_COMMAND "${XMLRPC_CFLAGS}")
target_compile_options(rssani PRIVATE ${XMLRPC_CFLAGS_LIST})

separate_arguments(XMLRPC_LIBS_LIST UNIX_COMMAND "${XMLRPC_LIBS}")
target_link_libraries(rssani Qt6::Core Qt6::Network IrcCore ${XMLRPC_LIBS_LIST})

set_target_properties(rssani PROPERTIES
  INSTALL_RPATH "${LIBCOMMUNI_INSTALL_DIR}/lib"
  BUILD_WITH_INSTALL_RPATH ON
)

qt6_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
