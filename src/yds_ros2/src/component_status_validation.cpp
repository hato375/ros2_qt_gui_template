#include <yds/ros2/component_status_validation.h>

#include <yds_interfaces/msg/component_status.hpp>

namespace yds::ros2 {

bool isDefinedComponentState(std::uint8_t state) noexcept {
	switch (state) {
	case yds_interfaces::msg::ComponentStatus::STATE_UNKNOWN:
	case yds_interfaces::msg::ComponentStatus::STATE_INITIALIZING:
	case yds_interfaces::msg::ComponentStatus::STATE_READY:
	case yds_interfaces::msg::ComponentStatus::STATE_RUNNING:
	case yds_interfaces::msg::ComponentStatus::STATE_WARNING:
	case yds_interfaces::msg::ComponentStatus::STATE_ERROR:
	case yds_interfaces::msg::ComponentStatus::STATE_CRITICAL:
	case yds_interfaces::msg::ComponentStatus::STATE_STOPPED:
		return true;
	default:
		return false;
	}
}

bool ComponentStatusValidationResult::hasConsistencyWarning() const noexcept {
	return unexpectedErrorCode || missingErrorCode || missingMessage;
}

ComponentStatusValidationResult validateComponentStatus(
	ComponentState state,
	qint32 errorCode,
	const QString& message) {
	ComponentStatusValidationResult result{true, false, false, false};
	switch (state) {
	case ComponentState::kInitializing:
	case ComponentState::kReady:
	case ComponentState::kRunning:
	case ComponentState::kStopped:
		result.unexpectedErrorCode = errorCode != 0;
		break;
	case ComponentState::kWarning:
	case ComponentState::kError:
	case ComponentState::kCritical:
		result.missingErrorCode = errorCode == 0;
		result.missingMessage = message.trimmed().isEmpty();
		break;
	case ComponentState::kUnknown:
		break;
	default:
		result.validState = false;
		break;
	}
	return result;
}

}  // namespace yds::ros2
