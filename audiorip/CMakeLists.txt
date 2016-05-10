cmake_minimum_required ( VERSION 3.2 )

project ( audiorip C )

set ( SOURCES audiorip.c )

add_executable ( ${PROJECT_NAME} ${SOURCES} )
