cmake_minimum_required(VERSION 3.10)
project(BackEnd)

set(CMAKE_CXX_STANDARD 14)

# Include directories for header files
include_directories(CROW_ROUTEs)
include_directories(Programs)

file(GLOB SOURCE_FILES
        "CROW_ROUTEs/*.cpp"
        "Programs/*.cpp"
        "API/*.cpp"
)

file(GLOB HEADER_FILES
        "include/*.h"
)

# Add source files
add_executable(BackEnd ${SOURCE_FILES} ${HEADER_FILES} main.cpp)

# - pthread: provides support for multithreading
# - crypto: provides cryptographic functions
# - ssl: provides SSL/TLS support
# - curl: provides support for making HTTP requests
# - mysqlcppconn: provides MySQL database connectivity
# - vmime: provides support for handling MIME messages
# - bcrypt: provides support for bcrypt password hashing
target_link_libraries(BackEnd pthread crypto ssl curl mysqlcppconn vmime bcrypt)

add_definitions(-DCROW_ENABLE_SSL)