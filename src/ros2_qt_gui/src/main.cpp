#include <exception>
#include <memory>

#include <QApplication>
#include <QMessageBox>
#include <QObject>
#include <QStringList>

#include <rclcpp/rclcpp.hpp>

#include <yds/ros2/executor_runner.h>

#include "main_window.h"
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
			[&rosQtBridge](const yds::ros2::ApplicationEvent& event) {
				rosQtBridge.notifyApplicationEvent(event);
			},
			[&rosQtBridge](const yds::ros2::TopicReceptionStatus& status) {
				rosQtBridge.notifyTopicReceptionStatus(status);
			},
			[&rosQtBridge](const yds::ros2::ComponentStatus& status) {
				rosQtBridge.notifyComponentStatus(status);
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
		QObject::connect(
			&rosQtBridge,
			&ros2qtgui::RosQtBridge::topicReceptionStatusUpdated,
			&mainWindow,
			&ros2qtgui::MainWindow::setTopicReceptionStatus,
			Qt::QueuedConnection);
		QObject::connect(
			&rosQtBridge,
			&ros2qtgui::RosQtBridge::componentStatusUpdated,
			&mainWindow,
			&ros2qtgui::MainWindow::setComponentStatus,
			Qt::QueuedConnection);

		QStringList componentMonitorDescriptions;
		for (const auto& configuration : rosNode->componentMonitorConfigurations()) {
			mainWindow.setComponentDisplayName(
				configuration.statusTopicName,
				configuration.displayName);
			componentMonitorDescriptions.push_back(
				QStringLiteral("%1(display_name=%2, topic=%3, timeout_ms=%4)")
					.arg(
						configuration.name,
						configuration.displayName,
						configuration.statusTopicName)
					.arg(configuration.timeoutMs));
			rosQtBridge.notifyTopicReceptionStatus({
				configuration.statusTopicName,
				yds::ros2::TopicReceptionState::kWaiting,
				QDateTime(),
				0,
				QString()});
		}

		rosQtBridge.notifyApplicationEvent({
			yds::ros2::ApplicationEventLevel::kInfo,
			QDateTime::currentDateTime(),
			QStringLiteral("ROS 2 node started")});
		rosQtBridge.notifyApplicationEvent({
			yds::ros2::ApplicationEventLevel::kInfo,
			QDateTime::currentDateTime(),
			QStringLiteral(
				"Configuration: heartbeat_interval_ms=%1, gui_status_check_interval_ms=%2, "
				"component_monitors=[%3]")
				.arg(rosNode->heartbeatIntervalMs())
				.arg(rosNode->guiStatusCheckIntervalMs())
				.arg(componentMonitorDescriptions.join(QStringLiteral(", ")))});

		yds::ros2::ExecutorRunner executorRunner(rosNode);
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
