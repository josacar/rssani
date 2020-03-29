cmake_minimum_required(VERSION 3.5)

project(rssani LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Core LinguistTools Network REQUIRED)

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

include_directories("/home/selu/code/libcommuni/include/IrcCore" "/home/selu/code/ulxmlrpcpp")

link_directories("/home/selu/code/libcommuni/include/IrcCore" "/home/selu/code/ulxmlrpcpp/ulxmlrpcpp/.libs")

target_link_libraries(rssani Qt5::Core Qt5::Network -L/home/selu/code/ulxmlrpcpp/ulxmlrpcpp/.libs ulxmlrpcpp -L/home/selu/code/libcommuni/lib IrcCore)


qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
