#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

#include <yds/ros2/application_event.h>
#include <yds/ros2/topic_reception_status.h>

namespace ros2qtgui {

/// @brief GUIと連携するROS 2ノード
class RosNode final : public rclcpp::Node {
public:
	using HeartbeatCallback = std::function<void(std::uint64_t)>;
	using ApplicationEventCallback = std::function<void(const yds::ros2::ApplicationEvent&)>;
	using TopicReceptionStatusCallback =
		std::function<void(const yds::ros2::TopicReceptionStatus&)>;

	/// @brief ROS 2ノードを生成する
	/// @param heartbeatCallback ハートビート更新時に呼び出す関数
	/// @param applicationEventCallback アプリケーションイベント発生時に呼び出す関数
	/// @param topicReceptionStatusCallback トピック受信状況の更新時に呼び出す関数
	/// @param options ROS 2ノードの生成オプション
	explicit RosNode(
		HeartbeatCallback heartbeatCallback,
		ApplicationEventCallback applicationEventCallback = ApplicationEventCallback(),
		TopicReceptionStatusCallback topicReceptionStatusCallback =
			TopicReceptionStatusCallback(),
		const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

	/// @brief ハートビート周期を取得する
	/// @return ハートビート周期（ミリ秒）
	std::int64_t heartbeatIntervalMs() const noexcept;

	/// @brief GUI状態確認周期を取得する
	/// @return GUI状態確認周期（ミリ秒）
	std::int64_t guiStatusCheckIntervalMs() const noexcept;

	/// @brief 監視するROSトピック名を取得する
	/// @return 監視するROSトピック名
	const std::string& monitoredTopic() const noexcept;

	/// @brief トピック受信タイムアウト時間を取得する
	/// @return トピック受信タイムアウト時間（ミリ秒）
	std::int64_t topicReceptionTimeoutMs() const noexcept;

private:
	void onHeartbeat() noexcept;
	void onMonitoredTopic(const std_msgs::msg::String::SharedPtr message) noexcept;
	void updateTopicReceptionStatus() noexcept;
	void notifyTopicReceptionStatus() noexcept;
	void reportApplicationEvent(
		yds::ros2::ApplicationEventLevel level,
		const QString& message) noexcept;

	HeartbeatCallback heartbeatCallback_;
	ApplicationEventCallback applicationEventCallback_;
	TopicReceptionStatusCallback topicReceptionStatusCallback_;
	std::int64_t heartbeatIntervalMs_;
	std::int64_t guiStatusCheckIntervalMs_;
	std::string monitoredTopic_;
	std::int64_t topicReceptionTimeoutMs_;
	std::uint64_t heartbeatCount_;
	rclcpp::TimerBase::SharedPtr heartbeatTimer_;
	rclcpp::Subscription<std_msgs::msg::String>::SharedPtr monitoredTopicSubscription_;
	rclcpp::TimerBase::SharedPtr topicReceptionStatusTimer_;
	yds::ros2::TopicReceptionStatus topicReceptionStatus_;
	std::chrono::steady_clock::time_point lastTopicReceptionTime_;
	bool topicReceptionStatusDirty_;
};

}  // namespace ros2qtgui
