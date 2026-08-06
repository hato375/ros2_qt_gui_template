#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

#include <QString>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>

#include <yds/ros2/component_status_publisher.h>

namespace sampleprocessor {

/// @brief コンポーネント状態通知を実装したLifecycleサンプルノード
class SampleLifecycleProcessorNode : public rclcpp_lifecycle::LifecycleNode {
public:
	/// @brief Lifecycleサンプル処理ノードを生成する
	/// @param options ROSノードオプション
	explicit SampleLifecycleProcessorNode(
		const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

	/// @brief デストラクタ
	~SampleLifecycleProcessorNode() override = default;

	SampleLifecycleProcessorNode(const SampleLifecycleProcessorNode&) = delete;
	SampleLifecycleProcessorNode& operator=(const SampleLifecycleProcessorNode&) = delete;
	SampleLifecycleProcessorNode(SampleLifecycleProcessorNode&&) = delete;
	SampleLifecycleProcessorNode& operator=(SampleLifecycleProcessorNode&&) = delete;

	/// @brief サンプル処理周期を取得する
	std::int64_t processingIntervalMs() const noexcept;

	/// @brief 完了したサンプル処理回数を取得する
	std::uint64_t processedCount() const noexcept;

	/// @brief 現在のコンポーネント状態を取得する
	yds::ros2::ComponentStatus componentStatus() const;

protected:
	/// @brief configure時の設備固有処理。派生クラスで接続や設定処理へ置き換える
	/// @param errorMessage 失敗理由。falseを返す場合に設定する
	/// @return 成功した場合true
	virtual bool configureProcessor(QString& errorMessage);
	/// @brief activate時の設備固有処理。派生クラスで処理開始へ置き換える
	/// @param errorMessage 失敗理由。falseを返す場合に設定する
	/// @return 成功した場合true
	virtual bool activateProcessor(QString& errorMessage);

	CallbackReturn on_configure(const rclcpp_lifecycle::State& previousState) override;
	CallbackReturn on_cleanup(const rclcpp_lifecycle::State& previousState) override;
	CallbackReturn on_activate(const rclcpp_lifecycle::State& previousState) override;
	CallbackReturn on_deactivate(const rclcpp_lifecycle::State& previousState) override;
	CallbackReturn on_shutdown(const rclcpp_lifecycle::State& previousState) override;
	CallbackReturn on_error(const rclcpp_lifecycle::State& previousState) override;

private:
	void process() noexcept;
	bool updateComponentStatus(
		yds::ros2::ComponentState state,
		qint32 errorCode,
		const QString& message) noexcept;

	std::int64_t processingIntervalMs_;
	std::atomic<std::uint64_t> processedCount_;
	qint32 transitionErrorCode_;
	QString transitionErrorMessage_;
	std::unique_ptr<yds::ros2::ComponentStatusPublisher> statusPublisher_;
	rclcpp::TimerBase::SharedPtr processingTimer_;
};

}  // namespace sampleprocessor
