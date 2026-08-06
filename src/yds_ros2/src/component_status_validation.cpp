#include <yds/ros2/component_status_validation.h>

namespace yds::ros2 {

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
