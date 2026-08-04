#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include <QApplication>
#include <QColor>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QHash>
#include <QLabel>
#include <QObject>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QThread>

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <yds_interfaces/msg/component_status.hpp>

#include <yds/ros2/executor_runner.h>

#include "main_window.h"
#include "ros_node.h"
#include "ros_qt_bridge.h"

namespace {

class HeartbeatReceiver final : public QObject {
public:
	explicit HeartbeatReceiver(QObject* parent = nullptr)
		: QObject(parent),
		  receivedCount_(0),
		  heartbeatCount_(0),
		  receiverThread_(nullptr) {
	}

	void receiveHeartbeat(quint64 count) noexcept {
		++receivedCount_;
		heartbeatCount_ = count;
		receiverThread_ = QThread::currentThread();
	}

	int receivedCount() const noexcept {
		return receivedCount_;
	}

	quint64 heartbeatCount() const noexcept {
		return heartbeatCount_;
	}

	const QThread* receiverThread() const noexcept {
		return receiverThread_;
	}

private:
	int receivedCount_;
	quint64 heartbeatCount_;
	const QThread* receiverThread_;
};

class ApplicationEventReceiver final : public QObject {
public:
	explicit ApplicationEventReceiver(QObject* parent = nullptr)
		: QObject(parent),
		  receivedCount_(0),
		  lastEvent_({
			  yds::ros2::ApplicationEventLevel::kInfo,
			  QDateTime(),
			  QString()}),
		  receiverThread_(nullptr) {
	}

	void receiveApplicationEvent(const yds::ros2::ApplicationEvent& event) {
		++receivedCount_;
		lastEvent_ = event;
		receiverThread_ = QThread::currentThread();
	}

	int receivedCount() const noexcept {
		return receivedCount_;
	}

	const yds::ros2::ApplicationEvent& lastEvent() const noexcept {
		return lastEvent_;
	}

	const QThread* receiverThread() const noexcept {
		return receiverThread_;
	}

private:
	int receivedCount_;
	yds::ros2::ApplicationEvent lastEvent_;
	const QThread* receiverThread_;
};

class TopicReceptionStatusReceiver final : public QObject {
public:
	explicit TopicReceptionStatusReceiver(QObject* parent = nullptr)
		: QObject(parent),
		  receivedCount_(0),
		  lastStatus_({
			  QString(),
			  yds::ros2::TopicReceptionState::kWaiting,
			  QDateTime(),
			  0,
			  QString()}),
		  receiverThread_(nullptr) {
	}

	void receiveTopicReceptionStatus(const yds::ros2::TopicReceptionStatus& status) {
		++receivedCount_;
		lastStatus_ = status;
		statuses_.insert(status.topicName, status);
		receiverThread_ = QThread::currentThread();
	}

	int receivedCount() const noexcept {
		return receivedCount_;
	}

	const yds::ros2::TopicReceptionStatus& lastStatus() const noexcept {
		return lastStatus_;
	}

	bool hasStatus(const QString& topicName) const noexcept {
		return statuses_.contains(topicName);
	}

	yds::ros2::TopicReceptionStatus status(const QString& topicName) const {
		return statuses_.value(topicName);
	}

	const QThread* receiverThread() const noexcept {
		return receiverThread_;
	}

private:
	int receivedCount_;
	yds::ros2::TopicReceptionStatus lastStatus_;
	QHash<QString, yds::ros2::TopicReceptionStatus> statuses_;
	const QThread* receiverThread_;
};

class ComponentStatusReceiver final : public QObject {
public:
	explicit ComponentStatusReceiver(QObject* parent = nullptr)
		: QObject(parent),
		  receivedCount_(0),
		  lastStatus_({
			  QString(),
			  QString(),
			  yds::ros2::ComponentState::kUnknown,
			  0,
			  QString(),
			  QDateTime()}),
		  receiverThread_(nullptr) {
	}

	void receiveComponentStatus(const yds::ros2::ComponentStatus& status) {
		++receivedCount_;
		lastStatus_ = status;
		statuses_.insert(status.topicName, status);
		receiverThread_ = QThread::currentThread();
	}

	int receivedCount() const noexcept {
		return receivedCount_;
	}

	const yds::ros2::ComponentStatus& lastStatus() const noexcept {
		return lastStatus_;
	}

	bool hasStatus(const QString& topicName) const noexcept {
		return statuses_.contains(topicName);
	}

	yds::ros2::ComponentStatus status(const QString& topicName) const {
		return statuses_.value(topicName);
	}

	const QThread* receiverThread() const noexcept {
		return receiverThread_;
	}

private:
	int receivedCount_;
	yds::ros2::ComponentStatus lastStatus_;
	QHash<QString, yds::ros2::ComponentStatus> statuses_;
	const QThread* receiverThread_;
};

bool waitFor(const std::function<bool()>& condition, int timeoutMilliseconds) {
	QElapsedTimer timer;
	timer.start();

	while (!condition() && timer.elapsed() < timeoutMilliseconds) {
		QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
		QThread::msleep(5);
	}
	return condition();
}

TEST(RosQtBridgeTest, DeliversNotificationOnQtThread) {
	ros2qtgui::RosQtBridge bridge;
	HeartbeatReceiver receiver;
	QObject::connect(
		&bridge,
		&ros2qtgui::RosQtBridge::heartbeatUpdated,
		&receiver,
		&HeartbeatReceiver::receiveHeartbeat,
		Qt::QueuedConnection);

	std::thread notifier([&bridge]() {
		bridge.notifyHeartbeat(42);
	});
	notifier.join();

	ASSERT_TRUE(waitFor([&receiver]() {
		return receiver.receivedCount() == 1;
	}, 1000));
	EXPECT_EQ(receiver.heartbeatCount(), 42U);
	EXPECT_EQ(receiver.receiverThread(), QThread::currentThread());
}

TEST(RosQtBridgeTest, DeliversApplicationEventOnQtThread) {
	ros2qtgui::RosQtBridge bridge;
	ApplicationEventReceiver receiver;
	QObject::connect(
		&bridge,
		&ros2qtgui::RosQtBridge::applicationEventOccurred,
		&receiver,
		&ApplicationEventReceiver::receiveApplicationEvent,
		Qt::QueuedConnection);

	const QDateTime timestamp = QDateTime::currentDateTime();
	std::thread notifier([&bridge, timestamp]() {
		bridge.notifyApplicationEvent({
			yds::ros2::ApplicationEventLevel::kWarning,
			timestamp,
			QStringLiteral("Connection retry")});
	});
	notifier.join();

	ASSERT_TRUE(waitFor([&receiver]() {
		return receiver.receivedCount() == 1;
	}, 1000));
	EXPECT_EQ(receiver.lastEvent().level, yds::ros2::ApplicationEventLevel::kWarning);
	EXPECT_EQ(receiver.lastEvent().timestamp, timestamp);
	EXPECT_EQ(receiver.lastEvent().message, QStringLiteral("Connection retry"));
	EXPECT_EQ(receiver.receiverThread(), QThread::currentThread());
}

TEST(RosQtBridgeTest, DeliversTopicReceptionStatusOnQtThread) {
	ros2qtgui::RosQtBridge bridge;
	TopicReceptionStatusReceiver receiver;
	QObject::connect(
		&bridge,
		&ros2qtgui::RosQtBridge::topicReceptionStatusUpdated,
		&receiver,
		&TopicReceptionStatusReceiver::receiveTopicReceptionStatus,
		Qt::QueuedConnection);

	const QDateTime timestamp = QDateTime::currentDateTime();
	std::thread notifier([&bridge, timestamp]() {
		bridge.notifyTopicReceptionStatus({
			QStringLiteral("system_status"),
			yds::ros2::TopicReceptionState::kReceiving,
			timestamp,
			3,
			QStringLiteral("ready")});
	});
	notifier.join();

	ASSERT_TRUE(waitFor([&receiver]() {
		return receiver.receivedCount() == 1;
	}, 1000));
	EXPECT_EQ(receiver.lastStatus().topicName, QStringLiteral("system_status"));
	EXPECT_EQ(receiver.lastStatus().state, yds::ros2::TopicReceptionState::kReceiving);
	EXPECT_EQ(receiver.lastStatus().lastReceivedAt, timestamp);
	EXPECT_EQ(receiver.lastStatus().receivedCount, 3U);
	EXPECT_EQ(receiver.lastStatus().lastMessage, QStringLiteral("ready"));
	EXPECT_EQ(receiver.receiverThread(), QThread::currentThread());
}

TEST(RosQtBridgeTest, DeliversComponentStatusOnQtThread) {
	ros2qtgui::RosQtBridge bridge;
	ComponentStatusReceiver receiver;
	QObject::connect(
		&bridge,
		&ros2qtgui::RosQtBridge::componentStatusUpdated,
		&receiver,
		&ComponentStatusReceiver::receiveComponentStatus,
		Qt::QueuedConnection);

	const QDateTime timestamp = QDateTime::currentDateTime();
	std::thread notifier([&bridge, timestamp]() {
		bridge.notifyComponentStatus({
			QStringLiteral("camera/status"),
			QStringLiteral("camera-1"),
			yds::ros2::ComponentState::kRunning,
			0,
			QStringLiteral("画像を取得しています"),
			timestamp});
	});
	notifier.join();

	ASSERT_TRUE(waitFor([&receiver]() {
		return receiver.receivedCount() == 1;
	}, 1000));
	EXPECT_EQ(receiver.lastStatus().componentId, QStringLiteral("camera-1"));
	EXPECT_EQ(receiver.lastStatus().state, yds::ros2::ComponentState::kRunning);
	EXPECT_EQ(receiver.lastStatus().message, QStringLiteral("画像を取得しています"));
	EXPECT_EQ(receiver.receiverThread(), QThread::currentThread());
}

TEST(MainWindowTest, LimitsApplicationEventLogEntries) {
	ros2qtgui::MainWindow mainWindow(200);
	auto* eventLog = mainWindow.findChild<QPlainTextEdit*>(
		QStringLiteral("applicationEventLog"));
	ASSERT_NE(eventLog, nullptr);

	for (int index = 0; index < 510; ++index) {
		mainWindow.appendApplicationEvent({
			yds::ros2::ApplicationEventLevel::kInfo,
			QDateTime::currentDateTime(),
			QStringLiteral("Event %1").arg(index)});
	}

	EXPECT_EQ(eventLog->document()->blockCount(), 500);
	EXPECT_FALSE(eventLog->toPlainText().contains(QStringLiteral("Event 0\n")));
	EXPECT_TRUE(eventLog->toPlainText().contains(QStringLiteral("Event 509")));
}

TEST(MainWindowTest, DisplaysMultipleTopicStatuses) {
	ros2qtgui::MainWindow mainWindow(200);
	auto* topicStatusTable = mainWindow.findChild<QTableWidget*>(
		QStringLiteral("topicStatusTable"));
	ASSERT_NE(topicStatusTable, nullptr);
	mainWindow.setComponentDisplayName(
		QStringLiteral("camera/status"),
		QStringLiteral("Front camera"));
	mainWindow.setComponentDisplayName(
		QStringLiteral("plc/status"),
		QStringLiteral("Main PLC"));

	mainWindow.setTopicReceptionStatus({
		QStringLiteral("camera/status"),
		yds::ros2::TopicReceptionState::kReceiving,
		QDateTime::currentDateTime(),
		2,
		QStringLiteral("capturing")});
	mainWindow.setTopicReceptionStatus({
		QStringLiteral("plc/status"),
		yds::ros2::TopicReceptionState::kTimedOut,
		QDateTime::currentDateTime(),
		1,
		QStringLiteral("connected")});
	mainWindow.setComponentStatus({
		QStringLiteral("camera/status"),
		QStringLiteral("camera-1"),
		yds::ros2::ComponentState::kRunning,
		0,
		QStringLiteral("capturing"),
		QDateTime::currentDateTime()});
	mainWindow.setComponentStatus({
		QStringLiteral("plc/status"),
		QStringLiteral("plc-1"),
		yds::ros2::ComponentState::kError,
		1001,
		QStringLiteral("connection failed"),
		QDateTime::currentDateTime()});

	ASSERT_EQ(topicStatusTable->rowCount(), 2);
	EXPECT_EQ(topicStatusTable->item(0, 0)->text(), QStringLiteral("Front camera"));
	EXPECT_EQ(topicStatusTable->item(0, 1)->text(), QStringLiteral("camera/status"));
	EXPECT_EQ(topicStatusTable->item(0, 2)->text(), QStringLiteral("camera-1"));
	EXPECT_EQ(topicStatusTable->item(0, 3)->text(), QStringLiteral("RECEIVING"));
	EXPECT_EQ(topicStatusTable->item(0, 4)->text(), QStringLiteral("RUNNING"));
	EXPECT_EQ(topicStatusTable->item(1, 0)->text(), QStringLiteral("Main PLC"));
	EXPECT_EQ(topicStatusTable->item(1, 1)->text(), QStringLiteral("plc/status"));
	EXPECT_EQ(topicStatusTable->item(1, 3)->text(), QStringLiteral("TIMED OUT"));
	EXPECT_EQ(topicStatusTable->item(1, 4)->text(), QStringLiteral("ERROR"));
	EXPECT_EQ(topicStatusTable->item(1, 5)->text(), QStringLiteral("1001"));
	EXPECT_EQ(topicStatusTable->item(1, 8)->text(), QStringLiteral("connection failed"));
	EXPECT_EQ(
		topicStatusTable->item(0, 3)->background().color(),
		QColor(QStringLiteral("#C8E6C9")));
	EXPECT_EQ(
		topicStatusTable->item(1, 3)->background().color(),
		QColor(QStringLiteral("#C62828")));
	EXPECT_EQ(
		topicStatusTable->item(1, 3)->foreground().color(),
		QColor(QStringLiteral("#FFFFFF")));
	EXPECT_EQ(
		topicStatusTable->item(0, 4)->background().color(),
		QColor(QStringLiteral("#C8E6C9")));
	EXPECT_EQ(
		topicStatusTable->item(1, 4)->background().color(),
		QColor(QStringLiteral("#EF9A9A")));
}

TEST(MainWindowTest, AggregatesOverallStatus) {
	ros2qtgui::MainWindow mainWindow(200);
	auto* overallStatusLabel = mainWindow.findChild<QLabel*>(
		QStringLiteral("overallStatusLabel"));
	ASSERT_NE(overallStatusLabel, nullptr);
	EXPECT_EQ(
		overallStatusLabel->text(),
		QStringLiteral("Overall status: WAITING (0/0 receiving)"));

	mainWindow.setComponentDisplayName(
		QStringLiteral("camera/status"),
		QStringLiteral("Camera"));
	mainWindow.setComponentDisplayName(
		QStringLiteral("plc/status"),
		QStringLiteral("PLC"));
	EXPECT_EQ(
		overallStatusLabel->text(),
		QStringLiteral("Overall status: WAITING (0/2 receiving)"));

	mainWindow.setTopicReceptionStatus({
		QStringLiteral("camera/status"),
		yds::ros2::TopicReceptionState::kReceiving,
		QDateTime::currentDateTime(),
		1,
		QString()});
	EXPECT_EQ(
		overallStatusLabel->text(),
		QStringLiteral("Overall status: WARNING (1/2 receiving)"));
	mainWindow.setComponentStatus({
		QStringLiteral("camera/status"),
		QStringLiteral("camera-1"),
		yds::ros2::ComponentState::kRunning,
		0,
		QString(),
		QDateTime::currentDateTime()});
	EXPECT_EQ(
		overallStatusLabel->text(),
		QStringLiteral("Overall status: WAITING (1/2 receiving)"));

	mainWindow.setTopicReceptionStatus({
		QStringLiteral("plc/status"),
		yds::ros2::TopicReceptionState::kReceiving,
		QDateTime::currentDateTime(),
		1,
		QString()});
	mainWindow.setComponentStatus({
		QStringLiteral("plc/status"),
		QStringLiteral("plc-1"),
		yds::ros2::ComponentState::kReady,
		0,
		QString(),
		QDateTime::currentDateTime()});
	EXPECT_EQ(
		overallStatusLabel->text(),
		QStringLiteral("Overall status: NORMAL (2/2 receiving)"));
	EXPECT_TRUE(overallStatusLabel->styleSheet().contains(QStringLiteral("#C8E6C9")));

	mainWindow.setComponentStatus({
		QStringLiteral("camera/status"),
		QStringLiteral("camera-1"),
		yds::ros2::ComponentState::kCritical,
		2001,
		QString(),
		QDateTime::currentDateTime()});
	EXPECT_EQ(
		overallStatusLabel->text(),
		QStringLiteral("Overall status: ERROR (2/2 receiving)"));

	mainWindow.setComponentStatus({
		QStringLiteral("camera/status"),
		QStringLiteral("camera-1"),
		yds::ros2::ComponentState::kWarning,
		1001,
		QString(),
		QDateTime::currentDateTime()});
	EXPECT_EQ(
		overallStatusLabel->text(),
		QStringLiteral("Overall status: WARNING (2/2 receiving)"));
	EXPECT_TRUE(overallStatusLabel->styleSheet().contains(QStringLiteral("#FFE082")));

	mainWindow.setTopicReceptionStatus({
		QStringLiteral("plc/status"),
		yds::ros2::TopicReceptionState::kTimedOut,
		QDateTime::currentDateTime(),
		1,
		QString()});
	EXPECT_EQ(
		overallStatusLabel->text(),
		QStringLiteral("Overall status: ERROR (1/2 receiving)"));
	EXPECT_TRUE(overallStatusLabel->styleSheet().contains(QStringLiteral("#C62828")));

	mainWindow.setTopicReceptionStatus({
		QStringLiteral("plc/status"),
		yds::ros2::TopicReceptionState::kReceiving,
		QDateTime::currentDateTime(),
		2,
		QString()});
	mainWindow.setComponentStatus({
		QStringLiteral("camera/status"),
		QStringLiteral("camera-1"),
		yds::ros2::ComponentState::kStopped,
		0,
		QString(),
		QDateTime::currentDateTime()});
	EXPECT_EQ(
		overallStatusLabel->text(),
		QStringLiteral("Overall status: NORMAL (2/2 receiving)"));
}

TEST(MainWindowTest, UsesColorsForEveryComponentState) {
	struct StateColor {
		yds::ros2::ComponentState state;
		const char* backgroundColor;
		const char* foregroundColor;
	};
	const StateColor stateColors[] = {
		{yds::ros2::ComponentState::kUnknown, "#E0E0E0", "#000000"},
		{yds::ros2::ComponentState::kInitializing, "#BBDEFB", "#000000"},
		{yds::ros2::ComponentState::kReady, "#DCEDC8", "#000000"},
		{yds::ros2::ComponentState::kRunning, "#C8E6C9", "#000000"},
		{yds::ros2::ComponentState::kWarning, "#FFE082", "#000000"},
		{yds::ros2::ComponentState::kError, "#EF9A9A", "#000000"},
		{yds::ros2::ComponentState::kCritical, "#B71C1C", "#FFFFFF"},
		{yds::ros2::ComponentState::kStopped, "#CFD8DC", "#000000"},
	};

	ros2qtgui::MainWindow mainWindow(200);
	auto* topicStatusTable = mainWindow.findChild<QTableWidget*>(
		QStringLiteral("topicStatusTable"));
	ASSERT_NE(topicStatusTable, nullptr);

	for (const auto& stateColor : stateColors) {
		mainWindow.setComponentStatus({
			QStringLiteral("camera/status"),
			QStringLiteral("camera-1"),
			stateColor.state,
			0,
			QString(),
			QDateTime::currentDateTime()});
		const auto* stateItem = topicStatusTable->item(0, 4);
		ASSERT_NE(stateItem, nullptr);
		EXPECT_EQ(
			stateItem->background().color(),
			QColor(QString::fromLatin1(stateColor.backgroundColor)));
		EXPECT_EQ(
			stateItem->foreground().color(),
			QColor(QString::fromLatin1(stateColor.foregroundColor)));
	}
}

TEST(MainWindowTest, UsesColorsForEveryReceptionState) {
	struct StateColor {
		yds::ros2::TopicReceptionState state;
		const char* backgroundColor;
		const char* foregroundColor;
	};
	const StateColor stateColors[] = {
		{yds::ros2::TopicReceptionState::kWaiting, "#E0E0E0", "#000000"},
		{yds::ros2::TopicReceptionState::kReceiving, "#C8E6C9", "#000000"},
		{yds::ros2::TopicReceptionState::kTimedOut, "#C62828", "#FFFFFF"},
	};

	ros2qtgui::MainWindow mainWindow(200);
	auto* topicStatusTable = mainWindow.findChild<QTableWidget*>(
		QStringLiteral("topicStatusTable"));
	ASSERT_NE(topicStatusTable, nullptr);

	for (const auto& stateColor : stateColors) {
		mainWindow.setTopicReceptionStatus({
			QStringLiteral("camera/status"),
			stateColor.state,
			QDateTime(),
			0,
			QString()});
		const auto* stateItem = topicStatusTable->item(0, 3);
		ASSERT_NE(stateItem, nullptr);
		EXPECT_EQ(
			stateItem->background().color(),
			QColor(QString::fromLatin1(stateColor.backgroundColor)));
		EXPECT_EQ(
			stateItem->foreground().color(),
			QColor(QString::fromLatin1(stateColor.foregroundColor)));
	}
}

TEST(RosNodeParameterTest, UsesDefaultValues) {
	auto node = std::make_shared<ros2qtgui::RosNode>([](std::uint64_t) {
	});

	EXPECT_EQ(node->heartbeatIntervalMs(), 1000);
	EXPECT_EQ(node->guiStatusCheckIntervalMs(), 200);
	const auto& configurations = node->componentMonitorConfigurations();
	ASSERT_EQ(configurations.size(), 2U);
	EXPECT_EQ(configurations[0].name, QStringLiteral("camera"));
	EXPECT_EQ(configurations[0].displayName, QStringLiteral("camera"));
	EXPECT_EQ(configurations[0].statusTopicName, QStringLiteral("camera/status"));
	EXPECT_EQ(configurations[0].timeoutMs, 3000);
	EXPECT_EQ(configurations[1].name, QStringLiteral("plc"));
	EXPECT_EQ(configurations[1].displayName, QStringLiteral("plc"));
	EXPECT_EQ(configurations[1].statusTopicName, QStringLiteral("plc/status"));
	EXPECT_EQ(configurations[1].timeoutMs, 5000);

	const auto result = node->set_parameter(rclcpp::Parameter("heartbeat_interval_ms", 500));
	EXPECT_FALSE(result.successful);
}

TEST(RosNodeParameterTest, UsesOverrideValues) {
	rclcpp::NodeOptions options;
	options.parameter_overrides({
		rclcpp::Parameter("heartbeat_interval_ms", 250),
		rclcpp::Parameter("gui_status_check_interval_ms", 100),
		rclcpp::Parameter(
			"component_monitor_names",
			std::vector<std::string>{"camera", "robot"}),
		rclcpp::Parameter("component_monitors.camera.enabled", false),
		rclcpp::Parameter("component_monitors.robot.enabled", true),
		rclcpp::Parameter("component_monitors.robot.display_name", "Robot controller"),
		rclcpp::Parameter("component_monitors.robot.status_topic", "robot/health"),
		rclcpp::Parameter("component_monitors.robot.timeout_ms", 7000),
	});
	auto node = std::make_shared<ros2qtgui::RosNode>(
		[](std::uint64_t) {
		},
		ros2qtgui::RosNode::ApplicationEventCallback(),
		ros2qtgui::RosNode::TopicReceptionStatusCallback(),
		ros2qtgui::RosNode::ComponentStatusCallback(),
		options);

	EXPECT_EQ(node->heartbeatIntervalMs(), 250);
	EXPECT_EQ(node->guiStatusCheckIntervalMs(), 100);
	const auto& configurations = node->componentMonitorConfigurations();
	ASSERT_EQ(configurations.size(), 1U);
	EXPECT_EQ(configurations[0].name, QStringLiteral("robot"));
	EXPECT_EQ(configurations[0].displayName, QStringLiteral("Robot controller"));
	EXPECT_EQ(configurations[0].statusTopicName, QStringLiteral("robot/health"));
	EXPECT_EQ(configurations[0].timeoutMs, 7000);
}

TEST(RosNodeParameterTest, RejectsOutOfRangeValues) {
	rclcpp::NodeOptions options;
	options.parameter_overrides({
		rclcpp::Parameter("heartbeat_interval_ms", 99),
	});

	EXPECT_ANY_THROW({
		auto node = std::make_shared<ros2qtgui::RosNode>(
			[](std::uint64_t) {
			},
			ros2qtgui::RosNode::ApplicationEventCallback(),
			ros2qtgui::RosNode::TopicReceptionStatusCallback(),
			ros2qtgui::RosNode::ComponentStatusCallback(),
			options);
	});
}

TEST(RosNodeParameterTest, RejectsInvalidComponentMonitorParameters) {
	rclcpp::NodeOptions noMonitorNamesOptions;
	noMonitorNamesOptions.parameter_overrides({
		rclcpp::Parameter(
			"component_monitor_names",
			std::vector<std::string>()),
	});
	EXPECT_ANY_THROW({
		auto node = std::make_shared<ros2qtgui::RosNode>(
			[](std::uint64_t) {
			},
			ros2qtgui::RosNode::ApplicationEventCallback(),
			ros2qtgui::RosNode::TopicReceptionStatusCallback(),
			ros2qtgui::RosNode::ComponentStatusCallback(),
			noMonitorNamesOptions);
	});

	rclcpp::NodeOptions invalidMonitorNameOptions;
	invalidMonitorNameOptions.parameter_overrides({
		rclcpp::Parameter(
			"component_monitor_names",
			std::vector<std::string>{"camera.status"}),
	});
	EXPECT_ANY_THROW({
		auto node = std::make_shared<ros2qtgui::RosNode>(
			[](std::uint64_t) {
			},
			ros2qtgui::RosNode::ApplicationEventCallback(),
			ros2qtgui::RosNode::TopicReceptionStatusCallback(),
			ros2qtgui::RosNode::ComponentStatusCallback(),
			invalidMonitorNameOptions);
	});

	rclcpp::NodeOptions duplicateMonitorNameOptions;
	duplicateMonitorNameOptions.parameter_overrides({
		rclcpp::Parameter(
			"component_monitor_names",
			std::vector<std::string>{"camera", "camera"}),
	});
	EXPECT_ANY_THROW({
		auto node = std::make_shared<ros2qtgui::RosNode>(
			[](std::uint64_t) {
			},
			ros2qtgui::RosNode::ApplicationEventCallback(),
			ros2qtgui::RosNode::TopicReceptionStatusCallback(),
			ros2qtgui::RosNode::ComponentStatusCallback(),
			duplicateMonitorNameOptions);
	});

	rclcpp::NodeOptions emptyTopicOptions;
	emptyTopicOptions.parameter_overrides({
		rclcpp::Parameter("component_monitors.camera.status_topic", ""),
	});

	rclcpp::NodeOptions emptyDisplayNameOptions;
	emptyDisplayNameOptions.parameter_overrides({
		rclcpp::Parameter("component_monitors.camera.display_name", ""),
	});
	EXPECT_ANY_THROW({
		auto node = std::make_shared<ros2qtgui::RosNode>(
			[](std::uint64_t) {
			},
			ros2qtgui::RosNode::ApplicationEventCallback(),
			ros2qtgui::RosNode::TopicReceptionStatusCallback(),
			ros2qtgui::RosNode::ComponentStatusCallback(),
			emptyDisplayNameOptions);
	});
	EXPECT_ANY_THROW({
		auto node = std::make_shared<ros2qtgui::RosNode>(
			[](std::uint64_t) {
			},
			ros2qtgui::RosNode::ApplicationEventCallback(),
			ros2qtgui::RosNode::TopicReceptionStatusCallback(),
			ros2qtgui::RosNode::ComponentStatusCallback(),
			emptyTopicOptions);
	});

	rclcpp::NodeOptions duplicateTopicOptions;
	duplicateTopicOptions.parameter_overrides({
		rclcpp::Parameter("component_monitors.plc.status_topic", "camera/status"),
	});
	EXPECT_ANY_THROW({
		auto node = std::make_shared<ros2qtgui::RosNode>(
			[](std::uint64_t) {
			},
			ros2qtgui::RosNode::ApplicationEventCallback(),
			ros2qtgui::RosNode::TopicReceptionStatusCallback(),
			ros2qtgui::RosNode::ComponentStatusCallback(),
			duplicateTopicOptions);
	});

	rclcpp::NodeOptions timeoutOptions;
	timeoutOptions.parameter_overrides({
		rclcpp::Parameter("component_monitors.camera.timeout_ms", 499),
	});
	EXPECT_ANY_THROW({
		auto node = std::make_shared<ros2qtgui::RosNode>(
			[](std::uint64_t) {
			},
			ros2qtgui::RosNode::ApplicationEventCallback(),
			ros2qtgui::RosNode::TopicReceptionStatusCallback(),
			ros2qtgui::RosNode::ComponentStatusCallback(),
			timeoutOptions);
	});

	rclcpp::NodeOptions allDisabledOptions;
	allDisabledOptions.parameter_overrides({
		rclcpp::Parameter("component_monitors.camera.enabled", false),
		rclcpp::Parameter("component_monitors.plc.enabled", false),
	});
	EXPECT_ANY_THROW({
		auto node = std::make_shared<ros2qtgui::RosNode>(
			[](std::uint64_t) {
			},
			ros2qtgui::RosNode::ApplicationEventCallback(),
			ros2qtgui::RosNode::TopicReceptionStatusCallback(),
			ros2qtgui::RosNode::ComponentStatusCallback(),
			allDisabledOptions);
	});
}

TEST(ExecutorRunnerIntegrationTest, DeliversHeartbeatAndStopsSafely) {
	ros2qtgui::RosQtBridge bridge;
	HeartbeatReceiver receiver;
	QObject::connect(
		&bridge,
		&ros2qtgui::RosQtBridge::heartbeatUpdated,
		&receiver,
		&HeartbeatReceiver::receiveHeartbeat,
		Qt::QueuedConnection);

	auto node = std::make_shared<ros2qtgui::RosNode>([&bridge](std::uint64_t count) {
		bridge.notifyHeartbeat(count);
	});
	yds::ros2::ExecutorRunner executorRunner(node);

	ASSERT_TRUE(waitFor([&receiver]() {
		return receiver.receivedCount() > 0;
	}, 2500));
	EXPECT_GE(receiver.heartbeatCount(), 1U);
	EXPECT_EQ(receiver.receiverThread(), QThread::currentThread());

	executorRunner.stop();
	executorRunner.stop();
}

TEST(TopicReceptionIntegrationTest, ReportsIndividualTimeoutAndRecovery) {
	using namespace std::chrono_literals;

	rclcpp::NodeOptions options;
	options.use_intra_process_comms(true);
	options.parameter_overrides({
		rclcpp::Parameter("component_monitors.camera.timeout_ms", 500),
		rclcpp::Parameter("component_monitors.plc.timeout_ms", 1200),
	});

	ros2qtgui::RosQtBridge bridge;
	TopicReceptionStatusReceiver statusReceiver;
	ComponentStatusReceiver componentStatusReceiver;
	ApplicationEventReceiver eventReceiver;
	QObject::connect(
		&bridge,
		&ros2qtgui::RosQtBridge::topicReceptionStatusUpdated,
		&statusReceiver,
		&TopicReceptionStatusReceiver::receiveTopicReceptionStatus,
		Qt::QueuedConnection);
	QObject::connect(
		&bridge,
		&ros2qtgui::RosQtBridge::componentStatusUpdated,
		&componentStatusReceiver,
		&ComponentStatusReceiver::receiveComponentStatus,
		Qt::QueuedConnection);
	QObject::connect(
		&bridge,
		&ros2qtgui::RosQtBridge::applicationEventOccurred,
		&eventReceiver,
		&ApplicationEventReceiver::receiveApplicationEvent,
		Qt::QueuedConnection);

	auto node = std::make_shared<ros2qtgui::RosNode>(
		[](std::uint64_t) {
		},
		[&bridge](const yds::ros2::ApplicationEvent& event) {
			bridge.notifyApplicationEvent(event);
		},
		[&bridge](const yds::ros2::TopicReceptionStatus& status) {
			bridge.notifyTopicReceptionStatus(status);
		},
		[&bridge](const yds::ros2::ComponentStatus& status) {
			bridge.notifyComponentStatus(status);
		},
		options);
	auto cameraPublisher =
		node->create_publisher<yds_interfaces::msg::ComponentStatus>("camera/status", 10);
	auto plcPublisher =
		node->create_publisher<yds_interfaces::msg::ComponentStatus>("plc/status", 10);
	auto plcKeepAliveTimer = node->create_wall_timer(100ms, [plcPublisher]() {
		yds_interfaces::msg::ComponentStatus plcMessage;
		plcMessage.component_id = "plc-1";
		plcMessage.state = yds_interfaces::msg::ComponentStatus::STATE_READY;
		plcMessage.message = "connected";
		plcPublisher->publish(plcMessage);
	});
	yds::ros2::ExecutorRunner executorRunner(node);

	yds_interfaces::msg::ComponentStatus message;
	message.component_id = "camera-1";
	message.state = yds_interfaces::msg::ComponentStatus::STATE_READY;
	for (int index = 0; index < 10; ++index) {
		message.message = "ready-" + std::to_string(index);
		cameraPublisher->publish(message);
	}

	ASSERT_TRUE(waitFor([&statusReceiver]() {
		return statusReceiver.hasStatus(QStringLiteral("camera/status")) &&
			statusReceiver.hasStatus(QStringLiteral("plc/status")) &&
			statusReceiver.status(QStringLiteral("camera/status")).state ==
				yds::ros2::TopicReceptionState::kReceiving &&
			statusReceiver.status(QStringLiteral("camera/status")).receivedCount == 10 &&
			statusReceiver.status(QStringLiteral("plc/status")).state ==
				yds::ros2::TopicReceptionState::kReceiving;
	}, 1500));
	EXPECT_EQ(
		statusReceiver.status(QStringLiteral("camera/status")).lastMessage,
		QStringLiteral("ready-9"));
	EXPECT_EQ(statusReceiver.receiverThread(), QThread::currentThread());
	ASSERT_TRUE(componentStatusReceiver.hasStatus(QStringLiteral("camera/status")));
	EXPECT_EQ(
		componentStatusReceiver.status(QStringLiteral("camera/status")).state,
		yds::ros2::ComponentState::kReady);
	EXPECT_EQ(
		componentStatusReceiver.status(QStringLiteral("camera/status")).componentId,
		QStringLiteral("camera-1"));
	EXPECT_EQ(componentStatusReceiver.receiverThread(), QThread::currentThread());

	ASSERT_TRUE(waitFor([&statusReceiver]() {
		return statusReceiver.status(QStringLiteral("camera/status")).state ==
			yds::ros2::TopicReceptionState::kTimedOut;
	}, 1500));
	EXPECT_EQ(
		statusReceiver.status(QStringLiteral("plc/status")).state,
		yds::ros2::TopicReceptionState::kReceiving);
	EXPECT_EQ(eventReceiver.lastEvent().level, yds::ros2::ApplicationEventLevel::kWarning);

	message.state = yds_interfaces::msg::ComponentStatus::STATE_RUNNING;
	message.message = "running";
	cameraPublisher->publish(message);
	ASSERT_TRUE(waitFor([&statusReceiver]() {
		return statusReceiver.status(QStringLiteral("camera/status")).state ==
				yds::ros2::TopicReceptionState::kReceiving &&
			statusReceiver.status(QStringLiteral("camera/status")).receivedCount == 11;
	}, 1500));
	EXPECT_EQ(
		statusReceiver.status(QStringLiteral("camera/status")).lastMessage,
		QStringLiteral("running"));
	ASSERT_TRUE(waitFor([&componentStatusReceiver]() {
		return componentStatusReceiver.status(QStringLiteral("camera/status")).state ==
			yds::ros2::ComponentState::kRunning;
	}, 1000));
	EXPECT_EQ(eventReceiver.lastEvent().level, yds::ros2::ApplicationEventLevel::kInfo);

	message.state = yds_interfaces::msg::ComponentStatus::STATE_CRITICAL;
	message.error_code = 2001;
	message.message = "emergency stop required";
	cameraPublisher->publish(message);
	ASSERT_TRUE(waitFor([&eventReceiver]() {
		return eventReceiver.lastEvent().level ==
			yds::ros2::ApplicationEventLevel::kCritical;
	}, 1000));
	ASSERT_TRUE(waitFor([&componentStatusReceiver]() {
		const auto status =
			componentStatusReceiver.status(QStringLiteral("camera/status"));
		return status.state == yds::ros2::ComponentState::kCritical &&
			status.errorCode == 2001;
	}, 1000));

	executorRunner.stop();
}

}  // namespace

int main(int argc, char* argv[]) {
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QApplication application(argc, argv);
	testing::InitGoogleTest(&argc, argv);

	int rosArgumentCount = 0;
	char** rosArguments = nullptr;
	rclcpp::init(rosArgumentCount, rosArguments);
	const int result = RUN_ALL_TESTS();
	rclcpp::shutdown();
	return result;
}
