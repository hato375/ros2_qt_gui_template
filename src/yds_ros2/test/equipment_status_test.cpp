#include <gtest/gtest.h>

#include <yds/ros2/equipment_status.h>

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
