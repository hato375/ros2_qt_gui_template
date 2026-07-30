cmake_minimum_required(VERSION 3.8)
project(ros2_qt_gui)

if(NOT CMAKE_CXX_STANDARD)
	set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(ament_cmake REQUIRED)
find_package(rcl_interfaces REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Widgets)
find_package(yds_ros2 REQUIRED)

add_executable(${PROJECT_NAME}
	src/ros_qt_bridge.h
	src/main.cpp
	src/main_window.cpp
	src/ros_node.cpp
	src/ros_qt_bridge.cpp
)

target_link_libraries(${PROJECT_NAME} Qt5::Widgets yds_ros2::yds_ros2)
ament_target_dependencies(${PROJECT_NAME} rcl_interfaces rclcpp std_msgs)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()

if(BUILD_TESTING)
	find_package(ament_cmake_gtest REQUIRED)

	ament_add_gtest(ros2_qt_gui_test
		SKIP_LINKING_MAIN_LIBRARIES
		test/ros2_qt_gui_test.cpp
		src/main_window.cpp
		src/ros_node.cpp
		src/ros_qt_bridge.cpp
		src/ros_qt_bridge.h
	)
	target_include_directories(ros2_qt_gui_test PRIVATE src)
	target_link_libraries(ros2_qt_gui_test Qt5::Widgets yds_ros2::yds_ros2)
	ament_target_dependencies(ros2_qt_gui_test rcl_interfaces rclcpp std_msgs)

	if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
		target_compile_options(ros2_qt_gui_test PRIVATE -Wall -Wextra -Wpedantic)
	elseif(MSVC)
		target_compile_options(ros2_qt_gui_test PRIVATE /W4)
	endif()
endif()

install(TARGETS ${PROJECT_NAME}
	DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch
	DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY config
	DESTINATION share/${PROJECT_NAME}
)

ament_package()
