cmake_minimum_required(VERSION 2.8.7 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(argos-zero)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} /opt/capnproto/cmake)

find_package(Threads)
#set(Python_ADDITIONAL_VERSIONS 3.6)
find_package(PythonLibs REQUIRED)
find_package(Boost COMPONENTS system filesystem program_options python REQUIRED)
find_package(CapnProto REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fPIC -fpermissive")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og -ggdb3 -fno-omit-frame-pointer")# -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -funroll-loops -ggdb -fno-omit-frame-pointer")

include_directories(SYSTEM ${Boost_INCLUDE_DIR})
include_directories(SYSTEM ${PYTHON_INCLUDE_DIRS})
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/src/third_party)
include_directories(SYSTEM "${MXNET_PATH}/include/")
include_directories(SYSTEM "${MXNET_PATH}/dmlc-core/include/")
include_directories(SYSTEM "${MXNET_PATH}/nnvm/include/")
include_directories(SYSTEM "${MXNET_PATH}/cpp-package/include/")
include_directories(${PROJECT_SOURCE_DIR}/src/ego/goboard)
include_directories(${PROJECT_SOURCE_DIR}/src/ego/utils)
include_directories(${PROJECT_SOURCE_DIR}/src/ego/gtp)
#set(CAPNP_INCLUDE_DIRS "/usr/local/include")

link_directories("${MXNET_PATH}/lib")

add_definitions(-DBOARDSIZE=9)
add_definitions(${CAPNP_DEFINITIONS})

add_subdirectory(src)
