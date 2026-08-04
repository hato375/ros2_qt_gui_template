#include <sample_processor/sample_processor_node.h>

#include <chrono>
#include <stdexcept>
#include <string>

#include <QString>

#include <rcl_interfaces/msg/integer_range.hpp>
#include <rcl_interfaces/msg/parameter_descriptor.hpp>

namespace {

constexpr std::int64_t kMinimumIntervalMs = 100;
constexpr std::int64_t kMaximumIntervalMs = 600000;

rcl_interfaces::msg::ParameterDescriptor readOnlyDescriptor(
	const std::string& description) {
	rcl_interfaces::msg::ParameterDescriptor descriptor;
	descriptor.description = description;
	descriptor.read_only = true;
	return descriptor;
}

rcl_interfaces::msg::ParameterDescriptor intervalDescriptor(
	const std::string& description) {
	auto descriptor = readOnlyDescriptor(description);
	rcl_interfaces::msg::IntegerRange range;
	range.from_value = kMinimumIntervalMs;
	range.to_value = kMaximumIntervalMs;
	range.step = 1;
	descriptor.integer_range.push_back(range);
	return descriptor;
}

void validateInterval(const std::string& parameterName, std::int64_t value) {
	if (value < kMinimumIntervalMs || value > kMaximumIntervalMs) {
		throw std::out_of_range(
			parameterName + " must be between 100 and 600000 milliseconds");
	}
}

}  // namespace

namespace sampleprocessor {

SampleProcessorNode::SampleProcessorNode(const rclcpp::NodeOptions& options)
	: rclcpp::Node("sample_processor_node", options),
	  processingIntervalMs_(declare_parameter<std::int64_t>(
		  "processing_interval_ms",
		  1000,
		  intervalDescriptor("Sample processing interval in milliseconds."))),
	  processedCount_(0),
	  ready_(false),
	  statusPublisher_(),
	  processingTimer_() {
	validateInterval("processing_interval_ms", processingIntervalMs_);

	const QString componentId = QString::fromStdString(declare_parameter<std::string>(
		"component_status.component_id",
		"sample-processor-1",
		readOnlyDescriptor("Component ID.")));
	const QString statusTopicName = QString::fromStdString(declare_parameter<std::string>(
		"component_status.status_topic",
		"sample_processor/status",
		readOnlyDescriptor("Component status topic name.")));
	const std::int64_t publishIntervalMs = declare_parameter<std::int64_t>(
		"component_status.publish_interval_ms",
		1000,
		intervalDescriptor("Component status publish interval in milliseconds."));
	validateInterval("component_status.publish_interval_ms", publishIntervalMs);

	statusPublisher_ = std::make_unique<yds::ros2::ComponentStatusPublisher>(
		*this,
		yds::ros2::ComponentStatusPublisherConfiguration{
			componentId,
			statusTopicName,
			std::chrono::milliseconds(publishIntervalMs)});
	processingTimer_ = create_wall_timer(
		std::chrono::milliseconds(processingIntervalMs_),
		[this]() {
			process();
		});

	RCLCPP_INFO(get_logger(), "Sample processor node started");
}

std::int64_t SampleProcessorNode::processingIntervalMs() const noexcept {
	return processingIntervalMs_;
}

std::uint64_t SampleProcessorNode::processedCount() const noexcept {
	return processedCount_.load();
}

void SampleProcessorNode::process() noexcept {
	if (!ready_) {
		ready_ = true;
		if (statusPublisher_->setStatus(
			yds::ros2::ComponentState::kReady,
			0,
			QStringLiteral("Initialization completed"))) {
			RCLCPP_INFO(get_logger(), "Sample processor initialization completed");
		}
		return;
	}

	++processedCount_;
	statusPublisher_->setStatus(
		yds::ros2::ComponentState::kRunning,
		0,
		QStringLiteral("Processed cycle %1").arg(processedCount_.load()));
	RCLCPP_DEBUG(
		get_logger(),
		"Sample processing completed: count=%lu",
		static_cast<unsigned long>(processedCount_.load()));
}

}  // namespace sampleprocessor
