#include "ros_node.h"

#include <chrono>
#include <exception>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

#include <QByteArray>
#include <QDateTime>

#include <rcl_interfaces/msg/integer_range.hpp>
#include <rcl_interfaces/msg/parameter_descriptor.hpp>

#include <yds/ros2/component_status_conversion.h>

namespace ros2qtgui {

namespace {

constexpr std::int64_t kDefaultHeartbeatIntervalMs = 1000;
constexpr std::int64_t kMinimumHeartbeatIntervalMs = 100;
constexpr std::int64_t kMaximumHeartbeatIntervalMs = 60000;
constexpr std::int64_t kDefaultGuiStatusCheckIntervalMs = 200;
constexpr std::int64_t kMinimumGuiStatusCheckIntervalMs = 50;
constexpr std::int64_t kMaximumGuiStatusCheckIntervalMs = 10000;
constexpr char kDefaultCameraMonitorName[] = "camera";
constexpr char kDefaultPlcMonitorName[] = "plc";
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

void validateMonitorName(const std::string& monitorName) {
	static const std::regex validNamePattern("^[A-Za-z_][A-Za-z0-9_]*$");
	if (!std::regex_match(monitorName, validNamePattern)) {
		throw std::invalid_argument(
			"component monitor names must start with a letter or underscore and contain only "
			"letters, numbers, and underscores: " +
			monitorName);
	}
}

std::int64_t defaultTimeoutMs(const std::string& monitorName) noexcept {
	return monitorName == kDefaultPlcMonitorName
		? 5000
		: kDefaultTopicReceptionTimeoutMs;
}

}  // namespace

RosNode::RosNode(
	HeartbeatCallback heartbeatCallback,
	ApplicationEventCallback applicationEventCallback,
	TopicReceptionStatusCallback topicReceptionStatusCallback,
	ComponentStatusCallback componentStatusCallback,
	const rclcpp::NodeOptions& options)
	: Node("ros2_qt_gui_node", options),
	  heartbeatCallback_(std::move(heartbeatCallback)),
	  applicationEventCallback_(std::move(applicationEventCallback)),
	  topicReceptionStatusCallback_(std::move(topicReceptionStatusCallback)),
	  componentStatusCallback_(std::move(componentStatusCallback)),
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
	  componentMonitorNames_(declare_parameter<std::vector<std::string>>(
		  "component_monitor_names",
		  std::vector<std::string>{
			  kDefaultCameraMonitorName,
			  kDefaultPlcMonitorName,
		  },
		  makeReadOnlyDescriptor("Names of component monitor configurations."))),
	  componentMonitorConfigurations_(),
	  topicReceptionMonitors_(),
	  latestComponentStatuses_(),
	  hasComponentStatuses_(),
	  componentStatusDirty_(),
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
	if (componentMonitorNames_.empty()) {
		throw std::invalid_argument("component_monitor_names must not be empty");
	}
	std::set<std::string> uniqueMonitorNames;
	std::set<std::string> uniqueTopicNames;
	for (const auto& monitorName : componentMonitorNames_) {
		validateMonitorName(monitorName);
		if (!uniqueMonitorNames.insert(monitorName).second) {
			throw std::invalid_argument("component_monitor_names must not contain duplicates");
		}

		const std::string parameterPrefix = "component_monitors." + monitorName;
		const bool enabled = declare_parameter<bool>(
			parameterPrefix + ".enabled",
			true,
			makeReadOnlyDescriptor("Whether this component monitor is enabled."));
		const std::string displayName = declare_parameter<std::string>(
			parameterPrefix + ".display_name",
			monitorName,
			makeReadOnlyDescriptor("Component display name."));
		const std::string topicName = declare_parameter<std::string>(
			parameterPrefix + ".status_topic",
			monitorName + "/status",
			makeReadOnlyDescriptor("yds_interfaces/ComponentStatus topic name."));
		const std::int64_t timeoutMs = declare_parameter<std::int64_t>(
			parameterPrefix + ".timeout_ms",
			defaultTimeoutMs(monitorName),
			makeReadOnlyIntegerDescriptor(
				"Topic reception timeout in milliseconds.",
				kMinimumTopicReceptionTimeoutMs,
				kMaximumTopicReceptionTimeoutMs));

		if (displayName.empty()) {
			throw std::invalid_argument(parameterPrefix + ".display_name must not be empty");
		}
		if (topicName.empty()) {
			throw std::invalid_argument(parameterPrefix + ".status_topic must not be empty");
		}
		if (!uniqueTopicNames.insert(topicName).second) {
			throw std::invalid_argument("component monitor status topics must not contain duplicates");
		}
		validateInterval(
			parameterPrefix + ".timeout_ms",
			timeoutMs,
			kMinimumTopicReceptionTimeoutMs,
			kMaximumTopicReceptionTimeoutMs);
		if (enabled) {
			componentMonitorConfigurations_.push_back({
				QString::fromStdString(monitorName),
				QString::fromStdString(displayName),
				QString::fromStdString(topicName),
				timeoutMs});
		}
	}
	if (componentMonitorConfigurations_.empty()) {
		throw std::invalid_argument("at least one component monitor must be enabled");
	}

	heartbeatTimer_ = create_wall_timer(std::chrono::milliseconds(heartbeatIntervalMs_), [this]() {
		onHeartbeat();
	});
	topicReceptionMonitors_.reserve(componentMonitorConfigurations_.size());
	latestComponentStatuses_.reserve(componentMonitorConfigurations_.size());
	hasComponentStatuses_.reserve(componentMonitorConfigurations_.size());
	componentStatusDirty_.reserve(componentMonitorConfigurations_.size());
	monitoredTopicSubscriptions_.reserve(componentMonitorConfigurations_.size());
	for (std::size_t index = 0; index < componentMonitorConfigurations_.size(); ++index) {
		const auto& configuration = componentMonitorConfigurations_[index];
		topicReceptionMonitors_.push_back(
			std::make_unique<yds::ros2::TopicReceptionMonitor>(
				configuration.statusTopicName,
				std::chrono::milliseconds(configuration.timeoutMs)));
		latestComponentStatuses_.push_back({
			configuration.statusTopicName,
			QString(),
			yds::ros2::ComponentState::kUnknown,
			0,
			QString(),
			QDateTime()});
		hasComponentStatuses_.push_back(false);
		componentStatusDirty_.push_back(false);
		monitoredTopicSubscriptions_.push_back(
			create_subscription<yds_interfaces::msg::ComponentStatus>(
			configuration.statusTopicName.toStdString(),
			rclcpp::QoS(10),
			[this, index](const yds_interfaces::msg::ComponentStatus::SharedPtr message) {
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
		"enabled_component_monitor_count=%lu",
		static_cast<long>(heartbeatIntervalMs_),
		static_cast<long>(guiStatusCheckIntervalMs_),
		static_cast<unsigned long>(componentMonitorConfigurations_.size()));
	for (const auto& configuration : componentMonitorConfigurations_) {
		const QByteArray monitorNameUtf8 = configuration.name.toUtf8();
		const QByteArray displayNameUtf8 = configuration.displayName.toUtf8();
		const QByteArray topicNameUtf8 = configuration.statusTopicName.toUtf8();
		RCLCPP_INFO(
			get_logger(),
			"Component monitor enabled: name=%s, display_name=%s, topic=%s, timeout_ms=%ld",
			monitorNameUtf8.constData(),
			displayNameUtf8.constData(),
			topicNameUtf8.constData(),
			static_cast<long>(configuration.timeoutMs));
	}
	RCLCPP_INFO(get_logger(), "ROS 2 node started");
}

std::int64_t RosNode::heartbeatIntervalMs() const noexcept {
	return heartbeatIntervalMs_;
}

std::int64_t RosNode::guiStatusCheckIntervalMs() const noexcept {
	return guiStatusCheckIntervalMs_;
}

const std::vector<ComponentMonitorConfiguration>& RosNode::componentMonitorConfigurations()
	const noexcept {
	return componentMonitorConfigurations_;
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
	const yds_interfaces::msg::ComponentStatus::SharedPtr message) noexcept {
	try {
		auto& monitor = *topicReceptionMonitors_.at(monitorIndex);
		const auto previousStatus = latestComponentStatuses_.at(monitorIndex);
		const bool hasPreviousStatus = hasComponentStatuses_.at(monitorIndex);
		auto status =
			yds::ros2::componentStatusFromRos(monitor.status().topicName, *message);
		if (!status.timestamp.isValid()) {
			status.timestamp = QDateTime::currentDateTime();
		}
		latestComponentStatuses_.at(monitorIndex) = status;
		hasComponentStatuses_.at(monitorIndex) = true;
		componentStatusDirty_.at(monitorIndex) = true;
		handleTopicReceptionTransition(
			monitor.status().topicName,
			monitor.recordReception(status.message));
		handleComponentStateTransition(previousStatus, status, hasPreviousStatus);
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
		notifyComponentStatus(index);
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

void RosNode::handleComponentStateTransition(
	const yds::ros2::ComponentStatus& previousStatus,
	const yds::ros2::ComponentStatus& currentStatus,
	bool hasPreviousStatus) noexcept {
	if (hasPreviousStatus &&
		previousStatus.state == currentStatus.state &&
		previousStatus.errorCode == currentStatus.errorCode) {
		return;
	}

	yds::ros2::ApplicationEventLevel level = yds::ros2::ApplicationEventLevel::kInfo;
	switch (currentStatus.state) {
	case yds::ros2::ComponentState::kWarning:
		level = yds::ros2::ApplicationEventLevel::kWarning;
		break;
	case yds::ros2::ComponentState::kError:
		level = yds::ros2::ApplicationEventLevel::kError;
		break;
	case yds::ros2::ComponentState::kCritical:
		level = yds::ros2::ApplicationEventLevel::kCritical;
		break;
	case yds::ros2::ComponentState::kUnknown:
	case yds::ros2::ComponentState::kInitializing:
	case yds::ros2::ComponentState::kReady:
	case yds::ros2::ComponentState::kRunning:
	case yds::ros2::ComponentState::kStopped:
		break;
	}

	const QString eventMessage =
		QStringLiteral("Component state changed: topic=%1, component_id=%2, state=%3, "
			"error_code=%4, message=%5")
			.arg(
				currentStatus.topicName,
				currentStatus.componentId,
				yds::ros2::componentStateText(currentStatus.state))
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

void RosNode::notifyComponentStatus(std::size_t monitorIndex) noexcept {
	if (!componentStatusCallback_ || !componentStatusDirty_.at(monitorIndex)) {
		return;
	}

	try {
		componentStatusCallback_(latestComponentStatuses_.at(monitorIndex));
		componentStatusDirty_.at(monitorIndex) = false;
	} catch (const std::exception& exception) {
		RCLCPP_ERROR(
			get_logger(),
			"Component status callback failed: %s",
			exception.what());
	} catch (...) {
		RCLCPP_ERROR(
			get_logger(),
			"Component status callback failed with an unknown error");
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
