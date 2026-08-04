#pragma once

#include <chrono>
#include <mutex>
#include <string>

#include <QString>

#include <rclcpp/rclcpp.hpp>
#include <yds_interfaces/msg/equipment_status.hpp>

#include <yds/ros2/equipment_status.h>

namespace yds::ros2 {

/// @brief 設備状態の即時通知と定期通知を提供するROS 2共通基底ノード
class EquipmentStatusNode : public rclcpp::Node {
public:
	/// @brief 設備状態ノードを生成する
	/// @param nodeName ROSノード名
	/// @param defaultEquipmentId 設備IDの既定値
	/// @param defaultStatusTopicName 状態通知トピック名の既定値
	/// @param defaultPublishInterval 定期通知周期の既定値
	/// @param options ROSノードオプション
	EquipmentStatusNode(
		const std::string& nodeName,
		const QString& defaultEquipmentId,
		const QString& defaultStatusTopicName,
		std::chrono::milliseconds defaultPublishInterval = std::chrono::milliseconds(1000),
		const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

	/// @brief デストラクタ
	~EquipmentStatusNode() override = default;

	EquipmentStatusNode(const EquipmentStatusNode&) = delete;
	EquipmentStatusNode& operator=(const EquipmentStatusNode&) = delete;
	EquipmentStatusNode(EquipmentStatusNode&&) = delete;
	EquipmentStatusNode& operator=(EquipmentStatusNode&&) = delete;

	/// @brief 設備状態を更新して直ちに通知する
	/// @param state 新しい設備状態
	/// @param errorCode エラーコード。正常時は0
	/// @param message 状態の補足メッセージ
	/// @return 通知に成功した場合はtrue
	bool setEquipmentStatus(
		EquipmentState state,
		qint32 errorCode = 0,
		const QString& message = QString()) noexcept;

	/// @brief 現在の設備状態を取得する
	/// @return 現在の設備状態のコピー
	EquipmentStatus equipmentStatus() const;

	/// @brief 設備IDを取得する
	/// @return 設備ID
	const QString& equipmentId() const noexcept;

	/// @brief 状態通知トピック名を取得する
	/// @return 状態通知トピック名
	const QString& statusTopicName() const noexcept;

	/// @brief 定期通知周期を取得する
	/// @return 定期通知周期
	std::chrono::milliseconds statusPublishInterval() const noexcept;

private:
	bool publishEquipmentStatus() noexcept;

	QString equipmentId_;
	QString statusTopicName_;
	std::chrono::milliseconds statusPublishInterval_;
	mutable std::mutex statusMutex_;
	EquipmentStatus equipmentStatus_;
	rclcpp::Publisher<yds_interfaces::msg::EquipmentStatus>::SharedPtr statusPublisher_;
	rclcpp::TimerBase::SharedPtr statusTimer_;
};

}  // namespace yds::ros2
