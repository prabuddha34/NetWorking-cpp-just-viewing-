cmake_minimum_required(VERSION 3.20)

project(NetScope LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# NetScope Server
add_executable(NetScope
        NetWorking.cpp
        server.cpp
        server.h
        tree.cpp
        tree.h
)

# NetScope Client
add_executable(NetScopeClient
        clientside.cpp
)

# Graph Assignment - Part 1
add_executable(Graphs
        graphs.c
)

# Binary Tree - Standalone C
add_executable(BinaryTree tree.c)

# Recursion practice
add_executable(Recursion recursion.c)

# Winsock practice
add_executable(NetPractice NetPrac.cpp NetPrac.h)

# Windows Networking
if(WIN32)
    target_link_libraries(NetScope PRIVATE ws2_32)
    target_link_libraries(NetScopeClient PRIVATE ws2_32)
    target_link_libraries(NetPractice PRIVATE ws2_32)
endif()