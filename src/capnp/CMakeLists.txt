set(src
    CapnpGame.h CapnpGame.cpp
)


add_library(capnp ${src})

target_link_libraries(capnp
    ${CAPNP_LIBRARIES}
    ${Boost_SYSTEM_LIBRARY})
