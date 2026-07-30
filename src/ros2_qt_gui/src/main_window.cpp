#include "main_window.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <rclcpp/utilities.hpp>

namespace ros2qtgui {

namespace {

constexpr int kMaximumEventLogEntries = 500;
constexpr int kTopicColumn = 0;
constexpr int kEquipmentIdColumn = 1;
constexpr int kReceptionStateColumn = 2;
constexpr int kEquipmentStateColumn = 3;
constexpr int kErrorCodeColumn = 4;
constexpr int kLastReceivedAtColumn = 5;
constexpr int kReceivedCountColumn = 6;
constexpr int kMessageColumn = 7;

}  // namespace

MainWindow::MainWindow(int statusCheckIntervalMs)
	: statusLabel_(new QLabel(tr("ROS 2 status: running"), this)),
	  heartbeatLabel_(new QLabel(this)),
	  topicStatusTable_(new QTableWidget(this)),
	  eventLog_(new QPlainTextEdit(this)),
	  statusTimer_(new QTimer(this)) {
	setWindowTitle(tr("ROS 2 + Qt GUI"));
	resize(960, 540);

	auto* centralWidget = new QWidget(this);
	auto* layout = new QVBoxLayout(centralWidget);
	layout->addWidget(new QLabel(tr("ROS 2 and Qt are connected."), centralWidget));
	layout->addWidget(statusLabel_);
	layout->addWidget(heartbeatLabel_);
	layout->addWidget(new QLabel(tr("Monitored topics"), centralWidget));
	topicStatusTable_->setObjectName(QStringLiteral("topicStatusTable"));
	topicStatusTable_->setColumnCount(8);
	topicStatusTable_->setHorizontalHeaderLabels({
		tr("Topic"),
		tr("Equipment ID"),
		tr("Communication"),
		tr("Equipment state"),
		tr("Error code"),
		tr("Last received at"),
		tr("Count"),
		tr("Message"),
	});
	topicStatusTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	topicStatusTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
	layout->addWidget(topicStatusTable_);
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
	const int targetRow = findOrCreateTopicRow(status.topicName);
	const QString lastReceivedAt = status.lastReceivedAt.isValid()
		? status.lastReceivedAt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))
		: tr("not received");
	topicStatusTable_->item(targetRow, kReceptionStateColumn)->setText(
		yds::ros2::topicReceptionStateText(status.state));
	topicStatusTable_->item(targetRow, kLastReceivedAtColumn)->setText(lastReceivedAt);
	topicStatusTable_->item(targetRow, kReceivedCountColumn)->setText(
		QString::number(status.receivedCount));
}

void MainWindow::setEquipmentStatus(
	const yds::ros2::EquipmentStatus& status) noexcept {
	const int targetRow = findOrCreateTopicRow(status.topicName);
	topicStatusTable_->item(targetRow, kEquipmentIdColumn)->setText(status.equipmentId);
	topicStatusTable_->item(targetRow, kEquipmentStateColumn)->setText(
		yds::ros2::equipmentStateText(status.state));
	topicStatusTable_->item(targetRow, kErrorCodeColumn)->setText(
		QString::number(status.errorCode));
	topicStatusTable_->item(targetRow, kMessageColumn)->setText(status.message);
}

int MainWindow::findOrCreateTopicRow(const QString& topicName) noexcept {
	for (int row = 0; row < topicStatusTable_->rowCount(); ++row) {
		const auto* topicItem = topicStatusTable_->item(row, kTopicColumn);
		if (topicItem && topicItem->text() == topicName) {
			return row;
		}
	}

	const int targetRow = topicStatusTable_->rowCount();
	topicStatusTable_->insertRow(targetRow);
	for (int column = 0; column < topicStatusTable_->columnCount(); ++column) {
		topicStatusTable_->setItem(targetRow, column, new QTableWidgetItem());
	}
	topicStatusTable_->item(targetRow, kTopicColumn)->setText(topicName);
	topicStatusTable_->item(targetRow, kEquipmentStateColumn)->setText(
		yds::ros2::equipmentStateText(yds::ros2::EquipmentState::kUnknown));
	topicStatusTable_->item(targetRow, kErrorCodeColumn)->setText(QStringLiteral("0"));
	return targetRow;
}

void MainWindow::updateRosStatus() noexcept {
	if (!rclcpp::ok()) {
		statusLabel_->setText(tr("ROS 2 status: stopped"));
		QApplication::quit();
		return;
	}
}

}  // namespace ros2qtgui
