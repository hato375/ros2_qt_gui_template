#include "main_window.h"

#include <QApplication>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <rclcpp/utilities.hpp>

namespace ros2qtgui {

namespace {

constexpr int kMaximumEventLogEntries = 500;

}  // namespace

MainWindow::MainWindow(int statusCheckIntervalMs)
	: statusLabel_(new QLabel(tr("ROS 2 status: running"), this)),
	  heartbeatLabel_(new QLabel(this)),
	  monitoredTopicLabel_(new QLabel(this)),
	  topicReceptionStateLabel_(new QLabel(this)),
	  topicLastReceivedAtLabel_(new QLabel(this)),
	  topicReceivedCountLabel_(new QLabel(this)),
	  topicLastMessageLabel_(new QLabel(this)),
	  eventLog_(new QPlainTextEdit(this)),
	  statusTimer_(new QTimer(this)) {
	setWindowTitle(tr("ROS 2 + Qt GUI"));
	resize(640, 480);

	auto* centralWidget = new QWidget(this);
	auto* layout = new QVBoxLayout(centralWidget);
	layout->addWidget(new QLabel(tr("ROS 2 and Qt are connected."), centralWidget));
	layout->addWidget(statusLabel_);
	layout->addWidget(heartbeatLabel_);
	monitoredTopicLabel_->setObjectName(QStringLiteral("monitoredTopicLabel"));
	topicReceptionStateLabel_->setObjectName(QStringLiteral("topicReceptionStateLabel"));
	topicLastReceivedAtLabel_->setObjectName(QStringLiteral("topicLastReceivedAtLabel"));
	topicReceivedCountLabel_->setObjectName(QStringLiteral("topicReceivedCountLabel"));
	topicLastMessageLabel_->setObjectName(QStringLiteral("topicLastMessageLabel"));
	layout->addWidget(monitoredTopicLabel_);
	layout->addWidget(topicReceptionStateLabel_);
	layout->addWidget(topicLastReceivedAtLabel_);
	layout->addWidget(topicReceivedCountLabel_);
	layout->addWidget(topicLastMessageLabel_);
	layout->addWidget(new QLabel(tr("Application events"), centralWidget));
	eventLog_->setObjectName(QStringLiteral("applicationEventLog"));
	eventLog_->setReadOnly(true);
	eventLog_->setMaximumBlockCount(kMaximumEventLogEntries);
	layout->addWidget(eventLog_);
	setCentralWidget(centralWidget);

	connect(statusTimer_, &QTimer::timeout, this, [this]() {
		updateRosStatus();
	});
	statusTimer_->start(statusCheckIntervalMs);
	setHeartbeatCount(0);
	updateRosStatus();
}

void MainWindow::setHeartbeatCount(quint64 count) noexcept {
	heartbeatLabel_->setText(tr("ROS heartbeat count: %1").arg(count));
}

void MainWindow::appendApplicationEvent(const yds::ros2::ApplicationEvent& event) noexcept {
	const QString timestamp = event.timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
	eventLog_->appendPlainText(
		QStringLiteral("[%1] [%2] %3")
			.arg(timestamp, yds::ros2::eventLevelText(event.level), event.message));
}

void MainWindow::setTopicReceptionStatus(
	const yds::ros2::TopicReceptionStatus& status) noexcept {
	monitoredTopicLabel_->setText(tr("Monitored topic: %1").arg(status.topicName));
	topicReceptionStateLabel_->setText(
		tr("Topic reception state: %1")
			.arg(yds::ros2::topicReceptionStateText(status.state)));
	const QString lastReceivedAt = status.lastReceivedAt.isValid()
		? status.lastReceivedAt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))
		: tr("not received");
	topicLastReceivedAtLabel_->setText(
		tr("Last received at: %1").arg(lastReceivedAt));
	topicReceivedCountLabel_->setText(
		tr("Received count: %1").arg(status.receivedCount));
	topicLastMessageLabel_->setText(
		tr("Last message: %1").arg(status.lastMessage));
}

void MainWindow::updateRosStatus() noexcept {
	if (!rclcpp::ok()) {
		statusLabel_->setText(tr("ROS 2 status: stopped"));
		QApplication::quit();
		return;
	}
}

}  // namespace ros2qtgui
