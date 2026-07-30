#include "ros_node.h"

#include <chrono>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

#include <rcl_interfaces/msg/integer_range.hpp>
#include <rcl_interfaces/msg/parameter_descriptor.hpp>

namespace ros2qtgui {

namespace {

constexpr std::int64_t kDefaultHeartbeatIntervalMs = 1000;
constexpr std::int64_t kMinimumHeartbeatIntervalMs = 100;
constexpr std::int64_t kMaximumHeartbeatIntervalMs = 60000;
constexpr std::int64_t kDefaultGuiStatusCheckIntervalMs = 200;
constexpr std::int64_t kMinimumGuiStatusCheckIntervalMs = 50;
constexpr std::int64_t kMaximumGuiStatusCheckIntervalMs = 10000;
constexpr char kDefaultMonitoredTopic[] = "system_status";
constexpr std::int64_t kDefaultTopicReceptionTimeoutMs = 3000;
constexpr std::int64_t kMinimumTopicReceptionTimeoutMs = 500;
constexpr std::int64_t kMaximumTopicReceptionTimeoutMs = 600000;
constexpr std::int64_t kTopicReceptionStatusUpdateIntervalMs = 200;

rcl_interfaces::msg::ParameterDescriptor makeReadOnlyDescriptor(
	const std::string& description) {
	rcl_interfaces::msg::ParameterDescriptor descriptor;
	descriptor.description = description;
	descriptor.read_only = true;
	return descriptor;
}

rcl_interfaces::msg::ParameterDescriptor makeReadOnlyIntegerDescriptor(
	const std::string& description,
	std::int64_t minimum,
	std::int64_t maximum) {
	auto descriptor = makeReadOnlyDescriptor(description);

	rcl_interfaces::msg::IntegerRange range;
	range.from_value = minimum;
	range.to_value = maximum;
	range.step = 1;
	descriptor.integer_range.push_back(range);
	return descriptor;
}

void validateInterval(
	const std::string& parameterName,
	std::int64_t value,
	std::int64_t minimum,
	std::int64_t maximum) {
	if (value < minimum || value > maximum) {
		throw std::invalid_argument(
			parameterName + " must be between " + std::to_string(minimum) + " and " +
			std::to_string(maximum) + " milliseconds");
	}
}

}  // namespace

RosNode::RosNode(
	HeartbeatCallback heartbeatCallback,
	ApplicationEventCallback applicationEventCallback,
	TopicReceptionStatusCallback topicReceptionStatusCallback,
	const rclcpp::NodeOptions& options)
	: Node("ros2_qt_gui_node", options),
	  heartbeatCallback_(std::move(heartbeatCallback)),
	  applicationEventCallback_(std::move(applicationEventCallback)),
	  topicReceptionStatusCallback_(std::move(topicReceptionStatusCallback)),
	  heartbeatIntervalMs_(declare_parameter<std::int64_t>(
		  "heartbeat_interval_ms",
		  kDefaultHeartbeatIntervalMs,
		  makeReadOnlyIntegerDescriptor(
			  "ROS 2 heartbeat interval in milliseconds.",
			  kMinimumHeartbeatIntervalMs,
			  kMaximumHeartbeatIntervalMs))),
	  guiStatusCheckIntervalMs_(declare_parameter<std::int64_t>(
		  "gui_status_check_interval_ms",
		  kDefaultGuiStatusCheckIntervalMs,
		  makeReadOnlyIntegerDescriptor(
			  "GUI status check interval in milliseconds.",
			  kMinimumGuiStatusCheckIntervalMs,
			  kMaximumGuiStatusCheckIntervalMs))),
	  monitoredTopic_(declare_parameter<std::string>(
		  "monitored_topic",
		  kDefaultMonitoredTopic,
		  makeReadOnlyDescriptor("Name of the std_msgs/String topic to monitor."))),
	  topicReceptionTimeoutMs_(declare_parameter<std::int64_t>(
		  "topic_reception_timeout_ms",
		  kDefaultTopicReceptionTimeoutMs,
		  makeReadOnlyIntegerDescriptor(
			  "Topic reception timeout in milliseconds.",
			  kMinimumTopicReceptionTimeoutMs,
			  kMaximumTopicReceptionTimeoutMs))),
	  heartbeatCount_(0),
	  topicReceptionStatus_({
		  QString::fromStdString(monitoredTopic_),
		  yds::ros2::TopicReceptionState::kWaiting,
		  QDateTime(),
		  0,
		  QString()}),
	  lastTopicReceptionTime_(),
	  topicReceptionStatusDirty_(false) {
	validateInterval(
		"heartbeat_interval_ms",
		heartbeatIntervalMs_,
		kMinimumHeartbeatIntervalMs,
		kMaximumHeartbeatIntervalMs);
	validateInterval(
		"gui_status_check_interval_ms",
		guiStatusCheckIntervalMs_,
		kMinimumGuiStatusCheckIntervalMs,
		kMaximumGuiStatusCheckIntervalMs);
	if (monitoredTopic_.empty()) {
		throw std::invalid_argument("monitored_topic must not be empty");
	}
	validateInterval(
		"topic_reception_timeout_ms",
		topicReceptionTimeoutMs_,
		kMinimumTopicReceptionTimeoutMs,
		kMaximumTopicReceptionTimeoutMs);

	heartbeatTimer_ = create_wall_timer(std::chrono::milliseconds(heartbeatIntervalMs_), [this]() {
		onHeartbeat();
	});
	monitoredTopicSubscription_ = create_subscription<std_msgs::msg::String>(
		monitoredTopic_,
		rclcpp::QoS(10),
		[this](const std_msgs::msg::String::SharedPtr message) {
			onMonitoredTopic(message);
		});
	topicReceptionStatusTimer_ = create_wall_timer(
		std::chrono::milliseconds(kTopicReceptionStatusUpdateIntervalMs),
		[this]() {
			updateTopicReceptionStatus();
		});
	RCLCPP_INFO(
		get_logger(),
		"Configuration: heartbeat_interval_ms=%ld, gui_status_check_interval_ms=%ld, "
		"monitored_topic=%s, topic_reception_timeout_ms=%ld",
		static_cast<long>(heartbeatIntervalMs_),
		static_cast<long>(guiStatusCheckIntervalMs_),
		monitoredTopic_.c_str(),
		static_cast<long>(topicReceptionTimeoutMs_));
	RCLCPP_INFO(get_logger(), "ROS 2 node started");
}

std::int64_t RosNode::heartbeatIntervalMs() const noexcept {
	return heartbeatIntervalMs_;
}

std::int64_t RosNode::guiStatusCheckIntervalMs() const noexcept {
	return guiStatusCheckIntervalMs_;
}

const std::string& RosNode::monitoredTopic() const noexcept {
	return monitoredTopic_;
}

std::int64_t RosNode::topicReceptionTimeoutMs() const noexcept {
	return topicReceptionTimeoutMs_;
}

void RosNode::onHeartbeat() noexcept {
	++heartbeatCount_;
	RCLCPP_DEBUG(get_logger(), "Heartbeat: %lu", static_cast<unsigned long>(heartbeatCount_));

	try {
		heartbeatCallback_(heartbeatCount_);
	} catch (const std::exception& exception) {
		RCLCPP_ERROR(get_logger(), "Heartbeat callback failed: %s", exception.what());
		reportApplicationEvent(
			yds::ros2::ApplicationEventLevel::kError,
			QStringLiteral("Heartbeat callback failed: %1").arg(exception.what()));
	} catch (...) {
		RCLCPP_ERROR(get_logger(), "Heartbeat callback failed with an unknown error");
		reportApplicationEvent(
			yds::ros2::ApplicationEventLevel::kError,
			QStringLiteral("Heartbeat callback failed with an unknown error"));
	}
}

void RosNode::onMonitoredTopic(const std_msgs::msg::String::SharedPtr message) noexcept {
	const bool isFirstReception =
		topicReceptionStatus_.state == yds::ros2::TopicReceptionState::kWaiting;
	const bool isRecovery =
		topicReceptionStatus_.state == yds::ros2::TopicReceptionState::kTimedOut;

	topicReceptionStatus_.state = yds::ros2::TopicReceptionState::kReceiving;
	topicReceptionStatus_.lastReceivedAt = QDateTime::currentDateTime();
	++topicReceptionStatus_.receivedCount;
	topicReceptionStatus_.lastMessage = QString::fromStdString(message->data);
	lastTopicReceptionTime_ = std::chrono::steady_clock::now();
	topicReceptionStatusDirty_ = true;

	if (isFirstReception) {
		RCLCPP_INFO(
			get_logger(),
			"Topic reception started: %s",
			monitoredTopic_.c_str());
		reportApplicationEvent(
			yds::ros2::ApplicationEventLevel::kInfo,
			QStringLiteral("Topic reception started: %1")
				.arg(QString::fromStdString(monitoredTopic_)));
	} else if (isRecovery) {
		RCLCPP_INFO(
			get_logger(),
			"Topic reception recovered: %s",
			monitoredTopic_.c_str());
		reportApplicationEvent(
			yds::ros2::ApplicationEventLevel::kInfo,
			QStringLiteral("Topic reception recovered: %1")
				.arg(QString::fromStdString(monitoredTopic_)));
	}
}

void RosNode::updateTopicReceptionStatus() noexcept {
	if (topicReceptionStatus_.state == yds::ros2::TopicReceptionState::kReceiving) {
		const auto elapsed = std::chrono::steady_clock::now() - lastTopicReceptionTime_;
		if (elapsed >= std::chrono::milliseconds(topicReceptionTimeoutMs_)) {
			topicReceptionStatus_.state = yds::ros2::TopicReceptionState::kTimedOut;
			topicReceptionStatusDirty_ = true;
			RCLCPP_WARN(
				get_logger(),
				"Topic reception timed out: %s",
				monitoredTopic_.c_str());
			reportApplicationEvent(
				yds::ros2::ApplicationEventLevel::kWarning,
				QStringLiteral("Topic reception timed out: %1")
					.arg(QString::fromStdString(monitoredTopic_)));
		}
	}

	if (topicReceptionStatusDirty_) {
		notifyTopicReceptionStatus();
	}
}

void RosNode::notifyTopicReceptionStatus() noexcept {
	topicReceptionStatusDirty_ = false;
	if (!topicReceptionStatusCallback_) {
		return;
	}

	try {
		topicReceptionStatusCallback_(topicReceptionStatus_);
	} catch (const std::exception& exception) {
		RCLCPP_ERROR(
			get_logger(),
			"Topic reception status callback failed: %s",
			exception.what());
	} catch (...) {
		RCLCPP_ERROR(
			get_logger(),
			"Topic reception status callback failed with an unknown error");
	}
}

void RosNode::reportApplicationEvent(
	yds::ros2::ApplicationEventLevel level,
	const QString& message) noexcept {
	if (!applicationEventCallback_) {
		return;
	}

	try {
		applicationEventCallback_({level, QDateTime::currentDateTime(), message});
	} catch (const std::exception& exception) {
		RCLCPP_ERROR(
			get_logger(),
			"Application event callback failed: %s",
			exception.what());
	} catch (...) {
		RCLCPP_ERROR(get_logger(), "Application event callback failed with an unknown error");
	}
}

}  // namespace ros2qtgui
