#pragma once

#include <chrono>
#include <mutex>

#include <QString>

#include <rclcpp/rclcpp.hpp>
#include <yds_interfaces/msg/component_status.hpp>

#include <yds/ros2/component_status.h>

namespace yds::ros2 {

/// @brief コンポーネント状態Publisherの設定
struct ComponentStatusPublisherConfiguration {
	/// @brief コンポーネントID
	QString componentId;
	/// @brief 状態通知トピック名
	QString statusTopicName;
	/// @brief 最新状態の定期通知周期
	std::chrono::milliseconds publishInterval;
};

/// @brief ROSノードへコンポーネント状態通知機能を追加する
class ComponentStatusPublisher final {
public:
	/// @brief Node Interfaceを持つノードへコンポーネント状態通知機能を追加する
	/// @tparam NodeT `rclcpp::Node`または互換Node Interfaceを提供する型
	/// @param node PublisherとTimerを所有するノード
	/// @param configuration コンポーネント状態Publisherの設定
	template<typename NodeT>
	ComponentStatusPublisher(
		NodeT& node,
		const ComponentStatusPublisherConfiguration& configuration)
		: ComponentStatusPublisher(
			node.get_node_base_interface(),
			node.get_node_timers_interface(),
			node.get_node_parameters_interface(),
			node.get_node_topics_interface(),
			node.get_logger(),
			configuration) {}

	/// @brief デストラクタ
	~ComponentStatusPublisher() = default;

	ComponentStatusPublisher(const ComponentStatusPublisher&) = delete;
	ComponentStatusPublisher& operator=(const ComponentStatusPublisher&) = delete;
	ComponentStatusPublisher(ComponentStatusPublisher&&) = delete;
	ComponentStatusPublisher& operator=(ComponentStatusPublisher&&) = delete;

	/// @brief コンポーネント状態を更新して直ちに通知する
	/// @param state 新しいコンポーネント状態
	/// @param errorCode エラーコード。正常時は0
	/// @param message 状態の補足メッセージ
	/// @return 通知に成功した場合はtrue
	bool setStatus(
		ComponentState state,
		qint32 errorCode = 0,
		const QString& message = QString()) noexcept;

	/// @brief 現在のコンポーネント状態を取得する
	/// @return 現在のコンポーネント状態のコピー
	ComponentStatus status() const;

	/// @brief コンポーネントIDを取得する
	/// @return コンポーネントID
	const QString& componentId() const noexcept;

	/// @brief 状態通知トピック名を取得する
	/// @return 状態通知トピック名
	const QString& statusTopicName() const noexcept;

	/// @brief 定期通知周期を取得する
	/// @return 定期通知周期
	std::chrono::milliseconds publishInterval() const noexcept;

private:
	ComponentStatusPublisher(
		const rclcpp::node_interfaces::NodeBaseInterface::SharedPtr& nodeBase,
		const rclcpp::node_interfaces::NodeTimersInterface::SharedPtr& nodeTimers,
		rclcpp::node_interfaces::NodeParametersInterface::SharedPtr nodeParameters,
		rclcpp::node_interfaces::NodeTopicsInterface::SharedPtr nodeTopics,
		const rclcpp::Logger& logger,
		const ComponentStatusPublisherConfiguration& configuration);

	bool publishStatus() noexcept;

	rclcpp::Logger logger_;
	ComponentStatusPublisherConfiguration configuration_;
	mutable std::mutex statusMutex_;
	ComponentStatus status_;
	ComponentState lastWarningState_;
	qint32 lastWarningErrorCode_;
	bool hasLastValidationWarning_;
	bool lastUnexpectedErrorCode_;
	bool lastMissingErrorCode_;
	bool lastMissingMessage_;
	rclcpp::Publisher<yds_interfaces::msg::ComponentStatus>::SharedPtr publisher_;
	rclcpp::TimerBase::SharedPtr timer_;
};

}  // namespace yds::ros2
