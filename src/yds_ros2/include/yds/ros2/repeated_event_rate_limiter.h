#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>

namespace yds::ros2 {

/// @brief 繰り返しイベントを通知するかどうかの判定結果
struct RepeatedEventRateLimitResult {
	/// @brief 今回のイベントを通知する場合はtrue
	bool shouldReport;
	/// @brief 前回の通知後に抑制したイベント数
	std::uint64_t suppressedCount;
};

/// @brief 同じ処理経路で繰り返すイベントの通知頻度を制限するクラス
class RepeatedEventRateLimiter final {
public:
	using Clock = std::chrono::steady_clock;

	/// @brief 通知間隔を指定して生成する
	/// @param interval 最小通知間隔。0より大きい値を指定する
	explicit RepeatedEventRateLimiter(std::chrono::milliseconds interval);

	/// @brief イベントを記録し、通知可否を判定する
	/// @param now 判定に使用する単調増加時刻
	/// @return 通知可否と、前回の通知後に抑制したイベント数
	RepeatedEventRateLimitResult record(Clock::time_point now = Clock::now()) noexcept;

	/// @brief 成功または復旧時に抑制状態を解除する
	void reset() noexcept;

private:
	std::chrono::milliseconds interval_;
	std::mutex mutex_;
	bool hasReported_;
	Clock::time_point lastReportedAt_;
	std::uint64_t suppressedCount_;
};

}  // namespace yds::ros2
