cmake_minimum_required(VERSION 3.26)

set(this_target "LockFreeQueue")
project(${this_target})

add_executable(${this_target})

target_sources (${this_target}
PRIVATE
	"main.cpp"
)

target_link_libraries(${this_target} PRIVATE Catch2::Catch2WithMain)