cmake_minimum_required ( VERSION 3.2 )

project ( spindown C )

set ( SOURCES spindown.c )

add_executable ( ${PROJECT_NAME} ${SOURCES} )
