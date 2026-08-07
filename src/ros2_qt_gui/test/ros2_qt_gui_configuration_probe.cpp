#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include "ros_node.h"

int main(int argc, char* argv[]) {
	rclcpp::init(argc, argv);
	try {
		auto node = std::make_shared<ros2qtgui::RosNode>([](std::uint64_t) {
		});
		rclcpp::shutdown();
		return 0;
	} catch (const std::exception& exception) {
		std::cerr << "Configuration error: " << exception.what() << std::endl;
		if (rclcpp::ok()) {
			rclcpp::shutdown();
		}
		return 1;
	}
}
