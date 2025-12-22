cmake_minimum_required(VERSION 3.16)
project(BDU_GUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Dependencies ---

# 1. Qt6
find_package(Qt6 COMPONENTS Widgets REQUIRED)

# 2. PostgreSQL (libpqxx)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# 3. CURL
find_package(CURL REQUIRED)

# 4. OpenSSL (Crypto)
find_package(OpenSSL REQUIRED)

# 5. OpenXLSX (Download and build automatically)
include(FetchContent)
FetchContent_Declare(
    OpenXLSX
    GIT_REPOSITORY https://github.com/troldal/OpenXLSX.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(OpenXLSX)

# --- Sources ---
set(LOGIC_SOURCES
    src/db.cpp src/db.hpp
    src/auth.cpp src/auth.hpp
    src/crypto.cpp src/crypto.hpp
    src/threats.cpp src/threats.hpp
    src/search_session.cpp src/search_session.hpp
    src/downloader.cpp src/downloader.hpp
    src/xlsx_converter.cpp src/xlsx_converter.hpp
    src/importer.cpp src/importer.hpp
)

set(GUI_SOURCES
    src/main.cpp
    src/mainwindow.cpp src/mainwindow.h
    src/logindialog.cpp src/logindialog.h
    src/addthreatdialog.cpp src/addthreatdialog.h
    src/manageusersdialog.cpp src/manageusersdialog.h
)

add_executable(bdu_app ${GUI_SOURCES} ${LOGIC_SOURCES})

# --- Include Directories ---
target_include_directories(bdu_app PRIVATE 
    src 
    ${PQXX_INCLUDE_DIRS}
)

# --- Linking ---
target_link_libraries(bdu_app PRIVATE
    Qt6::Widgets
    ${PQXX_LIBRARIES}
    CURL::libcurl
    OpenXLSX::OpenXLSX
    OpenSSL::Crypto
)