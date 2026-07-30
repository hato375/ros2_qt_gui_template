#pragma once

#include <memory>
#include <thread>

#include <rclcpp/executor.hpp>
#include <rclcpp/node.hpp>

namespace yds::ros2 {

/// @brief ROS 2 Executorを専用スレッドで実行する
///
/// デストラクタでExecutorを停止し、実行スレッドを必ずjoinする。
class ExecutorRunner final {
public:
	/// @brief ノードを登録してExecutorスレッドを開始する
	/// @param node Executorへ登録するROS 2ノード
	explicit ExecutorRunner(const std::shared_ptr<rclcpp::Node>& node);

	/// @brief Executorを停止して実行スレッドをjoinする
	~ExecutorRunner();

	ExecutorRunner(const ExecutorRunner&) = delete;
	ExecutorRunner& operator=(const ExecutorRunner&) = delete;
	ExecutorRunner(ExecutorRunner&&) = delete;
	ExecutorRunner& operator=(ExecutorRunner&&) = delete;

	/// @brief Executorを停止して実行スレッドをjoinする
	///
	/// 複数回呼び出しても安全である。
	void stop() noexcept;

private:
	std::shared_ptr<rclcpp::Executor> executor_;
	std::thread executorThread_;
};

}  // namespace yds::ros2
