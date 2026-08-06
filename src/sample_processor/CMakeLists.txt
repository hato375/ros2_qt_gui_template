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
find_package(rclcpp_lifecycle REQUIRED)
find_package(std_srvs REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core)
find_package(yds_interfaces REQUIRED)
find_package(yds_ros2 REQUIRED)

add_library(sample_processor_node
	src/sample_processor_node.cpp
)
target_include_directories(sample_processor_node
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
)
target_link_libraries(sample_processor_node Qt5::Core yds_ros2::yds_ros2)
ament_target_dependencies(sample_processor_node rcl_interfaces rclcpp std_srvs)

add_library(sample_lifecycle_processor_node
	src/sample_lifecycle_processor_node.cpp
)
target_include_directories(sample_lifecycle_processor_node
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
)
target_link_libraries(sample_lifecycle_processor_node Qt5::Core yds_ros2::yds_ros2)
ament_target_dependencies(
	sample_lifecycle_processor_node
	rcl_interfaces
	rclcpp
	rclcpp_lifecycle
)

add_executable(sample_processor
	src/main.cpp
)
target_link_libraries(sample_processor sample_processor_node)
ament_target_dependencies(sample_processor rclcpp)

add_executable(sample_lifecycle_processor
	src/lifecycle_main.cpp
)
target_link_libraries(sample_lifecycle_processor sample_lifecycle_processor_node)
ament_target_dependencies(sample_lifecycle_processor rclcpp rclcpp_lifecycle)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(sample_processor_node PRIVATE -Wall -Wextra -Wpedantic)
	target_compile_options(sample_lifecycle_processor_node PRIVATE -Wall -Wextra -Wpedantic)
	target_compile_options(sample_processor PRIVATE -Wall -Wextra -Wpedantic)
	target_compile_options(sample_lifecycle_processor PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
	target_compile_options(sample_processor_node PRIVATE /W4)
	target_compile_options(sample_lifecycle_processor_node PRIVATE /W4)
	target_compile_options(sample_processor PRIVATE /W4)
	target_compile_options(sample_lifecycle_processor PRIVATE /W4)
endif()

if(BUILD_TESTING)
	find_package(ament_cmake_gtest REQUIRED)
	find_package(launch_testing_ament_cmake REQUIRED)

	ament_add_gtest(sample_processor_test
		SKIP_LINKING_MAIN_LIBRARIES
		test/sample_processor_test.cpp
		test/sample_lifecycle_processor_test.cpp
	)
	target_link_libraries(
		sample_processor_test
		sample_processor_node
		sample_lifecycle_processor_node
	)
	ament_target_dependencies(
		sample_processor_test
		rclcpp
		rclcpp_lifecycle
		std_srvs
		yds_interfaces
	)

	if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
		target_compile_options(sample_processor_test PRIVATE -Wall -Wextra -Wpedantic)
	elseif(MSVC)
		target_compile_options(sample_processor_test PRIVATE /W4)
	endif()

	add_launch_test(
		test/sample_lifecycle_processor_launch_test.py
		TIMEOUT 60
	)
endif()

install(TARGETS
	sample_processor
	sample_lifecycle_processor
	DESTINATION lib/${PROJECT_NAME}
)

install(TARGETS sample_processor_node sample_lifecycle_processor_node
	EXPORT export_${PROJECT_NAME}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
)

install(DIRECTORY include/
	DESTINATION include
)

install(DIRECTORY launch config
	DESTINATION share/${PROJECT_NAME}
)

ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_include_directories(include)
ament_export_dependencies(
	rcl_interfaces
	rclcpp
	rclcpp_lifecycle
	std_srvs
	Qt5Core
	yds_interfaces
	yds_ros2
)

ament_package()
