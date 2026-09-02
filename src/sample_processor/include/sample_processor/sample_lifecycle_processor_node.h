#pragma once

#include <atomic>
#include <cstdint>

#include <QString>

#include <rclcpp/rclcpp.hpp>

#include <yds/ros2/lifecycle_component_status_node.h>

namespace sampleprocessor {

/// @brief コンポーネント状態通知を実装したLifecycleサンプルノード
class SampleLifecycleProcessorNode : public yds::ros2::LifecycleComponentStatusNode {
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

protected:
	/// @brief configure時の設備固有処理。派生クラスで接続や設定処理へ置き換える
	/// @param errorMessage 失敗理由。falseを返す場合に設定する
	/// @return 成功した場合true
	virtual bool configureProcessor(QString& errorMessage);
	/// @brief activate時の設備固有処理。派生クラスで処理開始へ置き換える
	/// @param errorMessage 失敗理由。falseを返す場合に設定する
	/// @return 成功した場合true
	virtual bool activateProcessor(QString& errorMessage);
	/// @brief deactivate時の設備固有処理。派生クラスで処理の安全停止へ置き換える
	/// @param errorMessage 失敗理由。falseを返す場合に設定する
	/// @return 成功した場合true
	virtual bool deactivateProcessor(QString& errorMessage);
	/// @brief cleanup時の設備固有処理。派生クラスで接続や設定資源の解放へ置き換える
	/// @param errorMessage 失敗理由。falseを返す場合に設定する
	/// @return 成功した場合true
	virtual bool cleanupProcessor(QString& errorMessage);
	/// @brief shutdown時の設備固有処理。派生クラスで最終終了処理へ置き換える
	/// @param errorMessage 失敗理由。falseを返す場合に設定する
	/// @return 成功した場合true
	virtual bool shutdownProcessor(QString& errorMessage);

	CallbackReturn on_configure(const rclcpp_lifecycle::State& previousState) override;
	CallbackReturn on_cleanup(const rclcpp_lifecycle::State& previousState) override;
	CallbackReturn on_activate(const rclcpp_lifecycle::State& previousState) override;
	CallbackReturn on_deactivate(const rclcpp_lifecycle::State& previousState) override;
	CallbackReturn on_shutdown(const rclcpp_lifecycle::State& previousState) override;
	CallbackReturn on_error(const rclcpp_lifecycle::State& previousState) override;

private:
	using ProcessorHook = bool (SampleLifecycleProcessorNode::*)(QString&);

	void process() noexcept;
	bool executeProcessorHook(
		ProcessorHook hook,
		qint32 errorCode,
		const char* operationName,
		const QString& defaultFailureMessage,
		const QString& exceptionMessage);
	bool updateComponentStatus(
		yds::ros2::ComponentState state,
		qint32 errorCode,
		const QString& message) noexcept;

	std::int64_t processingIntervalMs_;
	std::atomic<std::uint64_t> processedCount_;
	qint32 transitionErrorCode_;
	QString transitionErrorMessage_;
	rclcpp::TimerBase::SharedPtr processingTimer_;
};

}  // namespace sampleprocessor
