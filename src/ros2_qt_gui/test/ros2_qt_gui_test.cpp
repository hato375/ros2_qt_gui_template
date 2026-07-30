#include <cstdint>
#include <functional>
#include <memory>
#include <thread>

#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QObject>
#include <QPlainTextEdit>
#include <QThread>

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>

#include "application_event.h"
#include "main_window.h"
#include "ros_executor_runner.h"
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
		  receiverThread_(nullptr) {
	}

	void receiveApplicationEvent(const ros2qtgui::ApplicationEvent& event) {
		++receivedCount_;
		lastEvent_ = event;
		receiverThread_ = QThread::currentThread();
	}

	int receivedCount() const noexcept {
		return receivedCount_;
	}

	const ros2qtgui::ApplicationEvent& lastEvent() const noexcept {
		return lastEvent_;
	}

	const QThread* receiverThread() const noexcept {
		return receiverThread_;
	}

private:
	int receivedCount_;
	ros2qtgui::ApplicationEvent lastEvent_;
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
			ros2qtgui::ApplicationEventLevel::kWarning,
			timestamp,
			QStringLiteral("Connection retry")});
	});
	notifier.join();

	ASSERT_TRUE(waitFor([&receiver]() {
		return receiver.receivedCount() == 1;
	}, 1000));
	EXPECT_EQ(receiver.lastEvent().level, ros2qtgui::ApplicationEventLevel::kWarning);
	EXPECT_EQ(receiver.lastEvent().timestamp, timestamp);
	EXPECT_EQ(receiver.lastEvent().message, QStringLiteral("Connection retry"));
	EXPECT_EQ(receiver.receiverThread(), QThread::currentThread());
}

TEST(MainWindowTest, LimitsApplicationEventLogEntries) {
	ros2qtgui::MainWindow mainWindow(200);
	auto* eventLog = mainWindow.findChild<QPlainTextEdit*>(
		QStringLiteral("applicationEventLog"));
	ASSERT_NE(eventLog, nullptr);

	for (int index = 0; index < 510; ++index) {
		mainWindow.appendApplicationEvent({
			ros2qtgui::ApplicationEventLevel::kInfo,
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

	const auto result = node->set_parameter(rclcpp::Parameter("heartbeat_interval_ms", 500));
	EXPECT_FALSE(result.successful);
}

TEST(RosNodeParameterTest, UsesOverrideValues) {
	rclcpp::NodeOptions options;
	options.parameter_overrides({
		rclcpp::Parameter("heartbeat_interval_ms", 250),
		rclcpp::Parameter("gui_status_check_interval_ms", 100),
	});
	auto node = std::make_shared<ros2qtgui::RosNode>(
		[](std::uint64_t) {
		},
		ros2qtgui::RosNode::ApplicationEventCallback(),
		options);

	EXPECT_EQ(node->heartbeatIntervalMs(), 250);
	EXPECT_EQ(node->guiStatusCheckIntervalMs(), 100);
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
			options);
	});
}

TEST(RosExecutorRunnerTest, DeliversHeartbeatAndStopsSafely) {
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
	ros2qtgui::RosExecutorRunner executorRunner(node);

	ASSERT_TRUE(waitFor([&receiver]() {
		return receiver.receivedCount() > 0;
	}, 2500));
	EXPECT_GE(receiver.heartbeatCount(), 1U);
	EXPECT_EQ(receiver.receiverThread(), QThread::currentThread());

	executorRunner.stop();
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
