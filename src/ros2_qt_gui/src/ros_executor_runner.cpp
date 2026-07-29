#include "ros_executor_runner.h"

#include <utility>

#include <rclcpp/executors/single_threaded_executor.hpp>

namespace ros2qtgui {

RosExecutorRunner::RosExecutorRunner(const std::shared_ptr<rclcpp::Node>& node)
	: executor_(std::make_shared<rclcpp::executors::SingleThreadedExecutor>()) {
	executor_->add_node(node);
	executorThread_ = std::thread([executor = executor_]() {
		executor->spin();
	});
}

RosExecutorRunner::~RosExecutorRunner() {
	stop();
}

void RosExecutorRunner::stop() noexcept {
	executor_->cancel();
	if (executorThread_.joinable()) {
		executorThread_.join();
	}
}

}  // namespace ros2qtgui
