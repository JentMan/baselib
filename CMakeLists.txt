
set(SRC 
	socket.cpp
	thread.cpp
	exception.cpp
	logger.cpp
	daemon.cpp
)

add_library(base ${SRC})
target_link_libraries(base pthread)
