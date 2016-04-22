cmake_minimum_required(VERSION 3.3)
project(p3)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(SOURCE_FILES ProxyServer.cpp ProxyConfig.cpp) # ProxyServer.cpp ProxyServer.h ProxyConfig.cpp ProxyConfig.h) # EchoServer.cpp EchoServer.h

add_executable(p3 ${SOURCE_FILES})


find_path(EVENT_INCLUDE_DIR event.h
    PATHS /Users/kopbob/dev/tmp/libevent_lib/local/include
    PATH_SUFFIXES event
)

find_library(EVENT_LIBRARY
    NAMES event
    PATHS /Users/kopbob/dev/tmp/libevent_lib/local/lib
)

set(EVENT_LIBRARIES ${EVENT_LIBRARY} )

message(${EVENT_INCLUDE_DIR})
message(${EVENT_LIBRARIES})

include_directories(${EVENT_INCLUDE_DIR})
link_directories(${EVENT_LIBRARIES})
target_link_libraries(p3 ${EVENT_LIBRARIES})
#
#find_library(EVENT_LIBRARY
#    NAMES event
#    PATHS ${LIBEVENT_LIBRARIES}
#)
#
#target_link_libraries(p3 event)
