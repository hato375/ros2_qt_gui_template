#include <yds/ros2/component_status_node.h>

#include <yds/ros2/component_status_parameters.h>

namespace yds::ros2 {

ComponentStatusNode::ComponentStatusNode(
	const std::string& nodeName,
	const QString& defaultComponentId,
	const QString& defaultStatusTopicName,
	std::chrono::milliseconds defaultPublishInterval,
	const rclcpp::NodeOptions& options)
	: rclcpp::Node(nodeName, options),
	  statusPublisher_() {
	statusPublisher_ = std::make_unique<ComponentStatusPublisher>(
		*this,
		declareComponentStatusPublisherParameters(
			*this,
			ComponentStatusPublisherConfiguration{
				defaultComponentId,
				defaultStatusTopicName,
				defaultPublishInterval}));
}

bool ComponentStatusNode::setComponentStatus(
	ComponentState state,
	qint32 errorCode,
	const QString& message) noexcept {
	return statusPublisher_->setStatus(state, errorCode, message);
}

ComponentStatus ComponentStatusNode::componentStatus() const {
	return statusPublisher_->status();
}

const QString& ComponentStatusNode::componentId() const noexcept {
	return statusPublisher_->componentId();
}

const QString& ComponentStatusNode::statusTopicName() const noexcept {
	return statusPublisher_->statusTopicName();
}

std::chrono::milliseconds ComponentStatusNode::statusPublishInterval() const noexcept {
	return statusPublisher_->publishInterval();
}

}  // namespace yds::ros2
