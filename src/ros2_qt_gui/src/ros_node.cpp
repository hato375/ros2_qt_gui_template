#include "ros_node.h"

#include <chrono>
#include <exception>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

#include <QByteArray>
#include <QDateTime>

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
constexpr char kDefaultCameraStatusTopic[] = "camera/status";
constexpr char kDefaultPlcStatusTopic[] = "plc/status";
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

yds::ros2::EquipmentState equipmentStateFromRos(std::uint8_t state) noexcept {
	switch (state) {
	case yds_interfaces::msg::EquipmentStatus::STATE_INITIALIZING:
		return yds::ros2::EquipmentState::kInitializing;
	case yds_interfaces::msg::EquipmentStatus::STATE_READY:
		return yds::ros2::EquipmentState::kReady;
	case yds_interfaces::msg::EquipmentStatus::STATE_RUNNING:
		return yds::ros2::EquipmentState::kRunning;
	case yds_interfaces::msg::EquipmentStatus::STATE_WARNING:
		return yds::ros2::EquipmentState::kWarning;
	case yds_interfaces::msg::EquipmentStatus::STATE_ERROR:
		return yds::ros2::EquipmentState::kError;
	case yds_interfaces::msg::EquipmentStatus::STATE_CRITICAL:
		return yds::ros2::EquipmentState::kCritical;
	case yds_interfaces::msg::EquipmentStatus::STATE_STOPPED:
		return yds::ros2::EquipmentState::kStopped;
	case yds_interfaces::msg::EquipmentStatus::STATE_UNKNOWN:
	default:
		return yds::ros2::EquipmentState::kUnknown;
	}
}

QDateTime timestampFromRos(const builtin_interfaces::msg::Time& timestamp) noexcept {
	if (timestamp.sec == 0 && timestamp.nanosec == 0) {
		return QDateTime::currentDateTime();
	}
	const qint64 milliseconds =
		static_cast<qint64>(timestamp.sec) * 1000 +
		static_cast<qint64>(timestamp.nanosec) / 1000000;
	return QDateTime::fromMSecsSinceEpoch(milliseconds, Qt::UTC).toLocalTime();
}

}  // namespace

RosNode::RosNode(
	HeartbeatCallback heartbeatCallback,
	ApplicationEventCallback applicationEventCallback,
	TopicReceptionStatusCallback topicReceptionStatusCallback,
	EquipmentStatusCallback equipmentStatusCallback,
	const rclcpp::NodeOptions& options)
	: Node("ros2_qt_gui_node", options),
	  heartbeatCallback_(std::move(heartbeatCallback)),
	  applicationEventCallback_(std::move(applicationEventCallback)),
	  topicReceptionStatusCallback_(std::move(topicReceptionStatusCallback)),
	  equipmentStatusCallback_(std::move(equipmentStatusCallback)),
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
	  monitoredTopics_(declare_parameter<std::vector<std::string>>(
		  "monitored_topics",
		  std::vector<std::string>{
			  kDefaultCameraStatusTopic,
			  kDefaultPlcStatusTopic,
		  },
		  makeReadOnlyDescriptor("Names of the yds_interfaces/EquipmentStatus topics to monitor."))),
	  topicReceptionTimeoutMs_(declare_parameter<std::int64_t>(
		  "topic_reception_timeout_ms",
		  kDefaultTopicReceptionTimeoutMs,
		  makeReadOnlyIntegerDescriptor(
			  "Topic reception timeout in milliseconds.",
			  kMinimumTopicReceptionTimeoutMs,
			  kMaximumTopicReceptionTimeoutMs))),
	  topicReceptionMonitors_(),
	  latestEquipmentStatuses_(),
	  hasEquipmentStatuses_(),
	  equipmentStatusDirty_(),
	  heartbeatCount_(0),
	  heartbeatTimer_(),
	  monitoredTopicSubscriptions_(),
	  topicReceptionStatusTimer_() {
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
	if (monitoredTopics_.empty()) {
		throw std::invalid_argument("monitored_topics must not be empty");
	}
	std::set<std::string> uniqueTopics;
	for (const auto& topic : monitoredTopics_) {
		if (topic.empty()) {
			throw std::invalid_argument("monitored_topics must not contain an empty topic");
		}
		if (!uniqueTopics.insert(topic).second) {
			throw std::invalid_argument("monitored_topics must not contain duplicates");
		}
	}
	validateInterval(
		"topic_reception_timeout_ms",
		topicReceptionTimeoutMs_,
		kMinimumTopicReceptionTimeoutMs,
		kMaximumTopicReceptionTimeoutMs);

	heartbeatTimer_ = create_wall_timer(std::chrono::milliseconds(heartbeatIntervalMs_), [this]() {
		onHeartbeat();
	});
	topicReceptionMonitors_.reserve(monitoredTopics_.size());
	latestEquipmentStatuses_.reserve(monitoredTopics_.size());
	hasEquipmentStatuses_.reserve(monitoredTopics_.size());
	equipmentStatusDirty_.reserve(monitoredTopics_.size());
	monitoredTopicSubscriptions_.reserve(monitoredTopics_.size());
	for (std::size_t index = 0; index < monitoredTopics_.size(); ++index) {
		const auto& topic = monitoredTopics_[index];
		topicReceptionMonitors_.push_back(
			std::make_unique<yds::ros2::TopicReceptionMonitor>(
				QString::fromStdString(topic),
				std::chrono::milliseconds(topicReceptionTimeoutMs_)));
		latestEquipmentStatuses_.push_back({
			QString::fromStdString(topic),
			QString(),
			yds::ros2::EquipmentState::kUnknown,
			0,
			QString(),
			QDateTime()});
		hasEquipmentStatuses_.push_back(false);
		equipmentStatusDirty_.push_back(false);
		monitoredTopicSubscriptions_.push_back(
			create_subscription<yds_interfaces::msg::EquipmentStatus>(
			topic,
			rclcpp::QoS(10),
			[this, index](const yds_interfaces::msg::EquipmentStatus::SharedPtr message) {
				onMonitoredTopic(index, message);
			}));
	}
	topicReceptionStatusTimer_ = create_wall_timer(
		std::chrono::milliseconds(kTopicReceptionStatusUpdateIntervalMs),
		[this]() {
			updateTopicReceptionStatus();
		});
	RCLCPP_INFO(
		get_logger(),
		"Configuration: heartbeat_interval_ms=%ld, gui_status_check_interval_ms=%ld, "
		"monitored_topic_count=%lu, topic_reception_timeout_ms=%ld",
		static_cast<long>(heartbeatIntervalMs_),
		static_cast<long>(guiStatusCheckIntervalMs_),
		static_cast<unsigned long>(monitoredTopics_.size()),
		static_cast<long>(topicReceptionTimeoutMs_));
	for (const auto& topic : monitoredTopics_) {
		RCLCPP_INFO(get_logger(), "Monitoring topic: %s", topic.c_str());
	}
	RCLCPP_INFO(get_logger(), "ROS 2 node started");
}

std::int64_t RosNode::heartbeatIntervalMs() const noexcept {
	return heartbeatIntervalMs_;
}

std::int64_t RosNode::guiStatusCheckIntervalMs() const noexcept {
	return guiStatusCheckIntervalMs_;
}

const std::vector<std::string>& RosNode::monitoredTopics() const noexcept {
	return monitoredTopics_;
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

void RosNode::onMonitoredTopic(
	std::size_t monitorIndex,
	const yds_interfaces::msg::EquipmentStatus::SharedPtr message) noexcept {
	try {
		auto& monitor = *topicReceptionMonitors_.at(monitorIndex);
		const auto previousStatus = latestEquipmentStatuses_.at(monitorIndex);
		const bool hasPreviousStatus = hasEquipmentStatuses_.at(monitorIndex);
		yds::ros2::EquipmentStatus status{
			monitor.status().topicName,
			QString::fromStdString(message->equipment_id),
			equipmentStateFromRos(message->state),
			static_cast<qint32>(message->error_code),
			QString::fromStdString(message->message),
			timestampFromRos(message->header.stamp)};
		latestEquipmentStatuses_.at(monitorIndex) = status;
		hasEquipmentStatuses_.at(monitorIndex) = true;
		equipmentStatusDirty_.at(monitorIndex) = true;
		handleTopicReceptionTransition(
			monitor.status().topicName,
			monitor.recordReception(status.message));
		handleEquipmentStateTransition(previousStatus, status, hasPreviousStatus);
	} catch (const std::exception& exception) {
		RCLCPP_ERROR(
			get_logger(),
			"Failed to record topic reception: %s",
			exception.what());
		reportApplicationEvent(
			yds::ros2::ApplicationEventLevel::kError,
			QStringLiteral("Failed to record topic reception: %1")
				.arg(exception.what()));
	} catch (...) {
		RCLCPP_ERROR(get_logger(), "Failed to record topic reception with an unknown error");
		reportApplicationEvent(
			yds::ros2::ApplicationEventLevel::kError,
			QStringLiteral("Failed to record topic reception with an unknown error"));
	}
}

void RosNode::updateTopicReceptionStatus() noexcept {
	for (std::size_t index = 0; index < topicReceptionMonitors_.size(); ++index) {
		auto& monitor = topicReceptionMonitors_[index];
		handleTopicReceptionTransition(
			monitor->status().topicName,
			monitor->checkTimeout());
		notifyTopicReceptionStatus(*monitor);
		notifyEquipmentStatus(index);
	}
}

void RosNode::notifyTopicReceptionStatus(
	yds::ros2::TopicReceptionMonitor& monitor) noexcept {
	if (!topicReceptionStatusCallback_) {
		return;
	}

	try {
		auto status = monitor.takeStatusUpdate();
		if (!status) {
			return;
		}
		topicReceptionStatusCallback_(*status);
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

void RosNode::handleEquipmentStateTransition(
	const yds::ros2::EquipmentStatus& previousStatus,
	const yds::ros2::EquipmentStatus& currentStatus,
	bool hasPreviousStatus) noexcept {
	if (hasPreviousStatus &&
		previousStatus.state == currentStatus.state &&
		previousStatus.errorCode == currentStatus.errorCode) {
		return;
	}

	yds::ros2::ApplicationEventLevel level = yds::ros2::ApplicationEventLevel::kInfo;
	switch (currentStatus.state) {
	case yds::ros2::EquipmentState::kWarning:
		level = yds::ros2::ApplicationEventLevel::kWarning;
		break;
	case yds::ros2::EquipmentState::kError:
		level = yds::ros2::ApplicationEventLevel::kError;
		break;
	case yds::ros2::EquipmentState::kCritical:
		level = yds::ros2::ApplicationEventLevel::kCritical;
		break;
	case yds::ros2::EquipmentState::kUnknown:
	case yds::ros2::EquipmentState::kInitializing:
	case yds::ros2::EquipmentState::kReady:
	case yds::ros2::EquipmentState::kRunning:
	case yds::ros2::EquipmentState::kStopped:
		break;
	}

	const QString eventMessage =
		QStringLiteral("Equipment state changed: topic=%1, equipment_id=%2, state=%3, "
			"error_code=%4, message=%5")
			.arg(
				currentStatus.topicName,
				currentStatus.equipmentId,
				yds::ros2::equipmentStateText(currentStatus.state))
			.arg(currentStatus.errorCode)
			.arg(currentStatus.message);
	const QByteArray eventMessageUtf8 = eventMessage.toUtf8();
	switch (level) {
	case yds::ros2::ApplicationEventLevel::kInfo:
		RCLCPP_INFO(get_logger(), "%s", eventMessageUtf8.constData());
		break;
	case yds::ros2::ApplicationEventLevel::kWarning:
		RCLCPP_WARN(get_logger(), "%s", eventMessageUtf8.constData());
		break;
	case yds::ros2::ApplicationEventLevel::kError:
		RCLCPP_ERROR(get_logger(), "%s", eventMessageUtf8.constData());
		break;
	case yds::ros2::ApplicationEventLevel::kCritical:
		RCLCPP_FATAL(get_logger(), "%s", eventMessageUtf8.constData());
		break;
	}
	reportApplicationEvent(level, eventMessage);
}

void RosNode::notifyEquipmentStatus(std::size_t monitorIndex) noexcept {
	if (!equipmentStatusCallback_ || !equipmentStatusDirty_.at(monitorIndex)) {
		return;
	}

	try {
		equipmentStatusCallback_(latestEquipmentStatuses_.at(monitorIndex));
		equipmentStatusDirty_.at(monitorIndex) = false;
	} catch (const std::exception& exception) {
		RCLCPP_ERROR(
			get_logger(),
			"Equipment status callback failed: %s",
			exception.what());
	} catch (...) {
		RCLCPP_ERROR(
			get_logger(),
			"Equipment status callback failed with an unknown error");
	}
}

void RosNode::handleTopicReceptionTransition(
	const QString& topicName,
	yds::ros2::TopicReceptionTransition transition) noexcept {
	const QByteArray topicNameUtf8 = topicName.toUtf8();
	switch (transition) {
	case yds::ros2::TopicReceptionTransition::kNone:
		return;
	case yds::ros2::TopicReceptionTransition::kStarted:
		RCLCPP_INFO(
			get_logger(),
			"Topic reception started: %s",
			topicNameUtf8.constData());
		reportApplicationEvent(
			yds::ros2::ApplicationEventLevel::kInfo,
			QStringLiteral("Topic reception started: %1")
				.arg(topicName));
		return;
	case yds::ros2::TopicReceptionTransition::kTimedOut:
		RCLCPP_WARN(
			get_logger(),
			"Topic reception timed out: %s",
			topicNameUtf8.constData());
		reportApplicationEvent(
			yds::ros2::ApplicationEventLevel::kWarning,
			QStringLiteral("Topic reception timed out: %1")
				.arg(topicName));
		return;
	case yds::ros2::TopicReceptionTransition::kRecovered:
		RCLCPP_INFO(
			get_logger(),
			"Topic reception recovered: %s",
			topicNameUtf8.constData());
		reportApplicationEvent(
			yds::ros2::ApplicationEventLevel::kInfo,
			QStringLiteral("Topic reception recovered: %1")
				.arg(topicName));
		return;
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
