cmake_minimum_required(VERSION 3.10)
project(test_ePSU)  

FIND_PACKAGE(OpenMP REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/thirdparty/out/install")
include_directories(${CMAKE_SOURCE_DIR}/thirdparty/out/install/include)

include_directories(ePSU)
file(GLOB_RECURSE SRCS
    ePSU/*.cpp
)

find_package(volePSI REQUIRED)

add_executable(psPMT test/test_psPMT.cpp ${SRCS})
target_compile_options(psPMT PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++17> -lpthread -O3)
target_link_libraries(psPMT visa::volePSI  OpenMP::OpenMP_CXX)

add_executable(ssOTd test/test_ssOTd.cpp ${SRCS})
target_compile_options(ssOTd PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++17> -lpthread -O3)
target_link_libraries(ssOTd visa::volePSI  OpenMP::OpenMP_CXX)

add_executable(ePSU test/test_ePSU.cpp ${SRCS})
target_compile_options(ePSU PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++17> -lpthread -O3)
target_link_libraries(ePSU visa::volePSI  OpenMP::OpenMP_CXX)

add_executable(RPMT test/test_RPMT.cpp ${SRCS})
target_compile_options(RPMT PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++17> -lpthread -O3)
target_link_libraries(RPMT visa::volePSI  OpenMP::OpenMP_CXX)

add_executable(PSU test/test_PSU.cpp ${SRCS})
target_compile_options(PSU PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++17> -lpthread -O3)
target_link_libraries(PSU visa::volePSI  OpenMP::OpenMP_CXX)