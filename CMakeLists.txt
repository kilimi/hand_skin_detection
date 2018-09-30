cmake_minimum_required(VERSION 2.8)
project( skin_detection )

find_package( OpenCV REQUIRED )
find_package(Boost COMPONENTS system filesystem REQUIRED)

SET(HEADERS
    process_directory.h
)

SET(SOURCES
    process_directory.cpp
)

add_executable( skin_detection skin_detection.cpp ${HEADERS} ${SOURCES})
target_link_libraries( skin_detection ${OpenCV_LIBS} ${Boost_LIBRARIES})
