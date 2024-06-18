cmake_minimum_required(VERSION 3.12)

set(this_target "AsyncLogger")
project(${this_target})
add_executable(${this_target})

target_sources (${this_target}
PRIVATE
	"main.cpp"
)