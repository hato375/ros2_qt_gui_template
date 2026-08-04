#pragma once

#include <cstdint>

#include <QDateTime>
#include <QString>

#include <builtin_interfaces/msg/time.hpp>
#include <yds_interfaces/msg/component_status.hpp>

#include <yds/ros2/component_status.h>

namespace yds::ros2 {

/// @brief ROSコンポーネント状態値をQtコンポーネント状態へ変換する
/// @param state ROSコンポーネント状態値
/// @return Qtコンポーネント状態
ComponentState componentStateFromRos(std::uint8_t state) noexcept;

/// @brief Qtコンポーネント状態をROSコンポーネント状態値へ変換する
/// @param state Qtコンポーネント状態
/// @return ROSコンポーネント状態値
std::uint8_t componentStateToRos(ComponentState state) noexcept;

/// @brief ROS時刻をQt日時へ変換する
/// @param timestamp ROS時刻
/// @return Qt日時。ROS時刻がゼロの場合は無効な日時
QDateTime dateTimeFromRos(const builtin_interfaces::msg::Time& timestamp) noexcept;

/// @brief Qt日時をROS時刻へ変換する
/// @param dateTime Qt日時
/// @return ROS時刻。Qt日時が無効またはROS時刻の範囲外の場合はゼロ時刻
builtin_interfaces::msg::Time dateTimeToRos(const QDateTime& dateTime) noexcept;

/// @brief ROSコンポーネント状態メッセージをQtコンポーネント状態へ変換する
/// @param topicName メッセージを受信したトピック名
/// @param message ROSコンポーネント状態メッセージ
/// @return Qtコンポーネント状態
ComponentStatus componentStatusFromRos(
	const QString& topicName,
	const yds_interfaces::msg::ComponentStatus& message);

/// @brief Qtコンポーネント状態をROSコンポーネント状態メッセージへ変換する
/// @param status Qtコンポーネント状態
/// @return ROSコンポーネント状態メッセージ
yds_interfaces::msg::ComponentStatus componentStatusToRos(
	const ComponentStatus& status);

}  // namespace yds::ros2
