#include <sample_processor/sample_lifecycle_processor_node.h>
#include <sample_processor/sample_lifecycle_processor_error_codes.h>

#include <chrono>
#include <stdexcept>
#include <string>

#include <QByteArray>
#include <QString>

#include <rcl_interfaces/msg/integer_range.hpp>
#include <rcl_interfaces/msg/parameter_descriptor.hpp>

#include <yds/ros2/component_status.h>

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
	: LifecycleComponentStatusNode(
		  "sample_lifecycle_processor_node",
		  QStringLiteral("sample-lifecycle-processor-1"),
		  QStringLiteral("sample_lifecycle_processor/status"),
		  std::chrono::milliseconds(1000),
		  options),
	  processingIntervalMs_(declare_parameter<std::int64_t>(
		  "processing_interval_ms",
		  1000,
		  processingIntervalDescriptor())),
	  processedCount_(0),
	  transitionErrorCode_(0),
	  transitionErrorMessage_(),
	  processingTimer_() {
	if (processingIntervalMs_ < kMinimumIntervalMs ||
		processingIntervalMs_ > kMaximumIntervalMs) {
		throw std::out_of_range(
			"processing_interval_ms must be between 100 and 600000 milliseconds");
	}
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

bool SampleLifecycleProcessorNode::configureProcessor(QString&) {
	return true;
}

bool SampleLifecycleProcessorNode::activateProcessor(QString&) {
	return true;
}

bool SampleLifecycleProcessorNode::deactivateProcessor(QString&) {
	return true;
}

bool SampleLifecycleProcessorNode::cleanupProcessor(QString&) {
	return true;
}

bool SampleLifecycleProcessorNode::shutdownProcessor(QString&) {
	return true;
}

bool SampleLifecycleProcessorNode::executeProcessorHook(
	ProcessorHook hook,
	qint32 errorCode,
	const char* operationName,
	const QString& defaultFailureMessage,
	const QString& exceptionMessage) {
	transitionErrorCode_ = 0;
	transitionErrorMessage_.clear();
	QString errorMessage;
	try {
		if ((this->*hook)(errorMessage)) {
			return true;
		}
		transitionErrorCode_ = errorCode;
		transitionErrorMessage_ = errorMessage.trimmed().isEmpty()
			? defaultFailureMessage
			: errorMessage;
		RCLCPP_ERROR(
			get_logger(),
			"Lifecycle sample processor %s failed",
			operationName);
	} catch (const std::exception& exception) {
		transitionErrorCode_ = errorCode;
		transitionErrorMessage_ = exceptionMessage;
		RCLCPP_ERROR(
			get_logger(),
			"Lifecycle sample processor %s raised an exception: %s",
			operationName,
			exception.what());
	} catch (...) {
		transitionErrorCode_ = errorCode;
		transitionErrorMessage_ = exceptionMessage;
		RCLCPP_ERROR(
			get_logger(),
			"Lifecycle sample processor %s raised an unknown exception",
			operationName);
	}
	return false;
}

SampleLifecycleProcessorNode::CallbackReturn SampleLifecycleProcessorNode::on_configure(
	const rclcpp_lifecycle::State&) {
	RCLCPP_INFO(get_logger(), "Configuring lifecycle sample processor");
	if (!executeProcessorHook(
			&SampleLifecycleProcessorNode::configureProcessor,
			lifecycle_error_code::kConfiguration,
			"configuration",
			QStringLiteral("Processor configuration failed"),
			QStringLiteral("Processor configuration raised an exception"))) {
		return CallbackReturn::ERROR;
	}
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
	RCLCPP_INFO(get_logger(), "Cleaning up lifecycle sample processor");
	if (!executeProcessorHook(
			&SampleLifecycleProcessorNode::cleanupProcessor,
			lifecycle_error_code::kCleanup,
			"cleanup",
			QStringLiteral("Processor cleanup failed"),
			QStringLiteral("Processor cleanup raised an exception"))) {
		return CallbackReturn::ERROR;
	}
	processedCount_.store(0);
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
	if (!executeProcessorHook(
			&SampleLifecycleProcessorNode::activateProcessor,
			lifecycle_error_code::kActivation,
			"activation",
			QStringLiteral("Processor activation failed"),
			QStringLiteral("Processor activation raised an exception"))) {
		return CallbackReturn::ERROR;
	}
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
	if (!executeProcessorHook(
			&SampleLifecycleProcessorNode::deactivateProcessor,
			lifecycle_error_code::kDeactivation,
			"deactivation",
			QStringLiteral("Processor deactivation failed"),
			QStringLiteral("Processor deactivation raised an exception"))) {
		return CallbackReturn::ERROR;
	}
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
	if (!executeProcessorHook(
			&SampleLifecycleProcessorNode::shutdownProcessor,
			lifecycle_error_code::kShutdown,
			"shutdown",
			QStringLiteral("Processor shutdown failed"),
			QStringLiteral("Processor shutdown raised an exception"))) {
		return CallbackReturn::ERROR;
	}
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
	const qint32 errorCode = transitionErrorCode_ == 0
		? lifecycle_error_code::kTransition
		: transitionErrorCode_;
	const QString errorMessage = transitionErrorMessage_.isEmpty()
		? QStringLiteral("Lifecycle transition failed")
		: transitionErrorMessage_;
	transitionErrorCode_ = 0;
	transitionErrorMessage_.clear();
	return updateComponentStatus(
		yds::ros2::ComponentState::kError,
		errorCode,
		errorMessage)
		? CallbackReturn::SUCCESS
		: CallbackReturn::FAILURE;
}

void SampleLifecycleProcessorNode::process() noexcept {
	++processedCount_;
	setComponentStatus(
		yds::ros2::ComponentState::kRunning,
		0,
		QStringLiteral("Processed cycle %1").arg(processedCount_.load()));
}

bool SampleLifecycleProcessorNode::updateComponentStatus(
	yds::ros2::ComponentState state,
	qint32 errorCode,
	const QString& message) noexcept {
	if (!setComponentStatus(state, errorCode, message)) {
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
