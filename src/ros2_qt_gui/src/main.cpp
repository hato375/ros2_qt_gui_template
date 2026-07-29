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
		auto rosNode = std::make_shared<ros2qtgui::RosNode>([&rosQtBridge](std::uint64_t count) {
			rosQtBridge.notifyHeartbeat(count);
		});

		ros2qtgui::MainWindow mainWindow;
		QObject::connect(
			&rosQtBridge,
			&ros2qtgui::RosQtBridge::heartbeatUpdated,
			&mainWindow,
			&ros2qtgui::MainWindow::setHeartbeatCount,
			Qt::QueuedConnection);

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
