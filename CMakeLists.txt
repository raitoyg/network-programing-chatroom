cmake_minimum_required(VERSION 3.22)
project(network_programing_chatroom)

set(CMAKE_CXX_STANDARD 14)

add_executable(network_programing_chatroom
        client.cpp
        server.cpp)
