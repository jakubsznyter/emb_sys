# ==============================================================
# Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2026.1 (64-bit)
# Tool Version Limit: 2026.06
# Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
# Copyright 2022-2026 Advanced Micro Devices, Inc. All Rights Reserved.
# 
# ==============================================================
cmake_minimum_required(VERSION 3.15)
project(snn_mlp)
find_package(common)
collector_create (PROJECT_LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}")
collect (PROJECT_LIB_SOURCES xsnn_mlp.c)
collect (PROJECT_LIB_SOURCES xsnn_mlp_sinit.c)
collect (PROJECT_LIB_SOURCES xsnn_mlp_linux.c)
collect (PROJECT_LIB_SOURCES xsnn_mlp_g.c)
collector_list (_sources PROJECT_LIB_SOURCES)
collector_create (PROJECT_LIB_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}")
collect (PROJECT_LIB_HEADERS xsnn_mlp.h)
collect (PROJECT_LIB_HEADERS xsnn_mlp_hw.h)
collector_list (_headers PROJECT_LIB_HEADERS)
file(COPY ${_headers} DESTINATION ${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_BINARY_DIR}/include)
add_library(snn_mlp STATIC ${_sources})
set_target_properties(snn_mlp PROPERTIES LINKER_LANGUAGE C)

