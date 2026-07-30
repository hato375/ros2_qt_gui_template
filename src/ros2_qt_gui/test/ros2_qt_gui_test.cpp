#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QObject>
#include <QPlainTextEdit>
#include <QThread>

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

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
		receiverThread_ = QThread::currentThread();
	}

	int receivedCount() const noexcept {
		return receivedCount_;
	}

	const yds::ros2::TopicReceptionStatus& lastStatus() const noexcept {
		return lastStatus_;
	}

	const QThread* receiverThread() const noexcept {
		return receiverThread_;
	}

private:
	int receivedCount_;
	yds::ros2::TopicReceptionStatus lastStatus_;
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

TEST(RosNodeParameterTest, UsesDefaultValues) {
	auto node = std::make_shared<ros2qtgui::RosNode>([](std::uint64_t) {
	});

	EXPECT_EQ(node->heartbeatIntervalMs(), 1000);
	EXPECT_EQ(node->guiStatusCheckIntervalMs(), 200);
	EXPECT_EQ(node->monitoredTopic(), "system_status");
	EXPECT_EQ(node->topicReceptionTimeoutMs(), 3000);

	const auto result = node->set_parameter(rclcpp::Parameter("heartbeat_interval_ms", 500));
	EXPECT_FALSE(result.successful);
}

TEST(RosNodeParameterTest, UsesOverrideValues) {
	rclcpp::NodeOptions options;
	options.parameter_overrides({
		rclcpp::Parameter("heartbeat_interval_ms", 250),
		rclcpp::Parameter("gui_status_check_interval_ms", 100),
		rclcpp::Parameter("monitored_topic", "equipment_status"),
		rclcpp::Parameter("topic_reception_timeout_ms", 5000),
	});
	auto node = std::make_shared<ros2qtgui::RosNode>(
		[](std::uint64_t) {
		},
		ros2qtgui::RosNode::ApplicationEventCallback(),
		ros2qtgui::RosNode::TopicReceptionStatusCallback(),
		options);

	EXPECT_EQ(node->heartbeatIntervalMs(), 250);
	EXPECT_EQ(node->guiStatusCheckIntervalMs(), 100);
	EXPECT_EQ(node->monitoredTopic(), "equipment_status");
	EXPECT_EQ(node->topicReceptionTimeoutMs(), 5000);
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
			options);
	});
}

TEST(RosNodeParameterTest, RejectsInvalidTopicMonitorParameters) {
	rclcpp::NodeOptions emptyTopicOptions;
	emptyTopicOptions.parameter_overrides({
		rclcpp::Parameter("monitored_topic", ""),
	});
	EXPECT_ANY_THROW({
		auto node = std::make_shared<ros2qtgui::RosNode>(
			[](std::uint64_t) {
			},
			ros2qtgui::RosNode::ApplicationEventCallback(),
			ros2qtgui::RosNode::TopicReceptionStatusCallback(),
			emptyTopicOptions);
	});

	rclcpp::NodeOptions timeoutOptions;
	timeoutOptions.parameter_overrides({
		rclcpp::Parameter("topic_reception_timeout_ms", 499),
	});
	EXPECT_ANY_THROW({
		auto node = std::make_shared<ros2qtgui::RosNode>(
			[](std::uint64_t) {
			},
			ros2qtgui::RosNode::ApplicationEventCallback(),
			ros2qtgui::RosNode::TopicReceptionStatusCallback(),
			timeoutOptions);
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

TEST(TopicReceptionIntegrationTest, ReportsReceptionTimeoutAndRecovery) {
	rclcpp::NodeOptions options;
	options.use_intra_process_comms(true);
	options.parameter_overrides({
		rclcpp::Parameter("topic_reception_timeout_ms", 500),
	});

	ros2qtgui::RosQtBridge bridge;
	TopicReceptionStatusReceiver statusReceiver;
	ApplicationEventReceiver eventReceiver;
	QObject::connect(
		&bridge,
		&ros2qtgui::RosQtBridge::topicReceptionStatusUpdated,
		&statusReceiver,
		&TopicReceptionStatusReceiver::receiveTopicReceptionStatus,
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
		options);
	auto publisher = node->create_publisher<std_msgs::msg::String>("system_status", 10);
	yds::ros2::ExecutorRunner executorRunner(node);

	std_msgs::msg::String message;
	for (int index = 0; index < 10; ++index) {
		message.data = "ready-" + std::to_string(index);
		publisher->publish(message);
	}

	ASSERT_TRUE(waitFor([&statusReceiver]() {
		return statusReceiver.lastStatus().state ==
				yds::ros2::TopicReceptionState::kReceiving &&
			statusReceiver.lastStatus().receivedCount == 10;
	}, 1500));
	EXPECT_EQ(statusReceiver.receivedCount(), 1);
	EXPECT_EQ(statusReceiver.lastStatus().lastMessage, QStringLiteral("ready-9"));
	EXPECT_EQ(statusReceiver.receiverThread(), QThread::currentThread());

	ASSERT_TRUE(waitFor([&statusReceiver]() {
		return statusReceiver.lastStatus().state ==
			yds::ros2::TopicReceptionState::kTimedOut;
	}, 1500));
	EXPECT_EQ(eventReceiver.lastEvent().level, yds::ros2::ApplicationEventLevel::kWarning);

	message.data = "running";
	publisher->publish(message);
	ASSERT_TRUE(waitFor([&statusReceiver]() {
		return statusReceiver.lastStatus().state ==
				yds::ros2::TopicReceptionState::kReceiving &&
			statusReceiver.lastStatus().receivedCount == 11;
	}, 1500));
	EXPECT_EQ(statusReceiver.lastStatus().lastMessage, QStringLiteral("running"));
	EXPECT_EQ(eventReceiver.lastEvent().level, yds::ros2::ApplicationEventLevel::kInfo);

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
