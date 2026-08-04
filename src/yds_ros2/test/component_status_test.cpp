#include <gtest/gtest.h>

#include <yds/ros2/component_status.h>
#include <yds/ros2/component_status_conversion.h>

TEST(ComponentStatusTest, ConvertsAllStatesToText) {
	EXPECT_EQ(
		yds::ros2::componentStateText(yds::ros2::ComponentState::kUnknown),
		QStringLiteral("UNKNOWN"));
	EXPECT_EQ(
		yds::ros2::componentStateText(yds::ros2::ComponentState::kInitializing),
		QStringLiteral("INITIALIZING"));
	EXPECT_EQ(
		yds::ros2::componentStateText(yds::ros2::ComponentState::kReady),
		QStringLiteral("READY"));
	EXPECT_EQ(
		yds::ros2::componentStateText(yds::ros2::ComponentState::kRunning),
		QStringLiteral("RUNNING"));
	EXPECT_EQ(
		yds::ros2::componentStateText(yds::ros2::ComponentState::kWarning),
		QStringLiteral("WARNING"));
	EXPECT_EQ(
		yds::ros2::componentStateText(yds::ros2::ComponentState::kError),
		QStringLiteral("ERROR"));
	EXPECT_EQ(
		yds::ros2::componentStateText(yds::ros2::ComponentState::kCritical),
		QStringLiteral("CRITICAL"));
	EXPECT_EQ(
		yds::ros2::componentStateText(yds::ros2::ComponentState::kStopped),
		QStringLiteral("STOPPED"));
}

TEST(ComponentStatusTest, ConvertsInvalidStateToUnknown) {
	EXPECT_EQ(
		yds::ros2::componentStateText(static_cast<yds::ros2::ComponentState>(255)),
		QStringLiteral("UNKNOWN"));
}

TEST(ComponentStatusConversionTest, ConvertsStatesInBothDirections) {
	const yds::ros2::ComponentState states[] = {
		yds::ros2::ComponentState::kUnknown,
		yds::ros2::ComponentState::kInitializing,
		yds::ros2::ComponentState::kReady,
		yds::ros2::ComponentState::kRunning,
		yds::ros2::ComponentState::kWarning,
		yds::ros2::ComponentState::kError,
		yds::ros2::ComponentState::kCritical,
		yds::ros2::ComponentState::kStopped,
	};

	for (const auto state : states) {
		EXPECT_EQ(
			yds::ros2::componentStateFromRos(yds::ros2::componentStateToRos(state)),
			state);
	}
	EXPECT_EQ(
		yds::ros2::componentStateFromRos(255),
		yds::ros2::ComponentState::kUnknown);
	EXPECT_EQ(
		yds::ros2::componentStateToRos(
			static_cast<yds::ros2::ComponentState>(255)),
		yds_interfaces::msg::ComponentStatus::STATE_UNKNOWN);
}

TEST(ComponentStatusConversionTest, PreservesValidTimestampToMillisecondPrecision) {
	const QDateTime dateTime =
		QDateTime::fromMSecsSinceEpoch(1735689600123, Qt::UTC).toLocalTime();

	const auto rosTime = yds::ros2::dateTimeToRos(dateTime);
	const auto convertedDateTime = yds::ros2::dateTimeFromRos(rosTime);

	EXPECT_EQ(convertedDateTime.toMSecsSinceEpoch(), dateTime.toMSecsSinceEpoch());
}

TEST(ComponentStatusConversionTest, KeepsUnsetTimestampInvalid) {
	const builtin_interfaces::msg::Time rosTime;

	EXPECT_FALSE(yds::ros2::dateTimeFromRos(rosTime).isValid());
	const auto convertedRosTime = yds::ros2::dateTimeToRos(QDateTime());
	EXPECT_EQ(convertedRosTime.sec, 0);
	EXPECT_EQ(convertedRosTime.nanosec, 0U);
}

TEST(ComponentStatusConversionTest, ConvertsOutOfRangeTimestampToZero) {
	const QDateTime outOfRangeDateTime =
		QDateTime::fromMSecsSinceEpoch(4102444800000, Qt::UTC);

	const auto convertedRosTime = yds::ros2::dateTimeToRos(outOfRangeDateTime);

	EXPECT_EQ(convertedRosTime.sec, 0);
	EXPECT_EQ(convertedRosTime.nanosec, 0U);
}

TEST(ComponentStatusConversionTest, ConvertsComponentStatusInBothDirections) {
	const QDateTime timestamp =
		QDateTime::fromMSecsSinceEpoch(1735689600123, Qt::UTC).toLocalTime();
	const yds::ros2::ComponentStatus status{
		QStringLiteral("camera/status"),
		QStringLiteral("カメラ-1"),
		yds::ros2::ComponentState::kError,
		1001,
		QStringLiteral("カメラとの接続に失敗しました"),
		timestamp};

	const auto rosMessage = yds::ros2::componentStatusToRos(status);
	const auto convertedStatus =
		yds::ros2::componentStatusFromRos(status.topicName, rosMessage);

	EXPECT_EQ(convertedStatus.topicName, status.topicName);
	EXPECT_EQ(convertedStatus.componentId, status.componentId);
	EXPECT_EQ(convertedStatus.state, status.state);
	EXPECT_EQ(convertedStatus.errorCode, status.errorCode);
	EXPECT_EQ(convertedStatus.message, status.message);
	EXPECT_EQ(rosMessage.component_id, std::string(u8"カメラ-1"));
	EXPECT_EQ(rosMessage.message, std::string(u8"カメラとの接続に失敗しました"));
	EXPECT_EQ(
		convertedStatus.timestamp.toMSecsSinceEpoch(),
		status.timestamp.toMSecsSinceEpoch());
}
