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
constexpr int kDisplayNameColumn = 0;
constexpr int kTopicColumn = 1;
constexpr int kComponentIdColumn = 2;
constexpr int kReceptionStateColumn = 3;
constexpr int kComponentStateColumn = 4;
constexpr int kErrorCodeColumn = 5;
constexpr int kLastReceivedAtColumn = 6;
constexpr int kReceivedCountColumn = 7;
constexpr int kMessageColumn = 8;

enum class OverallStatus {
	kWaiting,
	kNormal,
	kWarning,
	kError,
};

QString overallStatusText(OverallStatus status) {
	switch (status) {
	case OverallStatus::kWaiting:
		return QStringLiteral("WAITING");
	case OverallStatus::kNormal:
		return QStringLiteral("NORMAL");
	case OverallStatus::kWarning:
		return QStringLiteral("WARNING");
	case OverallStatus::kError:
		return QStringLiteral("ERROR");
	}
	return QStringLiteral("WAITING");
}

void applyOverallStatusStyle(QLabel* label, OverallStatus status) noexcept {
	switch (status) {
	case OverallStatus::kWaiting:
		label->setStyleSheet(QStringLiteral("background-color: #E0E0E0; color: #000000;"));
		return;
	case OverallStatus::kNormal:
		label->setStyleSheet(QStringLiteral("background-color: #C8E6C9; color: #000000;"));
		return;
	case OverallStatus::kWarning:
		label->setStyleSheet(QStringLiteral("background-color: #FFE082; color: #000000;"));
		return;
	case OverallStatus::kError:
		label->setStyleSheet(QStringLiteral("background-color: #C62828; color: #FFFFFF;"));
		return;
	}
}

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

void applyComponentStateStyle(
	QTableWidgetItem* item,
	yds::ros2::ComponentState state) noexcept {
	item->setForeground(QColor(QStringLiteral("#000000")));
	switch (state) {
	case yds::ros2::ComponentState::kUnknown:
		item->setBackground(QColor(QStringLiteral("#E0E0E0")));
		return;
	case yds::ros2::ComponentState::kInitializing:
		item->setBackground(QColor(QStringLiteral("#BBDEFB")));
		return;
	case yds::ros2::ComponentState::kReady:
		item->setBackground(QColor(QStringLiteral("#DCEDC8")));
		return;
	case yds::ros2::ComponentState::kRunning:
		item->setBackground(QColor(QStringLiteral("#C8E6C9")));
		return;
	case yds::ros2::ComponentState::kWarning:
		item->setBackground(QColor(QStringLiteral("#FFE082")));
		return;
	case yds::ros2::ComponentState::kError:
		item->setBackground(QColor(QStringLiteral("#EF9A9A")));
		return;
	case yds::ros2::ComponentState::kCritical:
		item->setBackground(QColor(QStringLiteral("#B71C1C")));
		item->setForeground(QColor(QStringLiteral("#FFFFFF")));
		return;
	case yds::ros2::ComponentState::kStopped:
		item->setBackground(QColor(QStringLiteral("#CFD8DC")));
		return;
	}
}

}  // namespace

MainWindow::MainWindow(int statusCheckIntervalMs)
	: statusLabel_(new QLabel(tr("ROS 2 status: running"), this)),
	  overallStatusLabel_(new QLabel(this)),
	  heartbeatLabel_(new QLabel(this)),
	  topicStatusTable_(new QTableWidget(this)),
	  eventLog_(new QPlainTextEdit(this)),
	  statusTimer_(new QTimer(this)) {
	setWindowTitle(tr("ROS 2 + Qt GUI"));
	resize(1200, 540);

	auto* centralWidget = new QWidget(this);
	auto* layout = new QVBoxLayout(centralWidget);
	layout->addWidget(new QLabel(tr("ROS 2 and Qt are connected."), centralWidget));
	layout->addWidget(statusLabel_);
	overallStatusLabel_->setObjectName(QStringLiteral("overallStatusLabel"));
	layout->addWidget(overallStatusLabel_);
	layout->addWidget(heartbeatLabel_);
	layout->addWidget(new QLabel(tr("Monitored components"), centralWidget));
	topicStatusTable_->setObjectName(QStringLiteral("topicStatusTable"));
	topicStatusTable_->setColumnCount(9);
	topicStatusTable_->setHorizontalHeaderLabels({
		tr("Component"),
		tr("Topic"),
		tr("Component ID"),
		tr("Communication"),
		tr("Component state"),
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
	updateOverallStatus();
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

void MainWindow::setComponentDisplayName(
	const QString& topicName,
	const QString& displayName) noexcept {
	const int targetRow = findOrCreateTopicRow(topicName);
	topicStatusTable_->item(targetRow, kDisplayNameColumn)->setText(displayName);
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
	receptionStates_.insert(status.topicName, status.state);
	updateOverallStatus();
}

void MainWindow::setComponentStatus(
	const yds::ros2::ComponentStatus& status) noexcept {
	const int targetRow = findOrCreateTopicRow(status.topicName);
	topicStatusTable_->item(targetRow, kComponentIdColumn)->setText(status.componentId);
	auto* componentStateItem = topicStatusTable_->item(targetRow, kComponentStateColumn);
	componentStateItem->setText(yds::ros2::componentStateText(status.state));
	applyComponentStateStyle(componentStateItem, status.state);
	topicStatusTable_->item(targetRow, kErrorCodeColumn)->setText(
		QString::number(status.errorCode));
	topicStatusTable_->item(targetRow, kMessageColumn)->setText(status.message);
	componentStates_.insert(status.topicName, status.state);
	updateOverallStatus();
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
	auto* componentStateItem = topicStatusTable_->item(targetRow, kComponentStateColumn);
	componentStateItem->setText(
		yds::ros2::componentStateText(yds::ros2::ComponentState::kUnknown));
	applyComponentStateStyle(componentStateItem, yds::ros2::ComponentState::kUnknown);
	topicStatusTable_->item(targetRow, kErrorCodeColumn)->setText(QStringLiteral("0"));
	receptionStates_.insert(topicName, yds::ros2::TopicReceptionState::kWaiting);
	componentStates_.insert(topicName, yds::ros2::ComponentState::kUnknown);
	updateOverallStatus();
	return targetRow;
}

void MainWindow::updateOverallStatus() noexcept {
	OverallStatus overallStatus = OverallStatus::kNormal;
	int receivingCount = 0;
	bool hasWaiting = receptionStates_.isEmpty();
	bool hasWarning = false;
	bool hasError = false;

	for (auto iterator = receptionStates_.cbegin(); iterator != receptionStates_.cend(); ++iterator) {
		const yds::ros2::TopicReceptionState receptionState = iterator.value();
		const yds::ros2::ComponentState componentState = componentStates_.value(
			iterator.key(),
			yds::ros2::ComponentState::kUnknown);
		if (receptionState == yds::ros2::TopicReceptionState::kReceiving) {
			++receivingCount;
		} else if (receptionState == yds::ros2::TopicReceptionState::kWaiting) {
			hasWaiting = true;
		} else {
			hasError = true;
		}

		if (componentState == yds::ros2::ComponentState::kError ||
			componentState == yds::ros2::ComponentState::kCritical) {
			hasError = true;
		} else if (receptionState == yds::ros2::TopicReceptionState::kReceiving &&
			(componentState == yds::ros2::ComponentState::kWarning ||
			 componentState == yds::ros2::ComponentState::kUnknown)) {
			hasWarning = true;
		}
	}

	if (hasError) {
		overallStatus = OverallStatus::kError;
	} else if (hasWarning) {
		overallStatus = OverallStatus::kWarning;
	} else if (hasWaiting) {
		overallStatus = OverallStatus::kWaiting;
	}

	overallStatusLabel_->setText(
		tr("Overall status: %1 (%2/%3 receiving)")
			.arg(overallStatusText(overallStatus))
			.arg(receivingCount)
			.arg(receptionStates_.size()));
	applyOverallStatusStyle(overallStatusLabel_, overallStatus);
}

void MainWindow::updateRosStatus() noexcept {
	if (!rclcpp::ok()) {
		statusLabel_->setText(tr("ROS 2 status: stopped"));
		QApplication::quit();
		return;
	}
}

}  // namespace ros2qtgui
