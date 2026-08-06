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
constexpr qint32 kLifecycleTransitionErrorCode = 9001;
constexpr qint32 kProcessorConfigurationErrorCode = 9101;
constexpr qint32 kProcessorActivationErrorCode = 9102;
constexpr qint32 kProcessorDeactivationErrorCode = 9103;
constexpr qint32 kProcessorCleanupErrorCode = 9104;
constexpr qint32 kProcessorShutdownErrorCode = 9105;

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
	  transitionErrorCode_(0),
	  transitionErrorMessage_(),
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

yds::ros2::ComponentStatus SampleLifecycleProcessorNode::componentStatus() const {
	return statusPublisher_->status();
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

SampleLifecycleProcessorNode::CallbackReturn SampleLifecycleProcessorNode::on_configure(
	const rclcpp_lifecycle::State&) {
	RCLCPP_INFO(get_logger(), "Configuring lifecycle sample processor");
	transitionErrorCode_ = 0;
	transitionErrorMessage_.clear();
	QString errorMessage;
	try {
		if (!configureProcessor(errorMessage)) {
			transitionErrorCode_ = kProcessorConfigurationErrorCode;
			transitionErrorMessage_ = errorMessage.trimmed().isEmpty()
				? QStringLiteral("Processor configuration failed")
				: errorMessage;
			RCLCPP_ERROR(get_logger(), "Lifecycle sample processor configuration failed");
			return CallbackReturn::ERROR;
		}
	} catch (const std::exception& exception) {
		transitionErrorCode_ = kProcessorConfigurationErrorCode;
		transitionErrorMessage_ = QStringLiteral("Processor configuration raised an exception");
		RCLCPP_ERROR(
			get_logger(),
			"Lifecycle sample processor configuration raised an exception: %s",
			exception.what());
		return CallbackReturn::ERROR;
	} catch (...) {
		transitionErrorCode_ = kProcessorConfigurationErrorCode;
		transitionErrorMessage_ = QStringLiteral("Processor configuration raised an exception");
		RCLCPP_ERROR(
			get_logger(),
			"Lifecycle sample processor configuration raised an unknown exception");
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
	transitionErrorCode_ = 0;
	transitionErrorMessage_.clear();
	QString errorMessage;
	try {
		if (!cleanupProcessor(errorMessage)) {
			transitionErrorCode_ = kProcessorCleanupErrorCode;
			transitionErrorMessage_ = errorMessage.trimmed().isEmpty()
				? QStringLiteral("Processor cleanup failed")
				: errorMessage;
			RCLCPP_ERROR(get_logger(), "Lifecycle sample processor cleanup failed");
			return CallbackReturn::ERROR;
		}
	} catch (const std::exception& exception) {
		transitionErrorCode_ = kProcessorCleanupErrorCode;
		transitionErrorMessage_ = QStringLiteral("Processor cleanup raised an exception");
		RCLCPP_ERROR(
			get_logger(),
			"Lifecycle sample processor cleanup raised an exception: %s",
			exception.what());
		return CallbackReturn::ERROR;
	} catch (...) {
		transitionErrorCode_ = kProcessorCleanupErrorCode;
		transitionErrorMessage_ = QStringLiteral("Processor cleanup raised an exception");
		RCLCPP_ERROR(get_logger(), "Lifecycle sample processor cleanup raised an unknown exception");
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
	transitionErrorCode_ = 0;
	transitionErrorMessage_.clear();
	QString errorMessage;
	try {
		if (!activateProcessor(errorMessage)) {
			transitionErrorCode_ = kProcessorActivationErrorCode;
			transitionErrorMessage_ = errorMessage.trimmed().isEmpty()
				? QStringLiteral("Processor activation failed")
				: errorMessage;
			RCLCPP_ERROR(get_logger(), "Lifecycle sample processor activation failed");
			return CallbackReturn::ERROR;
		}
	} catch (const std::exception& exception) {
		transitionErrorCode_ = kProcessorActivationErrorCode;
		transitionErrorMessage_ = QStringLiteral("Processor activation raised an exception");
		RCLCPP_ERROR(
			get_logger(),
			"Lifecycle sample processor activation raised an exception: %s",
			exception.what());
		return CallbackReturn::ERROR;
	} catch (...) {
		transitionErrorCode_ = kProcessorActivationErrorCode;
		transitionErrorMessage_ = QStringLiteral("Processor activation raised an exception");
		RCLCPP_ERROR(
			get_logger(),
			"Lifecycle sample processor activation raised an unknown exception");
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
	transitionErrorCode_ = 0;
	transitionErrorMessage_.clear();
	QString errorMessage;
	try {
		if (!deactivateProcessor(errorMessage)) {
			transitionErrorCode_ = kProcessorDeactivationErrorCode;
			transitionErrorMessage_ = errorMessage.trimmed().isEmpty()
				? QStringLiteral("Processor deactivation failed")
				: errorMessage;
			RCLCPP_ERROR(get_logger(), "Lifecycle sample processor deactivation failed");
			return CallbackReturn::ERROR;
		}
	} catch (const std::exception& exception) {
		transitionErrorCode_ = kProcessorDeactivationErrorCode;
		transitionErrorMessage_ = QStringLiteral("Processor deactivation raised an exception");
		RCLCPP_ERROR(
			get_logger(),
			"Lifecycle sample processor deactivation raised an exception: %s",
			exception.what());
		return CallbackReturn::ERROR;
	} catch (...) {
		transitionErrorCode_ = kProcessorDeactivationErrorCode;
		transitionErrorMessage_ = QStringLiteral("Processor deactivation raised an exception");
		RCLCPP_ERROR(
			get_logger(),
			"Lifecycle sample processor deactivation raised an unknown exception");
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
	transitionErrorCode_ = 0;
	transitionErrorMessage_.clear();
	QString errorMessage;
	try {
		if (!shutdownProcessor(errorMessage)) {
			transitionErrorCode_ = kProcessorShutdownErrorCode;
			transitionErrorMessage_ = errorMessage.trimmed().isEmpty()
				? QStringLiteral("Processor shutdown failed")
				: errorMessage;
			RCLCPP_ERROR(get_logger(), "Lifecycle sample processor shutdown failed");
			return CallbackReturn::ERROR;
		}
	} catch (const std::exception& exception) {
		transitionErrorCode_ = kProcessorShutdownErrorCode;
		transitionErrorMessage_ = QStringLiteral("Processor shutdown raised an exception");
		RCLCPP_ERROR(
			get_logger(),
			"Lifecycle sample processor shutdown raised an exception: %s",
			exception.what());
		return CallbackReturn::ERROR;
	} catch (...) {
		transitionErrorCode_ = kProcessorShutdownErrorCode;
		transitionErrorMessage_ = QStringLiteral("Processor shutdown raised an exception");
		RCLCPP_ERROR(get_logger(), "Lifecycle sample processor shutdown raised an unknown exception");
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
		? kLifecycleTransitionErrorCode
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
