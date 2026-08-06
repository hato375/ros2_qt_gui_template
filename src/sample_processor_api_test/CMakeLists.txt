cmake_minimum_required(VERSION 3.8)
project(sample_processor_api_test)

if(NOT CMAKE_CXX_STANDARD)
	set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ament_cmake REQUIRED)

if(BUILD_TESTING)
	find_package(ament_cmake_gtest REQUIRED)
	find_package(rclcpp REQUIRED)
	find_package(sample_processor REQUIRED)

	ament_add_gtest(sample_processor_public_api_test
		SKIP_LINKING_MAIN_LIBRARIES
		test/sample_processor_public_api_test.cpp
	)
	target_link_libraries(
		sample_processor_public_api_test
		sample_processor::sample_lifecycle_processor_node
	)
	ament_target_dependencies(sample_processor_public_api_test rclcpp)

	if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
		target_compile_options(sample_processor_public_api_test PRIVATE -Wall -Wextra -Wpedantic)
	elseif(MSVC)
		target_compile_options(sample_processor_public_api_test PRIVATE /W4)
	endif()
endif()

ament_package()
