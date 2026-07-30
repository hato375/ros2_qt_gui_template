#include "ros_qt_bridge.h"

namespace ros2qtgui {

RosQtBridge::RosQtBridge(QObject* parent)
	: QObject(parent) {
	qRegisterMetaType<yds::ros2::ApplicationEvent>("yds::ros2::ApplicationEvent");
}

void RosQtBridge::notifyHeartbeat(std::uint64_t count) noexcept {
	emit heartbeatUpdated(static_cast<quint64>(count));
}

void RosQtBridge::notifyApplicationEvent(
	const yds::ros2::ApplicationEvent& event) noexcept {
	emit applicationEventOccurred(event);
}

}  // namespace ros2qtgui
