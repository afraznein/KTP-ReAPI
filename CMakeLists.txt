cmake_minimum_required(VERSION 3.1)
project(reapi CXX)

if (WIN32)
	message(FATAL_ERROR "CMakeLists.txt Windows platform isn't supported yet. Use msvc/reapi.sln instead it!")
endif()


add_subdirectory(reapi)
