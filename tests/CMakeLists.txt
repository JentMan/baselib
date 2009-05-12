
add_executable(testsocket testsocket.cpp)
target_link_libraries(testsocket base)

add_executable(testlogger testlogger.cpp)
target_link_libraries(testlogger base)

add_executable(testdaemon testdaemon.cpp)
target_link_libraries(testdaemon base)