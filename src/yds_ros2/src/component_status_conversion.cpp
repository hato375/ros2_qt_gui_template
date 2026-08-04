#include <yds/ros2/component_status_conversion.h>

#include <limits>

namespace yds::ros2 {

ComponentState componentStateFromRos(std::uint8_t state) noexcept {
	switch (state) {
	case yds_interfaces::msg::ComponentStatus::STATE_INITIALIZING:
		return ComponentState::kInitializing;
	case yds_interfaces::msg::ComponentStatus::STATE_READY:
		return ComponentState::kReady;
	case yds_interfaces::msg::ComponentStatus::STATE_RUNNING:
		return ComponentState::kRunning;
	case yds_interfaces::msg::ComponentStatus::STATE_WARNING:
		return ComponentState::kWarning;
	case yds_interfaces::msg::ComponentStatus::STATE_ERROR:
		return ComponentState::kError;
	case yds_interfaces::msg::ComponentStatus::STATE_CRITICAL:
		return ComponentState::kCritical;
	case yds_interfaces::msg::ComponentStatus::STATE_STOPPED:
		return ComponentState::kStopped;
	case yds_interfaces::msg::ComponentStatus::STATE_UNKNOWN:
	default:
		return ComponentState::kUnknown;
	}
}

std::uint8_t componentStateToRos(ComponentState state) noexcept {
	switch (state) {
	case ComponentState::kInitializing:
		return yds_interfaces::msg::ComponentStatus::STATE_INITIALIZING;
	case ComponentState::kReady:
		return yds_interfaces::msg::ComponentStatus::STATE_READY;
	case ComponentState::kRunning:
		return yds_interfaces::msg::ComponentStatus::STATE_RUNNING;
	case ComponentState::kWarning:
		return yds_interfaces::msg::ComponentStatus::STATE_WARNING;
	case ComponentState::kError:
		return yds_interfaces::msg::ComponentStatus::STATE_ERROR;
	case ComponentState::kCritical:
		return yds_interfaces::msg::ComponentStatus::STATE_CRITICAL;
	case ComponentState::kStopped:
		return yds_interfaces::msg::ComponentStatus::STATE_STOPPED;
	case ComponentState::kUnknown:
	default:
		return yds_interfaces::msg::ComponentStatus::STATE_UNKNOWN;
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

ComponentStatus componentStatusFromRos(
	const QString& topicName,
	const yds_interfaces::msg::ComponentStatus& message) {
	return {
		topicName,
		QString::fromUtf8(message.component_id.data(), static_cast<int>(message.component_id.size())),
		componentStateFromRos(message.state),
		static_cast<qint32>(message.error_code),
		QString::fromUtf8(message.message.data(), static_cast<int>(message.message.size())),
		dateTimeFromRos(message.header.stamp)};
}

yds_interfaces::msg::ComponentStatus componentStatusToRos(
	const ComponentStatus& status) {
	yds_interfaces::msg::ComponentStatus message;
	message.header.stamp = dateTimeToRos(status.timestamp);
	message.component_id = status.componentId.toUtf8().toStdString();
	message.state = componentStateToRos(status.state);
	message.error_code = static_cast<std::int32_t>(status.errorCode);
	message.message = status.message.toUtf8().toStdString();
	return message;
}

}  // namespace yds::ros2
