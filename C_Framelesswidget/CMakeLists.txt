﻿cmake_minimum_required(VERSION 3.16)
project(C_Framelesswidget)

#set(CMAKE_BUILD_TYPE Release)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/)

add_subdirectory(src)
