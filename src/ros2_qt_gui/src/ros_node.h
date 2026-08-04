#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <QString>

#include <rclcpp/rclcpp.hpp>
#include <yds_interfaces/msg/equipment_status.hpp>

#include <yds/ros2/application_event.h>
#include <yds/ros2/equipment_status.h>
#include <yds/ros2/topic_reception_monitor.h>
#include <yds/ros2/topic_reception_status.h>

namespace ros2qtgui {

/// @brief 有効なトピック監視設定
struct TopicMonitorConfiguration {
	/// @brief 設定内で監視対象を識別する名前
	QString name;
	/// @brief 購読するROSトピック名
	QString topicName;
	/// @brief 受信タイムアウト時間（ミリ秒）
	std::int64_t timeoutMs;
};

/// @brief GUIと連携するROS 2ノード
class RosNode final : public rclcpp::Node {
public:
	using HeartbeatCallback = std::function<void(std::uint64_t)>;
	using ApplicationEventCallback = std::function<void(const yds::ros2::ApplicationEvent&)>;
	using TopicReceptionStatusCallback =
		std::function<void(const yds::ros2::TopicReceptionStatus&)>;
	using EquipmentStatusCallback =
		std::function<void(const yds::ros2::EquipmentStatus&)>;

	/// @brief ROS 2ノードを生成する
	/// @param heartbeatCallback ハートビート更新時に呼び出す関数
	/// @param applicationEventCallback アプリケーションイベント発生時に呼び出す関数
	/// @param topicReceptionStatusCallback トピック受信状況の更新時に呼び出す関数
	/// @param equipmentStatusCallback 設備状態の更新時に呼び出す関数
	/// @param options ROS 2ノードの生成オプション
	explicit RosNode(
		HeartbeatCallback heartbeatCallback,
		ApplicationEventCallback applicationEventCallback = ApplicationEventCallback(),
		TopicReceptionStatusCallback topicReceptionStatusCallback =
			TopicReceptionStatusCallback(),
		EquipmentStatusCallback equipmentStatusCallback = EquipmentStatusCallback(),
		const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

	/// @brief ハートビート周期を取得する
	/// @return ハートビート周期（ミリ秒）
	std::int64_t heartbeatIntervalMs() const noexcept;

	/// @brief GUI状態確認周期を取得する
	/// @return GUI状態確認周期（ミリ秒）
	std::int64_t guiStatusCheckIntervalMs() const noexcept;

	/// @brief 有効なトピック監視設定の一覧を取得する
	/// @return 有効なトピック監視設定の一覧
	const std::vector<TopicMonitorConfiguration>& topicMonitorConfigurations()
		const noexcept;

private:
	void onHeartbeat() noexcept;
	void onMonitoredTopic(
		std::size_t monitorIndex,
		const yds_interfaces::msg::EquipmentStatus::SharedPtr message) noexcept;
	void updateTopicReceptionStatus() noexcept;
	void notifyTopicReceptionStatus(
		yds::ros2::TopicReceptionMonitor& monitor) noexcept;
	void handleTopicReceptionTransition(
		const QString& topicName,
		yds::ros2::TopicReceptionTransition transition) noexcept;
	void handleEquipmentStateTransition(
		const yds::ros2::EquipmentStatus& previousStatus,
		const yds::ros2::EquipmentStatus& currentStatus,
		bool hasPreviousStatus) noexcept;
	void notifyEquipmentStatus(std::size_t monitorIndex) noexcept;
	void reportApplicationEvent(
		yds::ros2::ApplicationEventLevel level,
		const QString& message) noexcept;

	HeartbeatCallback heartbeatCallback_;
	ApplicationEventCallback applicationEventCallback_;
	TopicReceptionStatusCallback topicReceptionStatusCallback_;
	EquipmentStatusCallback equipmentStatusCallback_;
	std::int64_t heartbeatIntervalMs_;
	std::int64_t guiStatusCheckIntervalMs_;
	std::vector<std::string> topicMonitorNames_;
	std::vector<TopicMonitorConfiguration> topicMonitorConfigurations_;
	std::vector<std::unique_ptr<yds::ros2::TopicReceptionMonitor>>
		topicReceptionMonitors_;
	std::vector<yds::ros2::EquipmentStatus> latestEquipmentStatuses_;
	std::vector<bool> hasEquipmentStatuses_;
	std::vector<bool> equipmentStatusDirty_;
	std::uint64_t heartbeatCount_;
	rclcpp::TimerBase::SharedPtr heartbeatTimer_;
	std::vector<rclcpp::Subscription<yds_interfaces::msg::EquipmentStatus>::SharedPtr>
		monitoredTopicSubscriptions_;
	rclcpp::TimerBase::SharedPtr topicReceptionStatusTimer_;
};

}  // namespace ros2qtgui
