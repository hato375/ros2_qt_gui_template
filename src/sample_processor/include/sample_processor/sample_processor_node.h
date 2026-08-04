#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include <yds/ros2/component_status_publisher.h>

namespace sampleprocessor {

/// @brief コンポーネント状態通知を実装したロジック系サンプルノード
class SampleProcessorNode final : public rclcpp::Node {
public:
	/// @brief サンプル処理ノードを生成する
	/// @param options ROSノードオプション
	explicit SampleProcessorNode(
		const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

	/// @brief デストラクタ
	~SampleProcessorNode() override = default;

	SampleProcessorNode(const SampleProcessorNode&) = delete;
	SampleProcessorNode& operator=(const SampleProcessorNode&) = delete;
	SampleProcessorNode(SampleProcessorNode&&) = delete;
	SampleProcessorNode& operator=(SampleProcessorNode&&) = delete;

	/// @brief サンプル処理周期を取得する
	/// @return サンプル処理周期（ミリ秒）
	std::int64_t processingIntervalMs() const noexcept;

	/// @brief 完了したサンプル処理回数を取得する
	/// @return 完了した処理回数
	std::uint64_t processedCount() const noexcept;

private:
	void process() noexcept;

	std::int64_t processingIntervalMs_;
	std::atomic<std::uint64_t> processedCount_;
	bool ready_;
	std::unique_ptr<yds::ros2::ComponentStatusPublisher> statusPublisher_;
	rclcpp::TimerBase::SharedPtr processingTimer_;
};

}  // namespace sampleprocessor
