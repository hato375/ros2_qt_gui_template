#include "main_window.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QColor>
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

void applyReceptionStateStyle(
	QTableWidgetItem* item,
	yds::ros2::TopicReceptionState state) noexcept {
	switch (state) {
	case yds::ros2::TopicReceptionState::kWaiting:
		item->setBackground(QColor(QStringLiteral("#E0E0E0")));
		item->setForeground(QColor(QStringLiteral("#000000")));
		return;
	case yds::ros2::TopicReceptionState::kReceiving:
		item->setBackground(QColor(QStringLiteral("#C8E6C9")));
		item->setForeground(QColor(QStringLiteral("#000000")));
		return;
	case yds::ros2::TopicReceptionState::kTimedOut:
		item->setBackground(QColor(QStringLiteral("#C62828")));
		item->setForeground(QColor(QStringLiteral("#FFFFFF")));
		return;
	}
}

void applyEquipmentStateStyle(
	QTableWidgetItem* item,
	yds::ros2::EquipmentState state) noexcept {
	item->setForeground(QColor(QStringLiteral("#000000")));
	switch (state) {
	case yds::ros2::EquipmentState::kUnknown:
		item->setBackground(QColor(QStringLiteral("#E0E0E0")));
		return;
	case yds::ros2::EquipmentState::kInitializing:
		item->setBackground(QColor(QStringLiteral("#BBDEFB")));
		return;
	case yds::ros2::EquipmentState::kReady:
		item->setBackground(QColor(QStringLiteral("#DCEDC8")));
		return;
	case yds::ros2::EquipmentState::kRunning:
		item->setBackground(QColor(QStringLiteral("#C8E6C9")));
		return;
	case yds::ros2::EquipmentState::kWarning:
		item->setBackground(QColor(QStringLiteral("#FFE082")));
		return;
	case yds::ros2::EquipmentState::kError:
		item->setBackground(QColor(QStringLiteral("#EF9A9A")));
		return;
	case yds::ros2::EquipmentState::kCritical:
		item->setBackground(QColor(QStringLiteral("#B71C1C")));
		item->setForeground(QColor(QStringLiteral("#FFFFFF")));
		return;
	case yds::ros2::EquipmentState::kStopped:
		item->setBackground(QColor(QStringLiteral("#CFD8DC")));
		return;
	}
}

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
	auto* receptionStateItem = topicStatusTable_->item(targetRow, kReceptionStateColumn);
	receptionStateItem->setText(yds::ros2::topicReceptionStateText(status.state));
	applyReceptionStateStyle(receptionStateItem, status.state);
	topicStatusTable_->item(targetRow, kLastReceivedAtColumn)->setText(lastReceivedAt);
	topicStatusTable_->item(targetRow, kReceivedCountColumn)->setText(
		QString::number(status.receivedCount));
}

void MainWindow::setEquipmentStatus(
	const yds::ros2::EquipmentStatus& status) noexcept {
	const int targetRow = findOrCreateTopicRow(status.topicName);
	topicStatusTable_->item(targetRow, kEquipmentIdColumn)->setText(status.equipmentId);
	auto* equipmentStateItem = topicStatusTable_->item(targetRow, kEquipmentStateColumn);
	equipmentStateItem->setText(yds::ros2::equipmentStateText(status.state));
	applyEquipmentStateStyle(equipmentStateItem, status.state);
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
	auto* receptionStateItem = topicStatusTable_->item(targetRow, kReceptionStateColumn);
	receptionStateItem->setText(yds::ros2::topicReceptionStateText(
		yds::ros2::TopicReceptionState::kWaiting));
	applyReceptionStateStyle(
		receptionStateItem,
		yds::ros2::TopicReceptionState::kWaiting);
	auto* equipmentStateItem = topicStatusTable_->item(targetRow, kEquipmentStateColumn);
	equipmentStateItem->setText(
		yds::ros2::equipmentStateText(yds::ros2::EquipmentState::kUnknown));
	applyEquipmentStateStyle(equipmentStateItem, yds::ros2::EquipmentState::kUnknown);
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
