cmake_minimum_required(VERSION 3.24)
project(search_server)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)

add_executable(search_server
        document.cpp
        document.h
        main.cpp
        paginator.h
        read_input_functions.cpp
        read_input_functions.h
        request_queue.cpp
        request_queue.h
        search_server.cpp
        search_server.h
        string_processing.cpp
        string_processing.h cmake-build-release/remove_duplicates.h remove_duplicates.h remove_duplicates.cpp)
