set(src
    CapnpGame.capnp.h CapnpGame.capnp.c++
)

add_library(serialization ${src})

target_link_libraries(serialization
    capnp
    ${Boost_SYSTEM_LIBRARY})
