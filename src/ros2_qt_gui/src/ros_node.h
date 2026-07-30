#pragma once

#include <cstdint>
#include <functional>

#include <rclcpp/rclcpp.hpp>

#include <yds/ros2/application_event.h>

namespace ros2qtgui {

/// @brief GUIと連携するROS 2ノード
class RosNode final : public rclcpp::Node {
public:
	using HeartbeatCallback = std::function<void(std::uint64_t)>;
	using ApplicationEventCallback = std::function<void(const yds::ros2::ApplicationEvent&)>;

	/// @brief ROS 2ノードを生成する
	/// @param heartbeatCallback ハートビート更新時に呼び出す関数
	/// @param applicationEventCallback アプリケーションイベント発生時に呼び出す関数
	/// @param options ROS 2ノードの生成オプション
	explicit RosNode(
		HeartbeatCallback heartbeatCallback,
		ApplicationEventCallback applicationEventCallback = ApplicationEventCallback(),
		const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

	/// @brief ハートビート周期を取得する
	/// @return ハートビート周期（ミリ秒）
	std::int64_t heartbeatIntervalMs() const noexcept;

	/// @brief GUI状態確認周期を取得する
	/// @return GUI状態確認周期（ミリ秒）
	std::int64_t guiStatusCheckIntervalMs() const noexcept;

private:
	void onHeartbeat() noexcept;
	void reportApplicationEvent(
		yds::ros2::ApplicationEventLevel level,
		const QString& message) noexcept;

	HeartbeatCallback heartbeatCallback_;
	ApplicationEventCallback applicationEventCallback_;
	std::int64_t heartbeatIntervalMs_;
	std::int64_t guiStatusCheckIntervalMs_;
	std::uint64_t heartbeatCount_;
	rclcpp::TimerBase::SharedPtr heartbeatTimer_;
};

}  // namespace ros2qtgui
