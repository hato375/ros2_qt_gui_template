#pragma once

#include <cstdint>
#include <functional>

#include <rclcpp/rclcpp.hpp>

namespace ros2qtgui {

/// @brief GUIと連携するROS 2ノード
class RosNode final : public rclcpp::Node {
public:
	using HeartbeatCallback = std::function<void(std::uint64_t)>;

	/// @brief ROS 2ノードを生成する
	/// @param heartbeatCallback ハートビート更新時に呼び出す関数
	explicit RosNode(HeartbeatCallback heartbeatCallback);

private:
	void onHeartbeat() noexcept;

	HeartbeatCallback heartbeatCallback_;
	std::uint64_t heartbeatCount_;
	rclcpp::TimerBase::SharedPtr heartbeatTimer_;
};

}  // namespace ros2qtgui
