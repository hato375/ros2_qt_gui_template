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
find_package(yds_interfaces REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Widgets)
find_package(yds_ros2 REQUIRED)
find_package(yds_ros2_widgets REQUIRED)

add_executable(${PROJECT_NAME}
	src/ros_qt_bridge.h
	src/main.cpp
	src/main_window.cpp
	src/ros_node.cpp
	src/ros_qt_bridge.cpp
)

target_link_libraries(
	${PROJECT_NAME}
	Qt5::Widgets
	yds_ros2::yds_ros2
	yds_ros2_widgets::yds_ros2_widgets
)
ament_target_dependencies(${PROJECT_NAME} rcl_interfaces rclcpp yds_interfaces)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()

if(BUILD_TESTING)
	find_package(ament_cmake_gtest REQUIRED)
	find_package(launch_testing_ament_cmake REQUIRED)
	find_package(rclcpp_lifecycle REQUIRED)

	ament_add_gtest(ros2_qt_gui_test
		SKIP_LINKING_MAIN_LIBRARIES
		test/ros2_qt_gui_test.cpp
		src/main_window.cpp
		src/ros_node.cpp
		src/ros_qt_bridge.cpp
		src/ros_qt_bridge.h
	)
	target_include_directories(ros2_qt_gui_test PRIVATE src)
	target_link_libraries(
		ros2_qt_gui_test
		Qt5::Widgets
		yds_ros2::yds_ros2
		yds_ros2_widgets::yds_ros2_widgets
	)
	ament_target_dependencies(
		ros2_qt_gui_test
		rcl_interfaces
		rclcpp
		rclcpp_lifecycle
		yds_interfaces
	)

	if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
		target_compile_options(ros2_qt_gui_test PRIVATE -Wall -Wextra -Wpedantic)
	elseif(MSVC)
		target_compile_options(ros2_qt_gui_test PRIVATE /W4)
	endif()

	add_launch_test(
		test/ros2_qt_gui_yaml_launch_test.py
		TIMEOUT 60
	)

	add_executable(ros2_qt_gui_configuration_probe
		test/ros2_qt_gui_configuration_probe.cpp
		src/ros_node.cpp
	)
	target_include_directories(ros2_qt_gui_configuration_probe PRIVATE src)
	target_link_libraries(
		ros2_qt_gui_configuration_probe
		Qt5::Widgets
		yds_ros2::yds_ros2
	)
	ament_target_dependencies(
		ros2_qt_gui_configuration_probe
		rcl_interfaces
		rclcpp
		yds_interfaces
	)
	if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
		target_compile_options(
			ros2_qt_gui_configuration_probe PRIVATE -Wall -Wextra -Wpedantic)
	elseif(MSVC)
		target_compile_options(ros2_qt_gui_configuration_probe PRIVATE /W4)
	endif()
	install(TARGETS ros2_qt_gui_configuration_probe
		DESTINATION lib/${PROJECT_NAME}
	)

	add_launch_test(
		test/ros2_qt_gui_invalid_yaml_launch_test.py
		TIMEOUT 30
	)
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
