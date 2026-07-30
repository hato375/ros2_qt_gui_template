#include <exception>
#include <memory>

#include <QApplication>
#include <QMessageBox>
#include <QObject>

#include <rclcpp/rclcpp.hpp>

#include "main_window.h"
#include "ros_executor_runner.h"
#include "ros_node.h"
#include "ros_qt_bridge.h"

int main(int argc, char* argv[]) {
	rclcpp::init(argc, argv);
	QApplication application(argc, argv);

	try {
		ros2qtgui::RosQtBridge rosQtBridge;
		auto rosNode = std::make_shared<ros2qtgui::RosNode>(
			[&rosQtBridge](std::uint64_t count) {
				rosQtBridge.notifyHeartbeat(count);
			},
			[&rosQtBridge](const ros2qtgui::ApplicationEvent& event) {
				rosQtBridge.notifyApplicationEvent(event);
			});

		ros2qtgui::MainWindow mainWindow(
			static_cast<int>(rosNode->guiStatusCheckIntervalMs()));
		QObject::connect(
			&rosQtBridge,
			&ros2qtgui::RosQtBridge::heartbeatUpdated,
			&mainWindow,
			&ros2qtgui::MainWindow::setHeartbeatCount,
			Qt::QueuedConnection);
		QObject::connect(
			&rosQtBridge,
			&ros2qtgui::RosQtBridge::applicationEventOccurred,
			&mainWindow,
			&ros2qtgui::MainWindow::appendApplicationEvent,
			Qt::QueuedConnection);

		rosQtBridge.notifyApplicationEvent({
			ros2qtgui::ApplicationEventLevel::kInfo,
			QDateTime::currentDateTime(),
			QStringLiteral("ROS 2 node started")});
		rosQtBridge.notifyApplicationEvent({
			ros2qtgui::ApplicationEventLevel::kInfo,
			QDateTime::currentDateTime(),
			QStringLiteral("Configuration: heartbeat_interval_ms=%1, gui_status_check_interval_ms=%2")
				.arg(rosNode->heartbeatIntervalMs())
				.arg(rosNode->guiStatusCheckIntervalMs())});

		ros2qtgui::RosExecutorRunner executorRunner(rosNode);
		mainWindow.show();
		const int result = application.exec();

		RCLCPP_INFO(rosNode->get_logger(), "GUI closed; stopping ROS 2 node");
		executorRunner.stop();
		rclcpp::shutdown();
		return result;
	} catch (const std::exception& exception) {
		if (rclcpp::ok()) {
			rclcpp::shutdown();
		}
		QMessageBox::critical(nullptr, "ROS 2 + Qt GUI", exception.what());
		return 1;
	}
}
