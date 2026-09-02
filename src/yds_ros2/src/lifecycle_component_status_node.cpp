#include <yds/ros2/lifecycle_component_status_node.h>

#include <yds/ros2/component_status_parameters.h>

namespace yds::ros2 {

LifecycleComponentStatusNode::LifecycleComponentStatusNode(
	const std::string& nodeName,
	const QString& defaultComponentId,
	const QString& defaultStatusTopicName,
	std::chrono::milliseconds defaultPublishInterval,
	const rclcpp::NodeOptions& options)
	: rclcpp_lifecycle::LifecycleNode(nodeName, options),
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

bool LifecycleComponentStatusNode::setComponentStatus(
	ComponentState state,
	qint32 errorCode,
	const QString& message) noexcept {
	return statusPublisher_->setStatus(state, errorCode, message);
}

ComponentStatus LifecycleComponentStatusNode::componentStatus() const {
	return statusPublisher_->status();
}

const QString& LifecycleComponentStatusNode::componentId() const noexcept {
	return statusPublisher_->componentId();
}

const QString& LifecycleComponentStatusNode::statusTopicName() const noexcept {
	return statusPublisher_->statusTopicName();
}

std::chrono::milliseconds LifecycleComponentStatusNode::statusPublishInterval() const noexcept {
	return statusPublisher_->publishInterval();
}

}  // namespace yds::ros2
