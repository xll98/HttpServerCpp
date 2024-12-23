cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME "HttpServer_cpp")
project(httpdemo)

#通用设置
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#引用main函数的文件
FILE(GLOB SRC_FILE 
    ${CMAKE_SOURCE_DIR}/HttpServer/*.c
    ${CMAKE_SOURCE_DIR}/HttpServer/*.cpp
    ${CMAKE_SOURCE_DIR}/common/*.c
    ${CMAKE_SOURCE_DIR}/common/*.cpp
)
include_directories(${CMAKE_SOURCE_DIR}/HttpServer/)
include_directories(${CMAKE_SOURCE_DIR}/common/)
#可执行程序的源文件
add_executable(${PROJECT_NAME} ${SRC_FILE})