#pragma once

#include <cstdint>

#include <QDateTime>
#include <QString>

#include <builtin_interfaces/msg/time.hpp>
#include <yds_interfaces/msg/equipment_status.hpp>

#include <yds/ros2/equipment_status.h>

namespace yds::ros2 {

/// @brief ROS設備状態値をQt設備状態へ変換する
/// @param state ROS設備状態値
/// @return Qt設備状態
EquipmentState equipmentStateFromRos(std::uint8_t state) noexcept;

/// @brief Qt設備状態をROS設備状態値へ変換する
/// @param state Qt設備状態
/// @return ROS設備状態値
std::uint8_t equipmentStateToRos(EquipmentState state) noexcept;

/// @brief ROS時刻をQt日時へ変換する
/// @param timestamp ROS時刻
/// @return Qt日時。ROS時刻がゼロの場合は無効な日時
QDateTime dateTimeFromRos(const builtin_interfaces::msg::Time& timestamp) noexcept;

/// @brief Qt日時をROS時刻へ変換する
/// @param dateTime Qt日時
/// @return ROS時刻。Qt日時が無効またはROS時刻の範囲外の場合はゼロ時刻
builtin_interfaces::msg::Time dateTimeToRos(const QDateTime& dateTime) noexcept;

/// @brief ROS設備状態メッセージをQt設備状態へ変換する
/// @param topicName メッセージを受信したトピック名
/// @param message ROS設備状態メッセージ
/// @return Qt設備状態
EquipmentStatus equipmentStatusFromRos(
	const QString& topicName,
	const yds_interfaces::msg::EquipmentStatus& message);

/// @brief Qt設備状態をROS設備状態メッセージへ変換する
/// @param status Qt設備状態
/// @return ROS設備状態メッセージ
yds_interfaces::msg::EquipmentStatus equipmentStatusToRos(
	const EquipmentStatus& status);

}  // namespace yds::ros2
