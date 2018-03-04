set(CAPNPC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
capnp_generate_cpp(CAPNP_SRCS CAPNP_HDRS CapnpGame.capnp)

add_library(serialization ${CAPNP_SRCS})

target_link_libraries(serialization
    ${CAPNP_LIBRARIES}
    ${Boost_SYSTEM_LIBRARY})
