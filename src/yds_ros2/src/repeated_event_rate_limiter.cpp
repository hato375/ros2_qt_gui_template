#include <yds/ros2/repeated_event_rate_limiter.h>

#include <stdexcept>

namespace yds::ros2 {

RepeatedEventRateLimiter::RepeatedEventRateLimiter(std::chrono::milliseconds interval)
	: interval_(interval),
	  mutex_(),
	  hasReported_(false),
	  lastReportedAt_(),
	  suppressedCount_(0) {
	if (interval_ <= std::chrono::milliseconds::zero()) {
		throw std::invalid_argument("event rate limit interval must be greater than zero");
	}
}

RepeatedEventRateLimitResult RepeatedEventRateLimiter::record(
	Clock::time_point now) noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	if (!hasReported_ || now - lastReportedAt_ >= interval_) {
		const std::uint64_t suppressedCount = suppressedCount_;
		hasReported_ = true;
		lastReportedAt_ = now;
		suppressedCount_ = 0;
		return {true, suppressedCount};
	}

	++suppressedCount_;
	return {false, 0};
}

void RepeatedEventRateLimiter::reset() noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	hasReported_ = false;
	lastReportedAt_ = Clock::time_point();
	suppressedCount_ = 0;
}

}  // namespace yds::ros2
