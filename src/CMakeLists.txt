add_subdirectory(argos)

add_executable(speedtest speedtest.cpp)
target_link_libraries(speedtest argos mxnet)

add_executable(selfplay selfplay.cpp)
target_link_libraries(selfplay argos mxnet)

add_executable(rollout rollout.cpp)
target_link_libraries(rollout argos mxnet)

add_executable(gtp gtp.cpp)
target_link_libraries(gtp argos mxnet)
