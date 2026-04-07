cmake_minimum_required(VERSION 3.14)

project(rssani LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ExternalProject)

find_package(Qt6 COMPONENTS Core Network Test REQUIRED)
find_package(Qt6 COMPONENTS LinguistTools QUIET)

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

# --- libirc (grumpy-irc) ---
ExternalProject_Add(libirc_ext
  GIT_REPOSITORY https://github.com/grumpy-irc/libirc.git
  GIT_SHALLOW ON
  PREFIX ${DEPS_PREFIX}/libirc
  CMAKE_ARGS
    -DQT6_BUILD=true
    -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DCMAKE_CXX_STANDARD=20
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
  BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR>
  INSTALL_COMMAND
    ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/libirc <INSTALL_DIR>/include/libirc
    COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/libircclient <INSTALL_DIR>/include/libircclient
    COMMAND ${CMAKE_COMMAND} -E copy <BINARY_DIR>/libirc/libirc.so <INSTALL_DIR>/lib/libirc.so
    COMMAND ${CMAKE_COMMAND} -E copy <BINARY_DIR>/libircclient/libircclient.so <INSTALL_DIR>/lib/libircclient.so
)
ExternalProject_Get_Property(libirc_ext INSTALL_DIR)
set(LIBIRC_INSTALL_DIR ${INSTALL_DIR})

# --- rssani ---
set(TS_FILES rssani_en_US.ts)

add_executable(rssani
  main.cpp
  mailsender.cpp
  myircsession.cpp
  rss_lite.cpp
  rssani_lite.cpp
  xmlrpc.cpp
  ${TS_FILES}
)

add_dependencies(rssani libirc_ext)

target_include_directories(rssani PRIVATE
  ${LIBIRC_INSTALL_DIR}/include
)

target_link_directories(rssani PRIVATE
  ${LIBIRC_INSTALL_DIR}/lib
)

separate_arguments(XMLRPC_CFLAGS_LIST UNIX_COMMAND "${XMLRPC_CFLAGS}")
target_compile_options(rssani PRIVATE ${XMLRPC_CFLAGS_LIST})

separate_arguments(XMLRPC_LIBS_LIST UNIX_COMMAND "${XMLRPC_LIBS}")
target_link_libraries(rssani Qt6::Core Qt6::Network ircclient irc ${XMLRPC_LIBS_LIST})

set_target_properties(rssani PROPERTIES
  INSTALL_RPATH "${LIBIRC_INSTALL_DIR}/lib"
  BUILD_WITH_INSTALL_RPATH ON
)

if(Qt6LinguistTools_FOUND)
  qt6_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

# --- Unit Tests ---
# Use QTest framework with separate test executables to avoid multiple main() definitions
add_executable(rssani_tests_values tests/test_values.cpp)
add_executable(rssani_tests_mail tests/test_mail_sender.cpp mailsender.cpp)
add_executable(rssani_tests_rss tests/test_rss_lite.cpp mailsender.cpp rss_lite.cpp)
add_executable(rssani_tests_rssani tests/test_rssani_lite.cpp rssani_lite.cpp rss_lite.cpp mailsender.cpp myircsession.cpp)
add_executable(rssani_tests_irc tests/test_myirc_session.cpp myircsession.cpp)

# Link each test executable separately
foreach(test_tgt rssani_tests_values rssani_tests_mail rssani_tests_rss rssani_tests_rssani rssani_tests_irc)
  target_include_directories(${test_tgt} PRIVATE
    ${LIBIRC_INSTALL_DIR}/include
    ${CMAKE_SOURCE_DIR}
  )
  target_link_directories(${test_tgt} PRIVATE
    ${LIBIRC_INSTALL_DIR}/lib
  )
  target_link_libraries(${test_tgt} PRIVATE
    Qt6::Core Qt6::Network Qt6::Test
  )
  set_target_properties(${test_tgt} PROPERTIES
    INSTALL_RPATH "${LIBIRC_INSTALL_DIR}/lib"
    BUILD_WITH_INSTALL_RPATH ON
  )
  target_compile_options(${test_tgt} PRIVATE ${XMLRPC_CFLAGS_LIST})
  add_dependencies(${test_tgt} libirc_ext)
endforeach()

# The rssani_tests_rss and rssani_tests_rssani need irc libs and xmlrpc
target_link_libraries(rssani_tests_rss PRIVATE ircclient irc ${XMLRPC_LIBS_LIST})
target_link_libraries(rssani_tests_rssani PRIVATE ircclient irc ${XMLRPC_LIBS_LIST})
target_link_libraries(rssani_tests_irc PRIVATE ircclient irc)

# Add all tests to CTest
add_test(NAME test_values COMMAND rssani_tests_values)
add_test(NAME test_mail_sender COMMAND rssani_tests_mail)
add_test(NAME test_rss_lite COMMAND rssani_tests_rss)
add_test(NAME test_rssani_lite COMMAND rssani_tests_rssani)
add_test(NAME test_myirc_session COMMAND rssani_tests_irc)
