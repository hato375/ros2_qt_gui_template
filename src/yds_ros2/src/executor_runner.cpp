#include "yds/ros2/executor_runner.h"

#include <exception>
#include <utility>

#include <rclcpp/executors/single_threaded_executor.hpp>
#include <rclcpp/logging.hpp>

namespace yds::ros2 {

namespace {

constexpr char kLoggerName[] = "yds.ros2.executor_runner";
constexpr char kExecutorStartedMessage[] = "ROS 2 executor thread started";
constexpr char kExecutorStoppingMessage[] = "Stopping ROS 2 executor thread";
constexpr char kExecutorStoppedMessage[] = "ROS 2 executor thread stopped";

}  // namespace

ExecutorRunner::ExecutorRunner(const std::shared_ptr<rclcpp::Node>& node)
	: executor_(std::make_shared<rclcpp::executors::SingleThreadedExecutor>()) {
	executor_->add_node(node);
	executorThread_ = std::thread([executor = executor_]() {
		const auto logger = rclcpp::get_logger(kLoggerName);
		RCLCPP_INFO(logger, "%s", kExecutorStartedMessage);
		try {
			executor->spin();
		} catch (const std::exception& exception) {
			RCLCPP_ERROR(logger, "ROS 2 executor failed: %s", exception.what());
		} catch (...) {
			RCLCPP_ERROR(logger, "ROS 2 executor failed with an unknown error");
		}
		RCLCPP_INFO(logger, "%s", kExecutorStoppedMessage);
	});
}

ExecutorRunner::~ExecutorRunner() {
	stop();
}

void ExecutorRunner::stop() noexcept {
	if (!executorThread_.joinable()) {
		return;
	}

	RCLCPP_INFO(rclcpp::get_logger(kLoggerName), "%s", kExecutorStoppingMessage);
	executor_->cancel();
	executorThread_.join();
}

}  // namespace yds::ros2
