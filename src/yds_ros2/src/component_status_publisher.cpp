#include <yds/ros2/component_status_publisher.h>

#include <cstdint>
#include <stdexcept>

#include <QByteArray>
#include <QDateTime>

#include <rclcpp/create_publisher.hpp>
#include <rclcpp/create_timer.hpp>

#include <yds/ros2/component_status_conversion.h>

namespace {

constexpr std::int64_t kMinimumPublishIntervalMs = 100;
constexpr std::int64_t kMaximumPublishIntervalMs = 600000;

}  // namespace

namespace yds::ros2 {

ComponentStatusPublisher::ComponentStatusPublisher(
	const rclcpp::node_interfaces::NodeBaseInterface::SharedPtr& nodeBase,
	const rclcpp::node_interfaces::NodeTimersInterface::SharedPtr& nodeTimers,
	rclcpp::node_interfaces::NodeParametersInterface::SharedPtr nodeParameters,
	rclcpp::node_interfaces::NodeTopicsInterface::SharedPtr nodeTopics,
	const rclcpp::Logger& logger,
	const ComponentStatusPublisherConfiguration& configuration)
	: logger_(logger),
	  configuration_(configuration),
	  status_{
		  configuration.statusTopicName,
		  configuration.componentId,
		  ComponentState::kInitializing,
		  0,
		  QString(),
		  QDateTime()},
	  publisher_(),
	  timer_() {
	if (configuration_.componentId.trimmed().isEmpty()) {
		throw std::invalid_argument("component status publisher component ID must not be empty");
	}
	if (configuration_.statusTopicName.trimmed().isEmpty()) {
		throw std::invalid_argument("component status publisher topic name must not be empty");
	}
	if (configuration_.publishInterval.count() < kMinimumPublishIntervalMs ||
		configuration_.publishInterval.count() > kMaximumPublishIntervalMs) {
		throw std::out_of_range(
			"component status publisher interval must be between 100 and 600000 milliseconds");
	}

	const auto qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
	publisher_ = rclcpp::create_publisher<yds_interfaces::msg::ComponentStatus>(
		nodeParameters,
		nodeTopics,
		configuration_.statusTopicName.toStdString(),
		qos);
	timer_ = rclcpp::create_wall_timer(
		configuration_.publishInterval,
		[this]() {
			publishStatus();
		},
		nullptr,
		nodeBase.get(),
		nodeTimers.get());

	const QByteArray componentIdUtf8 = configuration_.componentId.toUtf8();
	const QByteArray topicNameUtf8 = configuration_.statusTopicName.toUtf8();
	RCLCPP_INFO(
		logger_,
		"Component status publisher started: component_id=%s, topic=%s, interval_ms=%ld",
		componentIdUtf8.constData(),
		topicNameUtf8.constData(),
		static_cast<long>(configuration_.publishInterval.count()));

	if (!publishStatus()) {
		throw std::runtime_error("Failed to publish initial component status");
	}
}

bool ComponentStatusPublisher::setStatus(
	ComponentState state,
	qint32 errorCode,
	const QString& message) noexcept {
	try {
		{
			std::lock_guard<std::mutex> lock(statusMutex_);
			status_.state = state;
			status_.errorCode = errorCode;
			status_.message = message;
		}
		return publishStatus();
	} catch (const std::exception& exception) {
		RCLCPP_ERROR(logger_, "Failed to update component status: %s", exception.what());
	} catch (...) {
		RCLCPP_ERROR(logger_, "Failed to update component status: unknown exception");
	}
	return false;
}

ComponentStatus ComponentStatusPublisher::status() const {
	std::lock_guard<std::mutex> lock(statusMutex_);
	return status_;
}

const QString& ComponentStatusPublisher::componentId() const noexcept {
	return configuration_.componentId;
}

const QString& ComponentStatusPublisher::statusTopicName() const noexcept {
	return configuration_.statusTopicName;
}

std::chrono::milliseconds ComponentStatusPublisher::publishInterval() const noexcept {
	return configuration_.publishInterval;
}

bool ComponentStatusPublisher::publishStatus() noexcept {
	try {
		ComponentStatus status;
		{
			std::lock_guard<std::mutex> lock(statusMutex_);
			status_.timestamp = QDateTime::currentDateTime();
			status = status_;
		}
		publisher_->publish(componentStatusToRos(status));
		return true;
	} catch (const std::exception& exception) {
		RCLCPP_ERROR(logger_, "Failed to publish component status: %s", exception.what());
	} catch (...) {
		RCLCPP_ERROR(logger_, "Failed to publish component status: unknown exception");
	}
	return false;
}

}  // namespace yds::ros2
