#pragma once

#include <chrono>
#include <memory>
#include <string>

#include <QString>

#include <rclcpp/rclcpp.hpp>
#include <yds/ros2/component_status.h>
#include <yds/ros2/component_status_publisher.h>

namespace yds::ros2 {

/// @brief コンポーネント状態の即時通知と定期通知を提供するROS 2共通基底ノード
class ComponentStatusNode : public rclcpp::Node {
public:
	/// @brief コンポーネント状態ノードを生成する
	/// @param nodeName ROSノード名
	/// @param defaultComponentId コンポーネントIDの既定値
	/// @param defaultStatusTopicName 状態通知トピック名の既定値
	/// @param defaultPublishInterval 定期通知周期の既定値
	/// @param options ROSノードオプション
	ComponentStatusNode(
		const std::string& nodeName,
		const QString& defaultComponentId,
		const QString& defaultStatusTopicName,
		std::chrono::milliseconds defaultPublishInterval = std::chrono::milliseconds(1000),
		const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

	/// @brief デストラクタ
	~ComponentStatusNode() override = default;

	ComponentStatusNode(const ComponentStatusNode&) = delete;
	ComponentStatusNode& operator=(const ComponentStatusNode&) = delete;
	ComponentStatusNode(ComponentStatusNode&&) = delete;
	ComponentStatusNode& operator=(ComponentStatusNode&&) = delete;

	/// @brief コンポーネント状態を更新して直ちに通知する
	/// @param state 新しいコンポーネント状態
	/// @param errorCode エラーコード。正常時は0
	/// @param message 状態の補足メッセージ
	/// @return 通知に成功した場合はtrue
	bool setComponentStatus(
		ComponentState state,
		qint32 errorCode = 0,
		const QString& message = QString()) noexcept;

	/// @brief 現在のコンポーネント状態を取得する
	/// @return 現在のコンポーネント状態のコピー
	ComponentStatus componentStatus() const;

	/// @brief コンポーネントIDを取得する
	/// @return コンポーネントID
	const QString& componentId() const noexcept;

	/// @brief 状態通知トピック名を取得する
	/// @return 状態通知トピック名
	const QString& statusTopicName() const noexcept;

	/// @brief 定期通知周期を取得する
	/// @return 定期通知周期
	std::chrono::milliseconds statusPublishInterval() const noexcept;

private:
	std::unique_ptr<ComponentStatusPublisher> statusPublisher_;
};

}  // namespace yds::ros2
