#include <sample_processor/sample_processor_node.h>

#include <chrono>
#include <stdexcept>
#include <string>

#include <QString>
#include <QByteArray>

#include <rcl_interfaces/msg/integer_range.hpp>
#include <rcl_interfaces/msg/parameter_descriptor.hpp>

#include <yds/ros2/component_status_parameters.h>

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
	  testState_(yds::ros2::ComponentState::kRunning),
	  ready_(false),
	  statusPublisher_(),
	  processingTimer_(),
	  setWarningService_(),
	  setErrorService_(),
	  recoverService_() {
	validateInterval("processing_interval_ms", processingIntervalMs_);

	statusPublisher_ = std::make_unique<yds::ros2::ComponentStatusPublisher>(
		*this,
		yds::ros2::declareComponentStatusPublisherParameters(
			*this,
			yds::ros2::ComponentStatusPublisherConfiguration{
				QStringLiteral("sample-processor-1"),
				QStringLiteral("sample_processor/status"),
				std::chrono::milliseconds(1000)}));
	processingTimer_ = create_wall_timer(
		std::chrono::milliseconds(processingIntervalMs_),
		[this]() {
			process();
		});
	setWarningService_ = create_service<std_srvs::srv::Trigger>(
		"~/set_warning",
		[this](
			const std_srvs::srv::Trigger::Request::SharedPtr,
			std_srvs::srv::Trigger::Response::SharedPtr response) {
			setTestState(
				yds::ros2::ComponentState::kWarning,
				1001,
				QStringLiteral("Test warning requested"),
				*response);
		});
	setErrorService_ = create_service<std_srvs::srv::Trigger>(
		"~/set_error",
		[this](
			const std_srvs::srv::Trigger::Request::SharedPtr,
			std_srvs::srv::Trigger::Response::SharedPtr response) {
			setTestState(
				yds::ros2::ComponentState::kError,
				2001,
				QStringLiteral("Test error requested"),
				*response);
		});
	recoverService_ = create_service<std_srvs::srv::Trigger>(
		"~/recover",
		[this](
			const std_srvs::srv::Trigger::Request::SharedPtr,
			std_srvs::srv::Trigger::Response::SharedPtr response) {
			setTestState(
				yds::ros2::ComponentState::kRunning,
				0,
				QStringLiteral("Test state recovered"),
				*response);
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
	const yds::ros2::ComponentState testState = testState_.load();
	if (!ready_ && testState == yds::ros2::ComponentState::kRunning) {
		ready_ = true;
		if (statusPublisher_->setStatus(
			yds::ros2::ComponentState::kReady,
			0,
			QStringLiteral("Initialization completed"))) {
			RCLCPP_INFO(get_logger(), "Sample processor initialization completed");
		}
		return;
	}

	if (testState == yds::ros2::ComponentState::kWarning) {
		statusPublisher_->setStatus(
			testState,
			1001,
			QStringLiteral("Test warning requested"));
		return;
	}
	if (testState == yds::ros2::ComponentState::kError) {
		statusPublisher_->setStatus(
			testState,
			2001,
			QStringLiteral("Test error requested"));
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

void SampleProcessorNode::setTestState(
	yds::ros2::ComponentState state,
	qint32 errorCode,
	const QString& message,
	std_srvs::srv::Trigger::Response& response) noexcept {
	testState_.store(state);
	response.success = statusPublisher_->setStatus(state, errorCode, message);
	response.message = response.success
		? "Component test state updated"
		: "Failed to publish component test state";
	if (response.success) {
		const QByteArray stateTextUtf8 = yds::ros2::componentStateText(state).toUtf8();
		RCLCPP_INFO(
			get_logger(),
			"Component test state changed: state=%s, error_code=%ld",
			stateTextUtf8.constData(),
			static_cast<long>(errorCode));
	}
}

}  // namespace sampleprocessor
