#include "yds/ros2/topic_reception_monitor.h"

#include <stdexcept>

namespace yds::ros2 {

TopicReceptionMonitor::TopicReceptionMonitor(
	const QString& topicName,
	std::chrono::milliseconds timeout)
	: timeout_(timeout),
	  status_({
		  topicName,
		  TopicReceptionState::kWaiting,
		  QDateTime(),
		  0,
		  QString()}),
	  lastReceptionTime_(),
	  statusDirty_(false) {
	if (topicName.isEmpty()) {
		throw std::invalid_argument("topicName must not be empty");
	}
	if (timeout <= std::chrono::milliseconds::zero()) {
		throw std::invalid_argument("timeout must be greater than zero");
	}
}

TopicReceptionTransition TopicReceptionMonitor::recordReception(const QString& message) {
	TopicReceptionTransition transition = TopicReceptionTransition::kNone;
	if (status_.state == TopicReceptionState::kWaiting) {
		transition = TopicReceptionTransition::kStarted;
	} else if (status_.state == TopicReceptionState::kTimedOut) {
		transition = TopicReceptionTransition::kRecovered;
	}

	status_.state = TopicReceptionState::kReceiving;
	status_.lastReceivedAt = QDateTime::currentDateTime();
	++status_.receivedCount;
	status_.lastMessage = message;
	lastReceptionTime_ = std::chrono::steady_clock::now();
	statusDirty_ = true;
	return transition;
}

TopicReceptionTransition TopicReceptionMonitor::checkTimeout() noexcept {
	if (status_.state != TopicReceptionState::kReceiving) {
		return TopicReceptionTransition::kNone;
	}

	const auto elapsed = std::chrono::steady_clock::now() - lastReceptionTime_;
	if (elapsed < timeout_) {
		return TopicReceptionTransition::kNone;
	}

	status_.state = TopicReceptionState::kTimedOut;
	statusDirty_ = true;
	return TopicReceptionTransition::kTimedOut;
}

std::optional<TopicReceptionStatus> TopicReceptionMonitor::takeStatusUpdate() {
	if (!statusDirty_) {
		return std::nullopt;
	}

	statusDirty_ = false;
	return status_;
}

const TopicReceptionStatus& TopicReceptionMonitor::status() const noexcept {
	return status_;
}

}  // namespace yds::ros2
