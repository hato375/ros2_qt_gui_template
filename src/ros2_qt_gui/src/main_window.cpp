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

QString eventLevelText(ApplicationEventLevel level) {
	switch (level) {
	case ApplicationEventLevel::kInfo:
		return QStringLiteral("INFO");
	case ApplicationEventLevel::kWarning:
		return QStringLiteral("WARN");
	case ApplicationEventLevel::kError:
		return QStringLiteral("ERROR");
	}
	return QStringLiteral("UNKNOWN");
}

}  // namespace

MainWindow::MainWindow(int statusCheckIntervalMs)
	: statusLabel_(new QLabel(tr("ROS 2 status: running"), this)),
	  heartbeatLabel_(new QLabel(this)),
	  eventLog_(new QPlainTextEdit(this)),
	  statusTimer_(new QTimer(this)) {
	setWindowTitle(tr("ROS 2 + Qt GUI"));
	resize(640, 360);

	auto* centralWidget = new QWidget(this);
	auto* layout = new QVBoxLayout(centralWidget);
	layout->addWidget(new QLabel(tr("ROS 2 and Qt are connected."), centralWidget));
	layout->addWidget(statusLabel_);
	layout->addWidget(heartbeatLabel_);
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

void MainWindow::appendApplicationEvent(const ApplicationEvent& event) noexcept {
	const QString timestamp = event.timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
	eventLog_->appendPlainText(
		QStringLiteral("[%1] [%2] %3")
			.arg(timestamp, eventLevelText(event.level), event.message));
}

void MainWindow::updateRosStatus() noexcept {
	if (!rclcpp::ok()) {
		statusLabel_->setText(tr("ROS 2 status: stopped"));
		QApplication::quit();
		return;
	}
}

}  // namespace ros2qtgui
