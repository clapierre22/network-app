cmake_minimum_required(VERSION 3.10)
project(network-app C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -Wall -Wextra")

# Directories
include_directories(
	${CMAKE_SOURCE_DIR}/src/base
	${CMAKE_SOURCE_DIR}/src/network
	${CMAKE_SOURCE_DIR}/src/routing
	${CMAKE_SOURCE_DIR}/src/util
)

# Source File
file(GLOB_RECURSE SOURCES
	"src/base/*.c"
	"src/network/*.c"
	"src/routing/*.c"
	"src/util/*.c"
)

# Main Executable
add_executable(user ${SOURCES})
target_link_libraries(user pthread)

# Set Output Directory
set_target_properties(user PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Tools
add_executable(random_conn tools/random_conn.c)
set_target_properties(random_conn PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
