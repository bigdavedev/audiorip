cmake_minimum_required ( VERSION 3.2 )

project ( audiorip C )

include_directories ( include )

set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall -Wextra -Wpedantic" )

set ( SOURCES main.c
              audiorip.c
              audiorip_cdrom.c )

add_executable ( ${PROJECT_NAME} ${SOURCES} )
