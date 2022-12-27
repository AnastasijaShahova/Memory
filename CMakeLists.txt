cmake_minimum_required(VERSION 3.5)

project(Task3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

add_executable(Task4 main.cpp MemoryAllocator.h MemoryAllocator.cpp CoalesceAllocator.h CoalesceAllocator.cpp FixedSizeAllocator.h FixedSizeAllocator.cpp)
