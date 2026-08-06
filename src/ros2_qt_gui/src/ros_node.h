#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <QString>

#include <rclcpp/rclcpp.hpp>
#include <yds_interfaces/msg/component_status.hpp>

#include <yds/ros2/application_event.h>
#include <yds/ros2/component_status.h>
#include <yds/ros2/topic_reception_monitor.h>
#include <yds/ros2/topic_reception_status.h>

namespace ros2qtgui {

/// @brief 有効なトピック監視設定
struct ComponentMonitorConfiguration {
	/// @brief 設定内で監視対象を識別する名前
	QString name;
	/// @brief GUIに表示するコンポーネント名
	QString displayName;
	/// @brief 購読するROSトピック名
	QString statusTopicName;
	/// @brief 期待するコンポーネントID。空文字の場合は照合しない
	QString expectedComponentId;
	/// @brief 受信タイムアウト時間（ミリ秒）
	std::int64_t timeoutMs;
	/// @brief 許容する状態生成時刻の古さ。0は検証無効
	std::int64_t maximumStatusAgeMs;
	/// @brief 許容する未来方向の時刻ずれ。0は検証無効
	std::int64_t maximumFutureSkewMs;
};

/// @brief GUIと連携するROS 2ノード
class RosNode final : public rclcpp::Node {
public:
	using HeartbeatCallback = std::function<void(std::uint64_t)>;
	using ApplicationEventCallback = std::function<void(const yds::ros2::ApplicationEvent&)>;
	using TopicReceptionStatusCallback =
		std::function<void(const yds::ros2::TopicReceptionStatus&)>;
	using ComponentStatusCallback =
		std::function<void(const yds::ros2::ComponentStatus&)>;

	/// @brief ROS 2ノードを生成する
	/// @param heartbeatCallback ハートビート更新時に呼び出す関数
	/// @param applicationEventCallback アプリケーションイベント発生時に呼び出す関数
	/// @param topicReceptionStatusCallback トピック受信状況の更新時に呼び出す関数
	/// @param componentStatusCallback コンポーネント状態の更新時に呼び出す関数
	/// @param options ROS 2ノードの生成オプション
	explicit RosNode(
		HeartbeatCallback heartbeatCallback,
		ApplicationEventCallback applicationEventCallback = ApplicationEventCallback(),
		TopicReceptionStatusCallback topicReceptionStatusCallback =
			TopicReceptionStatusCallback(),
		ComponentStatusCallback componentStatusCallback = ComponentStatusCallback(),
		const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

	/// @brief ハートビート周期を取得する
	/// @return ハートビート周期（ミリ秒）
	std::int64_t heartbeatIntervalMs() const noexcept;

	/// @brief GUI状態確認周期を取得する
	/// @return GUI状態確認周期（ミリ秒）
	std::int64_t guiStatusCheckIntervalMs() const noexcept;

	/// @brief 有効なトピック監視設定の一覧を取得する
	/// @return 有効なトピック監視設定の一覧
	const std::vector<ComponentMonitorConfiguration>& componentMonitorConfigurations()
		const noexcept;

private:
	void onHeartbeat() noexcept;
	void onMonitoredTopic(
		std::size_t monitorIndex,
		const yds_interfaces::msg::ComponentStatus::SharedPtr message) noexcept;
	void updateTopicReceptionStatus() noexcept;
	void notifyTopicReceptionStatus(
		yds::ros2::TopicReceptionMonitor& monitor) noexcept;
	void handleTopicReceptionTransition(
		const QString& topicName,
		yds::ros2::TopicReceptionTransition transition) noexcept;
	void handleComponentStateTransition(
		const yds::ros2::ComponentStatus& previousStatus,
		const yds::ros2::ComponentStatus& currentStatus,
		bool hasPreviousStatus) noexcept;
	void notifyComponentStatus(std::size_t monitorIndex) noexcept;
	void reportApplicationEvent(
		yds::ros2::ApplicationEventLevel level,
		const QString& message) noexcept;

	HeartbeatCallback heartbeatCallback_;
	ApplicationEventCallback applicationEventCallback_;
	TopicReceptionStatusCallback topicReceptionStatusCallback_;
	ComponentStatusCallback componentStatusCallback_;
	std::int64_t heartbeatIntervalMs_;
	std::int64_t guiStatusCheckIntervalMs_;
	std::vector<std::string> componentMonitorNames_;
	std::vector<ComponentMonitorConfiguration> componentMonitorConfigurations_;
	std::vector<std::unique_ptr<yds::ros2::TopicReceptionMonitor>>
		topicReceptionMonitors_;
	std::vector<yds::ros2::ComponentStatus> latestComponentStatuses_;
	std::vector<bool> hasComponentStatuses_;
	std::vector<bool> componentStatusDirty_;
	std::vector<std::uint32_t> componentStatusQualityIssues_;
	std::uint64_t heartbeatCount_;
	rclcpp::TimerBase::SharedPtr heartbeatTimer_;
	std::vector<rclcpp::Subscription<yds_interfaces::msg::ComponentStatus>::SharedPtr>
		monitoredTopicSubscriptions_;
	rclcpp::TimerBase::SharedPtr topicReceptionStatusTimer_;
};

}  // namespace ros2qtgui
