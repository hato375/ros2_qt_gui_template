#include <yds/ros2/equipment_status_node.h>

#include <cstdint>
#include <stdexcept>

#include <QByteArray>
#include <QDateTime>

#include <rcl_interfaces/msg/parameter_descriptor.hpp>

#include <yds/ros2/equipment_status_conversion.h>

namespace {

constexpr std::int64_t kMinimumPublishIntervalMs = 100;
constexpr std::int64_t kMaximumPublishIntervalMs = 600000;

rcl_interfaces::msg::ParameterDescriptor readOnlyParameterDescriptor(
	const std::string& description) {
	rcl_interfaces::msg::ParameterDescriptor descriptor;
	descriptor.description = description;
	descriptor.read_only = true;
	return descriptor;
}

}  // namespace

namespace yds::ros2 {

EquipmentStatusNode::EquipmentStatusNode(
	const std::string& nodeName,
	const QString& defaultEquipmentId,
	const QString& defaultStatusTopicName,
	std::chrono::milliseconds defaultPublishInterval,
	const rclcpp::NodeOptions& options)
	: rclcpp::Node(nodeName, options),
	  equipmentId_(),
	  statusTopicName_(),
	  statusPublishInterval_(0),
	  equipmentStatus_{},
	  statusPublisher_(),
	  statusTimer_() {
	equipmentId_ = QString::fromStdString(declare_parameter<std::string>(
		"equipment_status.equipment_id",
		defaultEquipmentId.toStdString(),
		readOnlyParameterDescriptor("設備ID")));
	statusTopicName_ = QString::fromStdString(declare_parameter<std::string>(
		"equipment_status.topic_name",
		defaultStatusTopicName.toStdString(),
		readOnlyParameterDescriptor("設備状態の通知トピック名")));
	const std::int64_t publishIntervalMs = declare_parameter<std::int64_t>(
		"equipment_status.publish_interval_ms",
		defaultPublishInterval.count(),
		readOnlyParameterDescriptor("設備状態の定期通知周期（ミリ秒）"));

	if (equipmentId_.trimmed().isEmpty()) {
		throw std::invalid_argument("equipment_status.equipment_id must not be empty");
	}
	if (statusTopicName_.trimmed().isEmpty()) {
		throw std::invalid_argument("equipment_status.topic_name must not be empty");
	}
	if (publishIntervalMs < kMinimumPublishIntervalMs ||
		publishIntervalMs > kMaximumPublishIntervalMs) {
		throw std::out_of_range(
			"equipment_status.publish_interval_ms must be between 100 and 600000");
	}
	statusPublishInterval_ = std::chrono::milliseconds(publishIntervalMs);

	equipmentStatus_ = {
		statusTopicName_,
		equipmentId_,
		EquipmentState::kInitializing,
		0,
		QString(),
		QDateTime()};

	const auto qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
	statusPublisher_ = create_publisher<yds_interfaces::msg::EquipmentStatus>(
		statusTopicName_.toStdString(), qos);
	statusTimer_ = create_wall_timer(statusPublishInterval_, [this]() {
		publishEquipmentStatus();
	});

	const QByteArray equipmentIdUtf8 = equipmentId_.toUtf8();
	const QByteArray topicNameUtf8 = statusTopicName_.toUtf8();
	RCLCPP_INFO(
		get_logger(),
		"Equipment status publisher started: equipment_id=%s, topic=%s, interval_ms=%ld",
		equipmentIdUtf8.constData(),
		topicNameUtf8.constData(),
		static_cast<long>(statusPublishInterval_.count()));

	if (!publishEquipmentStatus()) {
		throw std::runtime_error("Failed to publish initial equipment status");
	}
}

bool EquipmentStatusNode::setEquipmentStatus(
	EquipmentState state,
	qint32 errorCode,
	const QString& message) noexcept {
	try {
		{
			std::lock_guard<std::mutex> lock(statusMutex_);
			equipmentStatus_.state = state;
			equipmentStatus_.errorCode = errorCode;
			equipmentStatus_.message = message;
		}
		return publishEquipmentStatus();
	} catch (const std::exception& exception) {
		RCLCPP_ERROR(get_logger(), "Failed to update equipment status: %s", exception.what());
	} catch (...) {
		RCLCPP_ERROR(get_logger(), "Failed to update equipment status: unknown exception");
	}
	return false;
}

EquipmentStatus EquipmentStatusNode::equipmentStatus() const {
	std::lock_guard<std::mutex> lock(statusMutex_);
	return equipmentStatus_;
}

const QString& EquipmentStatusNode::equipmentId() const noexcept {
	return equipmentId_;
}

const QString& EquipmentStatusNode::statusTopicName() const noexcept {
	return statusTopicName_;
}

std::chrono::milliseconds EquipmentStatusNode::statusPublishInterval() const noexcept {
	return statusPublishInterval_;
}

bool EquipmentStatusNode::publishEquipmentStatus() noexcept {
	try {
		EquipmentStatus status;
		{
			std::lock_guard<std::mutex> lock(statusMutex_);
			equipmentStatus_.timestamp = QDateTime::currentDateTime();
			status = equipmentStatus_;
		}
		statusPublisher_->publish(equipmentStatusToRos(status));
		return true;
	} catch (const std::exception& exception) {
		RCLCPP_ERROR(get_logger(), "Failed to publish equipment status: %s", exception.what());
	} catch (...) {
		RCLCPP_ERROR(get_logger(), "Failed to publish equipment status: unknown exception");
	}
	return false;
}

}  // namespace yds::ros2
