#include <gtest/gtest.h>

#include <yds/ros2/equipment_status.h>
#include <yds/ros2/equipment_status_conversion.h>

TEST(EquipmentStatusTest, ConvertsAllStatesToText) {
	EXPECT_EQ(
		yds::ros2::equipmentStateText(yds::ros2::EquipmentState::kUnknown),
		QStringLiteral("UNKNOWN"));
	EXPECT_EQ(
		yds::ros2::equipmentStateText(yds::ros2::EquipmentState::kInitializing),
		QStringLiteral("INITIALIZING"));
	EXPECT_EQ(
		yds::ros2::equipmentStateText(yds::ros2::EquipmentState::kReady),
		QStringLiteral("READY"));
	EXPECT_EQ(
		yds::ros2::equipmentStateText(yds::ros2::EquipmentState::kRunning),
		QStringLiteral("RUNNING"));
	EXPECT_EQ(
		yds::ros2::equipmentStateText(yds::ros2::EquipmentState::kWarning),
		QStringLiteral("WARNING"));
	EXPECT_EQ(
		yds::ros2::equipmentStateText(yds::ros2::EquipmentState::kError),
		QStringLiteral("ERROR"));
	EXPECT_EQ(
		yds::ros2::equipmentStateText(yds::ros2::EquipmentState::kCritical),
		QStringLiteral("CRITICAL"));
	EXPECT_EQ(
		yds::ros2::equipmentStateText(yds::ros2::EquipmentState::kStopped),
		QStringLiteral("STOPPED"));
}

TEST(EquipmentStatusTest, ConvertsInvalidStateToUnknown) {
	EXPECT_EQ(
		yds::ros2::equipmentStateText(static_cast<yds::ros2::EquipmentState>(255)),
		QStringLiteral("UNKNOWN"));
}

TEST(EquipmentStatusConversionTest, ConvertsStatesInBothDirections) {
	const yds::ros2::EquipmentState states[] = {
		yds::ros2::EquipmentState::kUnknown,
		yds::ros2::EquipmentState::kInitializing,
		yds::ros2::EquipmentState::kReady,
		yds::ros2::EquipmentState::kRunning,
		yds::ros2::EquipmentState::kWarning,
		yds::ros2::EquipmentState::kError,
		yds::ros2::EquipmentState::kCritical,
		yds::ros2::EquipmentState::kStopped,
	};

	for (const auto state : states) {
		EXPECT_EQ(
			yds::ros2::equipmentStateFromRos(yds::ros2::equipmentStateToRos(state)),
			state);
	}
	EXPECT_EQ(
		yds::ros2::equipmentStateFromRos(255),
		yds::ros2::EquipmentState::kUnknown);
	EXPECT_EQ(
		yds::ros2::equipmentStateToRos(
			static_cast<yds::ros2::EquipmentState>(255)),
		yds_interfaces::msg::EquipmentStatus::STATE_UNKNOWN);
}

TEST(EquipmentStatusConversionTest, PreservesValidTimestampToMillisecondPrecision) {
	const QDateTime dateTime =
		QDateTime::fromMSecsSinceEpoch(1735689600123, Qt::UTC).toLocalTime();

	const auto rosTime = yds::ros2::dateTimeToRos(dateTime);
	const auto convertedDateTime = yds::ros2::dateTimeFromRos(rosTime);

	EXPECT_EQ(convertedDateTime.toMSecsSinceEpoch(), dateTime.toMSecsSinceEpoch());
}

TEST(EquipmentStatusConversionTest, KeepsUnsetTimestampInvalid) {
	const builtin_interfaces::msg::Time rosTime;

	EXPECT_FALSE(yds::ros2::dateTimeFromRos(rosTime).isValid());
	const auto convertedRosTime = yds::ros2::dateTimeToRos(QDateTime());
	EXPECT_EQ(convertedRosTime.sec, 0);
	EXPECT_EQ(convertedRosTime.nanosec, 0U);
}

TEST(EquipmentStatusConversionTest, ConvertsOutOfRangeTimestampToZero) {
	const QDateTime outOfRangeDateTime =
		QDateTime::fromMSecsSinceEpoch(4102444800000, Qt::UTC);

	const auto convertedRosTime = yds::ros2::dateTimeToRos(outOfRangeDateTime);

	EXPECT_EQ(convertedRosTime.sec, 0);
	EXPECT_EQ(convertedRosTime.nanosec, 0U);
}

TEST(EquipmentStatusConversionTest, ConvertsEquipmentStatusInBothDirections) {
	const QDateTime timestamp =
		QDateTime::fromMSecsSinceEpoch(1735689600123, Qt::UTC).toLocalTime();
	const yds::ros2::EquipmentStatus status{
		QStringLiteral("camera/status"),
		QStringLiteral("camera-1"),
		yds::ros2::EquipmentState::kError,
		1001,
		QStringLiteral("connection failed"),
		timestamp};

	const auto rosMessage = yds::ros2::equipmentStatusToRos(status);
	const auto convertedStatus =
		yds::ros2::equipmentStatusFromRos(status.topicName, rosMessage);

	EXPECT_EQ(convertedStatus.topicName, status.topicName);
	EXPECT_EQ(convertedStatus.equipmentId, status.equipmentId);
	EXPECT_EQ(convertedStatus.state, status.state);
	EXPECT_EQ(convertedStatus.errorCode, status.errorCode);
	EXPECT_EQ(convertedStatus.message, status.message);
	EXPECT_EQ(
		convertedStatus.timestamp.toMSecsSinceEpoch(),
		status.timestamp.toMSecsSinceEpoch());
}
