#include "main_window.h"

#include <QApplication>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
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
	  overallStatusLabel_(new QLabel(this)),
	  heartbeatLabel_(new QLabel(this)),
	  showComponentMonitorButton_(new QPushButton(tr("Show component monitor"), this)),
	  eventLog_(new QPlainTextEdit(this)),
	  statusTimer_(new QTimer(this)),
	  componentMonitorDialog_(new yds::ros2::widgets::ComponentMonitorDialog(this)) {
	setWindowTitle(tr("ROS 2 + Qt GUI"));
	resize(720, 420);

	auto* centralWidget = new QWidget(this);
	auto* layout = new QVBoxLayout(centralWidget);
	layout->addWidget(new QLabel(tr("ROS 2 and Qt are connected."), centralWidget));
	layout->addWidget(statusLabel_);
	overallStatusLabel_->setObjectName(QStringLiteral("overallStatusLabel"));
	layout->addWidget(overallStatusLabel_);
	layout->addWidget(heartbeatLabel_);
	showComponentMonitorButton_->setObjectName(QStringLiteral("showComponentMonitorButton"));
	layout->addWidget(showComponentMonitorButton_);
	layout->addWidget(new QLabel(tr("Application events"), centralWidget));
	eventLog_->setObjectName(QStringLiteral("applicationEventLog"));
	eventLog_->setReadOnly(true);
	eventLog_->setMaximumBlockCount(kMaximumEventLogEntries);
	layout->addWidget(eventLog_);
	setCentralWidget(centralWidget);

	connect(showComponentMonitorButton_, &QPushButton::clicked, this, [this]() {
		componentMonitorDialog_->show();
		componentMonitorDialog_->raise();
		componentMonitorDialog_->activateWindow();
	});
	connect(
		componentMonitorDialog_,
		&yds::ros2::widgets::ComponentMonitorDialog::overallStatusChanged,
		this,
		&MainWindow::setOverallStatus);
	connect(statusTimer_, &QTimer::timeout, this, [this]() {
		updateRosStatus();
	});
	statusTimer_->start(statusCheckIntervalMs);
	setHeartbeatCount(0);
	setOverallStatus(
		yds::ros2::widgets::ComponentMonitorDialog::OverallStatus::kWaiting,
		0,
		0);
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

yds::ros2::widgets::ComponentMonitorDialog& MainWindow::componentMonitorDialog() noexcept {
	return *componentMonitorDialog_;
}

void MainWindow::setComponentDisplayName(
	const QString& topicName,
	const QString& displayName) noexcept {
	componentMonitorDialog_->setComponentDisplayName(topicName, displayName);
}

void MainWindow::setTopicReceptionStatus(
	const yds::ros2::TopicReceptionStatus& status) noexcept {
	componentMonitorDialog_->setTopicReceptionStatus(status);
}

void MainWindow::setComponentStatus(const yds::ros2::ComponentStatus& status) noexcept {
	componentMonitorDialog_->setComponentStatus(status);
}

void MainWindow::setOverallStatus(
	yds::ros2::widgets::ComponentMonitorDialog::OverallStatus status,
	int receivingCount,
	int totalCount) noexcept {
	overallStatusLabel_->setText(
		tr("全体状態: %1（受信中 %2/%3）")
			.arg(yds::ros2::widgets::overallStatusText(status))
			.arg(receivingCount)
			.arg(totalCount));
	overallStatusLabel_->setStyleSheet(yds::ros2::widgets::overallStatusStyleSheet(status));
}

void MainWindow::updateRosStatus() noexcept {
	if (!rclcpp::ok()) {
		statusLabel_->setText(tr("ROS 2 status: stopped"));
		QApplication::quit();
		return;
	}
}

}  // namespace ros2qtgui
