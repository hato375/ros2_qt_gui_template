#include <sample_processor/sample_lifecycle_processor_node.h>

#include <chrono>
#include <stdexcept>
#include <string>

#include <QByteArray>
#include <QString>

#include <rcl_interfaces/msg/integer_range.hpp>
#include <rcl_interfaces/msg/parameter_descriptor.hpp>

#include <yds/ros2/component_status.h>
#include <yds/ros2/component_status_parameters.h>

namespace {

constexpr std::int64_t kMinimumIntervalMs = 100;
constexpr std::int64_t kMaximumIntervalMs = 600000;

rcl_interfaces::msg::ParameterDescriptor processingIntervalDescriptor() {
	rcl_interfaces::msg::ParameterDescriptor descriptor;
	descriptor.description = "Sample processing interval in milliseconds.";
	descriptor.read_only = true;
	rcl_interfaces::msg::IntegerRange range;
	range.from_value = kMinimumIntervalMs;
	range.to_value = kMaximumIntervalMs;
	range.step = 1;
	descriptor.integer_range.push_back(range);
	return descriptor;
}

}  // namespace

namespace sampleprocessor {

SampleLifecycleProcessorNode::SampleLifecycleProcessorNode(
	const rclcpp::NodeOptions& options)
	: rclcpp_lifecycle::LifecycleNode("sample_lifecycle_processor_node", options),
	  processingIntervalMs_(declare_parameter<std::int64_t>(
		  "processing_interval_ms",
		  1000,
		  processingIntervalDescriptor())),
	  processedCount_(0),
	  statusPublisher_(),
	  processingTimer_() {
	if (processingIntervalMs_ < kMinimumIntervalMs ||
		processingIntervalMs_ > kMaximumIntervalMs) {
		throw std::out_of_range(
			"processing_interval_ms must be between 100 and 600000 milliseconds");
	}
	statusPublisher_ = std::make_unique<yds::ros2::ComponentStatusPublisher>(
		*this,
		yds::ros2::declareComponentStatusPublisherParameters(
			*this,
			{
				QStringLiteral("sample-lifecycle-processor-1"),
				QStringLiteral("sample_lifecycle_processor/status"),
				std::chrono::milliseconds(1000)}));
	processingTimer_ = create_wall_timer(
		std::chrono::milliseconds(processingIntervalMs_),
		[this]() {
			process();
		});
	processingTimer_->cancel();
	RCLCPP_INFO(get_logger(), "Lifecycle sample processor node started");
}

std::int64_t SampleLifecycleProcessorNode::processingIntervalMs() const noexcept {
	return processingIntervalMs_;
}

std::uint64_t SampleLifecycleProcessorNode::processedCount() const noexcept {
	return processedCount_.load();
}

SampleLifecycleProcessorNode::CallbackReturn SampleLifecycleProcessorNode::on_configure(
	const rclcpp_lifecycle::State&) {
	RCLCPP_INFO(get_logger(), "Configuring lifecycle sample processor");
	return updateComponentStatus(
		yds::ros2::ComponentState::kReady,
		0,
		QStringLiteral("Configuration completed"))
		? CallbackReturn::SUCCESS
		: CallbackReturn::ERROR;
}

SampleLifecycleProcessorNode::CallbackReturn SampleLifecycleProcessorNode::on_cleanup(
	const rclcpp_lifecycle::State&) {
	processingTimer_->cancel();
	processedCount_.store(0);
	RCLCPP_INFO(get_logger(), "Cleaning up lifecycle sample processor");
	return updateComponentStatus(
		yds::ros2::ComponentState::kInitializing,
		0,
		QStringLiteral("Waiting for configuration"))
		? CallbackReturn::SUCCESS
		: CallbackReturn::ERROR;
}

SampleLifecycleProcessorNode::CallbackReturn SampleLifecycleProcessorNode::on_activate(
	const rclcpp_lifecycle::State&) {
	RCLCPP_INFO(get_logger(), "Activating lifecycle sample processor");
	if (!updateComponentStatus(
			yds::ros2::ComponentState::kRunning,
			0,
			QStringLiteral("Processing started"))) {
		return CallbackReturn::ERROR;
	}
	processingTimer_->reset();
	return CallbackReturn::SUCCESS;
}

SampleLifecycleProcessorNode::CallbackReturn SampleLifecycleProcessorNode::on_deactivate(
	const rclcpp_lifecycle::State&) {
	processingTimer_->cancel();
	RCLCPP_INFO(get_logger(), "Deactivating lifecycle sample processor");
	return updateComponentStatus(
		yds::ros2::ComponentState::kStopped,
		0,
		QStringLiteral("Processing stopped"))
		? CallbackReturn::SUCCESS
		: CallbackReturn::ERROR;
}

SampleLifecycleProcessorNode::CallbackReturn SampleLifecycleProcessorNode::on_shutdown(
	const rclcpp_lifecycle::State&) {
	processingTimer_->cancel();
	RCLCPP_INFO(get_logger(), "Shutting down lifecycle sample processor");
	return updateComponentStatus(
		yds::ros2::ComponentState::kStopped,
		0,
		QStringLiteral("Node shutting down"))
		? CallbackReturn::SUCCESS
		: CallbackReturn::ERROR;
}

SampleLifecycleProcessorNode::CallbackReturn SampleLifecycleProcessorNode::on_error(
	const rclcpp_lifecycle::State&) {
	processingTimer_->cancel();
	RCLCPP_ERROR(get_logger(), "Lifecycle sample processor transition failed");
	return updateComponentStatus(
		yds::ros2::ComponentState::kError,
		9001,
		QStringLiteral("Lifecycle transition failed"))
		? CallbackReturn::SUCCESS
		: CallbackReturn::FAILURE;
}

void SampleLifecycleProcessorNode::process() noexcept {
	++processedCount_;
	statusPublisher_->setStatus(
		yds::ros2::ComponentState::kRunning,
		0,
		QStringLiteral("Processed cycle %1").arg(processedCount_.load()));
}

bool SampleLifecycleProcessorNode::updateComponentStatus(
	yds::ros2::ComponentState state,
	qint32 errorCode,
	const QString& message) noexcept {
	if (!statusPublisher_->setStatus(state, errorCode, message)) {
		RCLCPP_ERROR(get_logger(), "Failed to publish lifecycle component status");
		return false;
	}
	const QByteArray stateTextUtf8 = yds::ros2::componentStateText(state).toUtf8();
	RCLCPP_INFO(
		get_logger(),
		"Lifecycle component state changed: state=%s, error_code=%ld",
		stateTextUtf8.constData(),
		static_cast<long>(errorCode));
	return true;
}

}  // namespace sampleprocessor
