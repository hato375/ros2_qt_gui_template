#include "ros_qt_bridge.h"

namespace ros2qtgui {

RosQtBridge::RosQtBridge(QObject* parent)
	: QObject(parent) {
	qRegisterMetaType<yds::ros2::ApplicationEvent>("yds::ros2::ApplicationEvent");
	qRegisterMetaType<yds::ros2::TopicReceptionStatus>(
		"yds::ros2::TopicReceptionStatus");
	qRegisterMetaType<yds::ros2::EquipmentStatus>("yds::ros2::EquipmentStatus");
}

void RosQtBridge::notifyHeartbeat(std::uint64_t count) noexcept {
	emit heartbeatUpdated(static_cast<quint64>(count));
}

void RosQtBridge::notifyApplicationEvent(
	const yds::ros2::ApplicationEvent& event) noexcept {
	emit applicationEventOccurred(event);
}

void RosQtBridge::notifyTopicReceptionStatus(
	const yds::ros2::TopicReceptionStatus& status) noexcept {
	emit topicReceptionStatusUpdated(status);
}

void RosQtBridge::notifyEquipmentStatus(
	const yds::ros2::EquipmentStatus& status) noexcept {
	emit equipmentStatusUpdated(status);
}

}  // namespace ros2qtgui
