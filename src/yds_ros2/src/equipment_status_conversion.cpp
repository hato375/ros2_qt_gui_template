#include <yds/ros2/equipment_status_conversion.h>

#include <limits>

namespace yds::ros2 {

EquipmentState equipmentStateFromRos(std::uint8_t state) noexcept {
	switch (state) {
	case yds_interfaces::msg::EquipmentStatus::STATE_INITIALIZING:
		return EquipmentState::kInitializing;
	case yds_interfaces::msg::EquipmentStatus::STATE_READY:
		return EquipmentState::kReady;
	case yds_interfaces::msg::EquipmentStatus::STATE_RUNNING:
		return EquipmentState::kRunning;
	case yds_interfaces::msg::EquipmentStatus::STATE_WARNING:
		return EquipmentState::kWarning;
	case yds_interfaces::msg::EquipmentStatus::STATE_ERROR:
		return EquipmentState::kError;
	case yds_interfaces::msg::EquipmentStatus::STATE_CRITICAL:
		return EquipmentState::kCritical;
	case yds_interfaces::msg::EquipmentStatus::STATE_STOPPED:
		return EquipmentState::kStopped;
	case yds_interfaces::msg::EquipmentStatus::STATE_UNKNOWN:
	default:
		return EquipmentState::kUnknown;
	}
}

std::uint8_t equipmentStateToRos(EquipmentState state) noexcept {
	switch (state) {
	case EquipmentState::kInitializing:
		return yds_interfaces::msg::EquipmentStatus::STATE_INITIALIZING;
	case EquipmentState::kReady:
		return yds_interfaces::msg::EquipmentStatus::STATE_READY;
	case EquipmentState::kRunning:
		return yds_interfaces::msg::EquipmentStatus::STATE_RUNNING;
	case EquipmentState::kWarning:
		return yds_interfaces::msg::EquipmentStatus::STATE_WARNING;
	case EquipmentState::kError:
		return yds_interfaces::msg::EquipmentStatus::STATE_ERROR;
	case EquipmentState::kCritical:
		return yds_interfaces::msg::EquipmentStatus::STATE_CRITICAL;
	case EquipmentState::kStopped:
		return yds_interfaces::msg::EquipmentStatus::STATE_STOPPED;
	case EquipmentState::kUnknown:
	default:
		return yds_interfaces::msg::EquipmentStatus::STATE_UNKNOWN;
	}
}

QDateTime dateTimeFromRos(const builtin_interfaces::msg::Time& timestamp) noexcept {
	if (timestamp.sec == 0 && timestamp.nanosec == 0) {
		return QDateTime();
	}
	const qint64 milliseconds =
		static_cast<qint64>(timestamp.sec) * 1000 +
		static_cast<qint64>(timestamp.nanosec) / 1000000;
	return QDateTime::fromMSecsSinceEpoch(milliseconds, Qt::UTC).toLocalTime();
}

builtin_interfaces::msg::Time dateTimeToRos(const QDateTime& dateTime) noexcept {
	builtin_interfaces::msg::Time timestamp;
	if (!dateTime.isValid()) {
		return timestamp;
	}

	const qint64 milliseconds = dateTime.toMSecsSinceEpoch();
	qint64 seconds = milliseconds / 1000;
	qint64 remainingMilliseconds = milliseconds % 1000;
	if (remainingMilliseconds < 0) {
		--seconds;
		remainingMilliseconds += 1000;
	}
	if (seconds < std::numeric_limits<std::int32_t>::min() ||
		seconds > std::numeric_limits<std::int32_t>::max()) {
		return timestamp;
	}
	timestamp.sec = static_cast<std::int32_t>(seconds);
	timestamp.nanosec =
		static_cast<std::uint32_t>(remainingMilliseconds * 1000000);
	return timestamp;
}

EquipmentStatus equipmentStatusFromRos(
	const QString& topicName,
	const yds_interfaces::msg::EquipmentStatus& message) {
	return {
		topicName,
		QString::fromStdString(message.equipment_id),
		equipmentStateFromRos(message.state),
		static_cast<qint32>(message.error_code),
		QString::fromStdString(message.message),
		dateTimeFromRos(message.header.stamp)};
}

yds_interfaces::msg::EquipmentStatus equipmentStatusToRos(
	const EquipmentStatus& status) {
	yds_interfaces::msg::EquipmentStatus message;
	message.header.stamp = dateTimeToRos(status.timestamp);
	message.equipment_id = status.equipmentId.toStdString();
	message.state = equipmentStateToRos(status.state);
	message.error_code = static_cast<std::int32_t>(status.errorCode);
	message.message = status.message.toStdString();
	return message;
}

}  // namespace yds::ros2
