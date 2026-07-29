#include "ros_node.h"

#include <chrono>
#include <exception>
#include <utility>

namespace ros2qtgui {

RosNode::RosNode(HeartbeatCallback heartbeatCallback)
	: Node("ros2_qt_gui_node"),
	  heartbeatCallback_(std::move(heartbeatCallback)),
	  heartbeatCount_(0) {
	using namespace std::chrono_literals;

	heartbeatTimer_ = create_wall_timer(1s, [this]() {
		onHeartbeat();
	});
	RCLCPP_INFO(get_logger(), "ROS 2 node started");
}

void RosNode::onHeartbeat() noexcept {
	++heartbeatCount_;
	RCLCPP_DEBUG(get_logger(), "Heartbeat: %lu", static_cast<unsigned long>(heartbeatCount_));

	try {
		heartbeatCallback_(heartbeatCount_);
	} catch (const std::exception& exception) {
		RCLCPP_ERROR(get_logger(), "Heartbeat callback failed: %s", exception.what());
	} catch (...) {
		RCLCPP_ERROR(get_logger(), "Heartbeat callback failed with an unknown error");
	}
}

}  // namespace ros2qtgui
