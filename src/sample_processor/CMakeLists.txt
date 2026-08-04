cmake_minimum_required(VERSION 3.8)
project(sample_processor)

if(NOT CMAKE_CXX_STANDARD)
	set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ament_cmake REQUIRED)
find_package(rcl_interfaces REQUIRED)
find_package(rclcpp REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core)
find_package(yds_interfaces REQUIRED)
find_package(yds_ros2 REQUIRED)

add_library(sample_processor_node
	src/sample_processor_node.cpp
)
target_include_directories(sample_processor_node
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(sample_processor_node Qt5::Core yds_ros2::yds_ros2)
ament_target_dependencies(sample_processor_node rcl_interfaces rclcpp)

add_executable(sample_processor
	src/main.cpp
)
target_link_libraries(sample_processor sample_processor_node)
ament_target_dependencies(sample_processor rclcpp)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(sample_processor_node PRIVATE -Wall -Wextra -Wpedantic)
	target_compile_options(sample_processor PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
	target_compile_options(sample_processor_node PRIVATE /W4)
	target_compile_options(sample_processor PRIVATE /W4)
endif()

if(BUILD_TESTING)
	find_package(ament_cmake_gtest REQUIRED)

	ament_add_gtest(sample_processor_test
		SKIP_LINKING_MAIN_LIBRARIES
		test/sample_processor_test.cpp
	)
	target_link_libraries(sample_processor_test sample_processor_node)
	ament_target_dependencies(sample_processor_test rclcpp yds_interfaces)

	if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
		target_compile_options(sample_processor_test PRIVATE -Wall -Wextra -Wpedantic)
	elseif(MSVC)
		target_compile_options(sample_processor_test PRIVATE /W4)
	endif()
endif()

install(TARGETS sample_processor
	DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch config
	DESTINATION share/${PROJECT_NAME}
)

ament_package()
