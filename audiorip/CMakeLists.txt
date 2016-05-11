cmake_minimum_required ( VERSION 3.2 )

project ( audiorip C )

include_directories ( include )

set ( SOURCES main.c
              audiorip.c )

add_executable ( ${PROJECT_NAME} ${SOURCES} )
