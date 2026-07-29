#pragma once

#include <memory>
#include <thread>

#include <rclcpp/executor.hpp>
#include <rclcpp/node.hpp>

namespace ros2qtgui {

/// @brief ROS 2 Executorを専用スレッドで実行する
///
/// デストラクタでExecutorを停止し、実行スレッドを必ずjoinする。
class RosExecutorRunner final {
public:
	/// @brief ノードを登録してExecutorスレッドを開始する
	/// @param node Executorへ登録するROS 2ノード
	explicit RosExecutorRunner(const std::shared_ptr<rclcpp::Node>& node);

	/// @brief Executorを停止して実行スレッドをjoinする
	~RosExecutorRunner();

	RosExecutorRunner(const RosExecutorRunner&) = delete;
	RosExecutorRunner& operator=(const RosExecutorRunner&) = delete;
	RosExecutorRunner(RosExecutorRunner&&) = delete;
	RosExecutorRunner& operator=(RosExecutorRunner&&) = delete;

	/// @brief Executorを停止して実行スレッドをjoinする
	///
	/// 複数回呼び出しても安全である。
	void stop() noexcept;

private:
	std::shared_ptr<rclcpp::Executor> executor_;
	std::thread executorThread_;
};

}  // namespace ros2qtgui
