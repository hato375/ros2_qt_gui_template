#pragma once

#include <chrono>
#include <optional>

#include <QString>

#include "yds/ros2/topic_reception_status.h"

namespace yds::ros2 {

/// @brief ROSトピックの受信状態遷移
enum class TopicReceptionTransition {
	kNone,
	kStarted,
	kTimedOut,
	kRecovered,
};

/// @brief ROSトピックの受信時刻、件数、およびタイムアウトを管理する
///
/// ROSメッセージ型には依存せず、Subscriber側から受信内容を渡して使用する。
class TopicReceptionMonitor final {
public:
	/// @brief トピック受信モニターを生成する
	/// @param topicName 監視対象のトピック名
	/// @param timeout 受信タイムアウト時間
	TopicReceptionMonitor(
		const QString& topicName,
		std::chrono::milliseconds timeout);

	TopicReceptionMonitor(const TopicReceptionMonitor&) = delete;
	TopicReceptionMonitor& operator=(const TopicReceptionMonitor&) = delete;
	TopicReceptionMonitor(TopicReceptionMonitor&&) = delete;
	TopicReceptionMonitor& operator=(TopicReceptionMonitor&&) = delete;

	/// @brief トピックの受信を記録する
	/// @param message 最新メッセージを表す文字列
	/// @return 受信によって発生した状態遷移
	TopicReceptionTransition recordReception(const QString& message);

	/// @brief 受信タイムアウトを確認する
	/// @return 確認によって発生した状態遷移
	TopicReceptionTransition checkTimeout() noexcept;

	/// @brief 未通知の最新受信状況を取得する
	/// @return 更新がある場合は最新受信状況、ない場合は`std::nullopt`
	std::optional<TopicReceptionStatus> takeStatusUpdate();

	/// @brief 現在の受信状況を取得する
	/// @return 現在の受信状況
	const TopicReceptionStatus& status() const noexcept;

private:
	std::chrono::milliseconds timeout_;
	TopicReceptionStatus status_;
	std::chrono::steady_clock::time_point lastReceptionTime_;
	bool statusDirty_;
};

}  // namespace yds::ros2
