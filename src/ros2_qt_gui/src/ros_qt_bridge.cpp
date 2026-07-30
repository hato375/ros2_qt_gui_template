#include "ros_qt_bridge.h"

namespace ros2qtgui {

RosQtBridge::RosQtBridge(QObject* parent)
	: QObject(parent) {
	qRegisterMetaType<ApplicationEvent>("ros2qtgui::ApplicationEvent");
}

void RosQtBridge::notifyHeartbeat(std::uint64_t count) noexcept {
	emit heartbeatUpdated(static_cast<quint64>(count));
}

void RosQtBridge::notifyApplicationEvent(const ApplicationEvent& event) noexcept {
	emit applicationEventOccurred(event);
}

}  // namespace ros2qtgui
