set(src
    Engine.h Engine.cpp
    Tree.h Tree.cpp
    Node.h Node.cpp
    Position.h Position.cpp
    Util.h Util.cpp
    Network.h Network.cpp
    ThreadPool.h ThreadPool.cpp
    TimeControl.h TimeControl.cpp
    BatchProcessor.h BatchProcessor.cpp
    Statistics.h
    Config.h
    SpinLock.h
    Collector.h Collector.cpp)

file(GLOB_RECURSE ego_src
    "${PROJECT_SOURCE_DIR}/src/ego/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/ego/*.hpp"
    "${PROJECT_SOURCE_DIR}/src/ego/*.h")

include_directories("${CMAKE_BINARY_DIR}/src/capnp/")

add_library(argos ${src} ${ego_src})

target_link_libraries(argos
    serialization
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY})
