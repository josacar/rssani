cmake_minimum_required(VERSION 3.14)

project(rssani LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ExternalProject)

find_package(Qt5 COMPONENTS Core LinguistTools Network REQUIRED)

set(DEPS_PREFIX ${CMAKE_BINARY_DIR}/deps)

# --- ulxmlrpcpp ---
ExternalProject_Add(ulxmlrpcpp_ext
  URL https://sourceforge.net/projects/ulxmlrpcpp/files/ulxmlrpcpp/1.7.5/ulxmlrpcpp-1.7.5-src.tar.bz2
  PREFIX ${DEPS_PREFIX}/ulxmlrpcpp
  PATCH_COMMAND find <SOURCE_DIR> -name "*.cpp" -exec sed -i "s|SSL_METHOD \\*meth = SSLv23_method|const SSL_METHOD *meth = SSLv23_method|g" {} +
  CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
  BUILD_COMMAND make -j4
  INSTALL_COMMAND bash -c "make -C ulxmlrpcpp install-libLTLIBRARIES && make -C ulxmlrpcpp/contrib install-libLTLIBRARIES && mkdir -p <INSTALL_DIR>/include/ulxmlrpcpp && cp <SOURCE_DIR>/ulxmlrpcpp/*.h <INSTALL_DIR>/include/ulxmlrpcpp/"
)
ExternalProject_Get_Property(ulxmlrpcpp_ext INSTALL_DIR)
set(ULXMLRPCPP_INSTALL_DIR ${INSTALL_DIR})

# --- libcommuni ---
ExternalProject_Add(libcommuni_ext
  GIT_REPOSITORY https://github.com/communi/libcommuni.git
  GIT_TAG v3.7.0
  GIT_SHALLOW ON
  PREFIX ${DEPS_PREFIX}/libcommuni
  CONFIGURE_COMMAND qmake <SOURCE_DIR>/libcommuni.pro IRC_INSTALL_LIBS=<INSTALL_DIR>/lib IRC_INSTALL_HEADERS=<INSTALL_DIR>/include IRC_INSTALL_FEATURES=<INSTALL_DIR>/features
  BUILD_COMMAND make -j4
  INSTALL_COMMAND make install
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

add_dependencies(rssani ulxmlrpcpp_ext libcommuni_ext)

target_include_directories(rssani PRIVATE
  ${ULXMLRPCPP_INSTALL_DIR}/include
  ${LIBCOMMUNI_INSTALL_DIR}/include/IrcCore
)

target_link_directories(rssani PRIVATE
  ${ULXMLRPCPP_INSTALL_DIR}/lib
  ${LIBCOMMUNI_INSTALL_DIR}/lib
)

target_link_libraries(rssani Qt5::Core Qt5::Network ulxmlrpcpp IrcCore)

set_target_properties(rssani PROPERTIES
  INSTALL_RPATH "${ULXMLRPCPP_INSTALL_DIR}/lib;${LIBCOMMUNI_INSTALL_DIR}/lib"
  BUILD_WITH_INSTALL_RPATH ON
)

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
