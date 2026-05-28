
cmake_minimum_required(VERSION 3.10)
project(ePSU)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Release)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

set(THIRDPARTY_INSTALL_DIR "${CMAKE_SOURCE_DIR}/thirdparty/out/install")
set(CMAKE_PREFIX_PATH "${THIRDPARTY_INSTALL_DIR}")

find_package(OpenMP REQUIRED)
find_package(volePSI REQUIRED)
find_package(libOTe REQUIRED)
find_package(cryptoTools REQUIRED)

include_directories(include)
include_directories("${THIRDPARTY_INSTALL_DIR}/include")
include_directories("${THIRDPARTY_INSTALL_DIR}/include/secureJoin")

file(GLOB_RECURSE SRCS src/*.cpp)

set(ePSU_LINK_LIBS
    visa::volePSI
    oc::libOTe
    oc::cryptoTools
    OpenMP::OpenMP_CXX
    "${THIRDPARTY_INSTALL_DIR}/lib/libsecureJoin.a"
    pthread
)

if(EXISTS "${CMAKE_SOURCE_DIR}/test/test_ePSU.cpp")
    add_executable(ePSU test/test_ePSU.cpp ${SRCS})

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i.86")
        target_compile_options(ePSU PRIVATE -mpclmul -maes -msse2 -msse4.1 -O3)
    endif()

    target_link_libraries(ePSU ${ePSU_LINK_LIBS})
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/test/test_ssPMT.cpp")
    add_executable(ssPMT test/test_ssPMT.cpp ${SRCS})

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i.86")
        target_compile_options(ssPMT PRIVATE -mpclmul -maes -msse2 -msse4.1 -O3)
    endif()

    target_link_libraries(ssPMT ${ePSU_LINK_LIBS})
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/test/test_ssOTd.cpp")
    add_executable(ssOTd test/test_ssOTd.cpp ${SRCS})

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i.86")
        target_compile_options(ssOTd PRIVATE -mpclmul -maes -msse2 -msse4.1 -O3)
    endif()

    target_link_libraries(ssOTd ${ePSU_LINK_LIBS})
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/test/test_SoOPPRF.cpp")
    add_executable(SoOPPRF test/test_SoOPPRF.cpp ${SRCS})

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i.86")
        target_compile_options(SoOPPRF PRIVATE -mpclmul -maes -msse2 -msse4.1 -O3)
    endif()

    target_link_libraries(SoOPPRF ${ePSU_LINK_LIBS})
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/test/test_RPMT.cpp")
    add_executable(RPMT test/test_RPMT.cpp ${SRCS})

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i.86")
        target_compile_options(RPMT PRIVATE -mpclmul -maes -msse2 -msse4.1 -O3)
    endif()

    target_link_libraries(RPMT ${ePSU_LINK_LIBS})
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/test/test_PSU.cpp")
    add_executable(PSU test/test_PSU.cpp ${SRCS})

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i.86")
        target_compile_options(PSU PRIVATE -mpclmul -maes -msse2 -msse4.1 -O3)
    endif()

    target_link_libraries(PSU ${ePSU_LINK_LIBS})
endif()
