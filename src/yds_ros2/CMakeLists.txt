cmake_minimum_required(VERSION 3.8)
project(yds_ros2)

if(NOT CMAKE_CXX_STANDARD)
	set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core)

add_library(${PROJECT_NAME}
	src/executor_runner.cpp
	src/topic_reception_monitor.cpp
)

target_include_directories(${PROJECT_NAME}
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
)
target_link_libraries(${PROJECT_NAME} Qt5::Core)
ament_target_dependencies(${PROJECT_NAME} rclcpp)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()

if(BUILD_TESTING)
	find_package(ament_cmake_gtest REQUIRED)

	ament_add_gtest(yds_ros2_test
		test/equipment_status_test.cpp
		test/executor_runner_test.cpp
		test/topic_reception_monitor_test.cpp
	)
	target_link_libraries(yds_ros2_test ${PROJECT_NAME})
	ament_target_dependencies(yds_ros2_test rclcpp)

	if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
		target_compile_options(yds_ros2_test PRIVATE -Wall -Wextra -Wpedantic)
	elseif(MSVC)
		target_compile_options(yds_ros2_test PRIVATE /W4)
	endif()
endif()

install(
	TARGETS ${PROJECT_NAME}
	EXPORT export_${PROJECT_NAME}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
)

install(
	DIRECTORY include/
	DESTINATION include
)

ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(Qt5Core rclcpp)

ament_package()
