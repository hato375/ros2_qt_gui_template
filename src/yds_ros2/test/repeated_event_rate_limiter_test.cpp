#include <chrono>

#include <gtest/gtest.h>

#include <yds/ros2/repeated_event_rate_limiter.h>

namespace {

TEST(RepeatedEventRateLimiterTest, ReportsFirstEventAndSummarizesSuppressedEvents) {
	using namespace std::chrono_literals;
	yds::ros2::RepeatedEventRateLimiter limiter(10s);
	const auto start = yds::ros2::RepeatedEventRateLimiter::Clock::time_point(100s);

	const auto first = limiter.record(start);
	EXPECT_TRUE(first.shouldReport);
	EXPECT_EQ(first.suppressedCount, 0U);

	EXPECT_FALSE(limiter.record(start + 1s).shouldReport);
	EXPECT_FALSE(limiter.record(start + 9s).shouldReport);

	const auto nextWindow = limiter.record(start + 10s);
	EXPECT_TRUE(nextWindow.shouldReport);
	EXPECT_EQ(nextWindow.suppressedCount, 2U);
}

TEST(RepeatedEventRateLimiterTest, ResetReportsNextEventImmediately) {
	using namespace std::chrono_literals;
	yds::ros2::RepeatedEventRateLimiter limiter(10s);
	const auto start = yds::ros2::RepeatedEventRateLimiter::Clock::time_point(100s);

	EXPECT_TRUE(limiter.record(start).shouldReport);
	EXPECT_FALSE(limiter.record(start + 1s).shouldReport);
	limiter.reset();

	const auto afterReset = limiter.record(start + 2s);
	EXPECT_TRUE(afterReset.shouldReport);
	EXPECT_EQ(afterReset.suppressedCount, 0U);
}

TEST(RepeatedEventRateLimiterTest, RejectsNonPositiveInterval) {
	EXPECT_THROW(
		yds::ros2::RepeatedEventRateLimiter(std::chrono::milliseconds::zero()),
		std::invalid_argument);
	EXPECT_THROW(
		yds::ros2::RepeatedEventRateLimiter(std::chrono::milliseconds(-1)),
		std::invalid_argument);
}

}  // namespace
