#include <yds/ros2/widgets/component_monitor_dialog.h>

#include <memory>

#include <QColor>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>

#include "ui_component_monitor_dialog.h"

namespace yds::ros2::widgets {

namespace {

constexpr int kDisplayNameColumn = 0;
constexpr int kTopicColumn = 1;
constexpr int kComponentIdColumn = 2;
constexpr int kReceptionStateColumn = 3;
constexpr int kComponentStateColumn = 4;
constexpr int kErrorCodeColumn = 5;
constexpr int kLastReceivedAtColumn = 6;
constexpr int kReceivedCountColumn = 7;
constexpr int kMessageColumn = 8;

class TableSortingGuard final {
public:
	explicit TableSortingGuard(QTableWidget* table) noexcept
		: table_(table),
		  sortingEnabled_(table->isSortingEnabled()),
		  sortColumn_(table->horizontalHeader()->sortIndicatorSection()),
		  sortOrder_(table->horizontalHeader()->sortIndicatorOrder()) {
		if (sortingEnabled_) {
			table_->setSortingEnabled(false);
		}
	}

	~TableSortingGuard() {
		if (sortingEnabled_) {
			table_->setSortingEnabled(true);
			table_->sortItems(sortColumn_, sortOrder_);
		}
	}

	TableSortingGuard(const TableSortingGuard&) = delete;
	TableSortingGuard& operator=(const TableSortingGuard&) = delete;

private:
	QTableWidget* table_;
	bool sortingEnabled_;
	int sortColumn_;
	Qt::SortOrder sortOrder_;
};

void applyReceptionStateStyle(
	QTableWidgetItem* item,
	TopicReceptionState state) noexcept {
	switch (state) {
	case TopicReceptionState::kWaiting:
		item->setBackground(QColor(QStringLiteral("#E0E0E0")));
		item->setForeground(QColor(QStringLiteral("#000000")));
		return;
	case TopicReceptionState::kReceiving:
		item->setBackground(QColor(QStringLiteral("#C8E6C9")));
		item->setForeground(QColor(QStringLiteral("#000000")));
		return;
	case TopicReceptionState::kTimedOut:
		item->setBackground(QColor(QStringLiteral("#C62828")));
		item->setForeground(QColor(QStringLiteral("#FFFFFF")));
		return;
	}
}

void applyComponentStateStyle(QTableWidgetItem* item, ComponentState state) noexcept {
	item->setForeground(QColor(QStringLiteral("#000000")));
	switch (state) {
	case ComponentState::kUnknown:
		item->setBackground(QColor(QStringLiteral("#E0E0E0")));
		return;
	case ComponentState::kInitializing:
		item->setBackground(QColor(QStringLiteral("#BBDEFB")));
		return;
	case ComponentState::kReady:
		item->setBackground(QColor(QStringLiteral("#DCEDC8")));
		return;
	case ComponentState::kRunning:
		item->setBackground(QColor(QStringLiteral("#C8E6C9")));
		return;
	case ComponentState::kWarning:
		item->setBackground(QColor(QStringLiteral("#FFE082")));
		return;
	case ComponentState::kError:
		item->setBackground(QColor(QStringLiteral("#EF9A9A")));
		return;
	case ComponentState::kCritical:
		item->setBackground(QColor(QStringLiteral("#B71C1C")));
		item->setForeground(QColor(QStringLiteral("#FFFFFF")));
		return;
	case ComponentState::kStopped:
		item->setBackground(QColor(QStringLiteral("#CFD8DC")));
		return;
	}
}

}  // namespace

ComponentMonitorDialog::ComponentMonitorDialog(QWidget* parent)
	: QDialog(parent),
	  ui_(std::make_unique<Ui::ComponentMonitorDialog>()),
	  receptionStates_(),
	  componentStates_() {
	ui_->setupUi(this);
	auto* header = ui_->topicStatusTable->horizontalHeader();
	header->setMinimumSectionSize(70);
	header->setSectionResizeMode(kDisplayNameColumn, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(kTopicColumn, QHeaderView::Interactive);
	header->setSectionResizeMode(kComponentIdColumn, QHeaderView::Interactive);
	header->setSectionResizeMode(kReceptionStateColumn, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(kComponentStateColumn, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(kErrorCodeColumn, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(kLastReceivedAtColumn, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(kReceivedCountColumn, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(kMessageColumn, QHeaderView::Stretch);
	header->resizeSection(kTopicColumn, 180);
	header->resizeSection(kComponentIdColumn, 160);
	ui_->topicStatusTable->setSortingEnabled(true);
	ui_->topicStatusTable->sortItems(kDisplayNameColumn, Qt::AscendingOrder);
	connect(ui_->filterLineEdit, &QLineEdit::textChanged, this, [this]() {
		updateRowVisibility();
	});
	connect(ui_->showAttentionOnlyCheckBox, &QCheckBox::toggled, this, [this]() {
		updateRowVisibility();
	});
	updateOverallStatus();
}

ComponentMonitorDialog::~ComponentMonitorDialog() = default;

void ComponentMonitorDialog::setComponentDisplayName(
	const QString& topicName,
	const QString& displayName) noexcept {
	{
		TableSortingGuard sortingGuard(ui_->topicStatusTable);
		const int targetRow = findOrCreateTopicRow(topicName);
		ui_->topicStatusTable->item(targetRow, kDisplayNameColumn)->setText(displayName);
	}
	updateRowVisibility();
}

void ComponentMonitorDialog::setTopicReceptionStatus(
	const TopicReceptionStatus& status) noexcept {
	{
		TableSortingGuard sortingGuard(ui_->topicStatusTable);
		const int targetRow = findOrCreateTopicRow(status.topicName);
		const QString lastReceivedAt = status.lastReceivedAt.isValid()
			? status.lastReceivedAt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))
			: tr("not received");
		auto* receptionStateItem = ui_->topicStatusTable->item(targetRow, kReceptionStateColumn);
		receptionStateItem->setText(topicReceptionStateText(status.state));
		applyReceptionStateStyle(receptionStateItem, status.state);
		ui_->topicStatusTable->item(targetRow, kLastReceivedAtColumn)->setText(lastReceivedAt);
		ui_->topicStatusTable->item(targetRow, kReceivedCountColumn)->setText(
			QString::number(status.receivedCount));
		receptionStates_.insert(status.topicName, status.state);
	}
	updateOverallStatus();
	updateRowVisibility();
}

void ComponentMonitorDialog::setComponentStatus(const ComponentStatus& status) noexcept {
	{
		TableSortingGuard sortingGuard(ui_->topicStatusTable);
		const int targetRow = findOrCreateTopicRow(status.topicName);
		ui_->topicStatusTable->item(targetRow, kComponentIdColumn)->setText(status.componentId);
		auto* componentStateItem = ui_->topicStatusTable->item(targetRow, kComponentStateColumn);
		componentStateItem->setText(componentStateText(status.state));
		applyComponentStateStyle(componentStateItem, status.state);
		ui_->topicStatusTable->item(targetRow, kErrorCodeColumn)->setText(
			QString::number(status.errorCode));
		ui_->topicStatusTable->item(targetRow, kMessageColumn)->setText(status.message);
		componentStates_.insert(status.topicName, status.state);
	}
	updateOverallStatus();
	updateRowVisibility();
}

int ComponentMonitorDialog::findOrCreateTopicRow(const QString& topicName) noexcept {
	for (int row = 0; row < ui_->topicStatusTable->rowCount(); ++row) {
		const auto* topicItem = ui_->topicStatusTable->item(row, kTopicColumn);
		if (topicItem && topicItem->text() == topicName) {
			return row;
		}
	}

	const int targetRow = ui_->topicStatusTable->rowCount();
	ui_->topicStatusTable->insertRow(targetRow);
	for (int column = 0; column < ui_->topicStatusTable->columnCount(); ++column) {
		ui_->topicStatusTable->setItem(targetRow, column, new QTableWidgetItem());
	}
	ui_->topicStatusTable->item(targetRow, kTopicColumn)->setText(topicName);
	auto* receptionStateItem = ui_->topicStatusTable->item(targetRow, kReceptionStateColumn);
	receptionStateItem->setText(topicReceptionStateText(TopicReceptionState::kWaiting));
	applyReceptionStateStyle(receptionStateItem, TopicReceptionState::kWaiting);
	auto* componentStateItem = ui_->topicStatusTable->item(targetRow, kComponentStateColumn);
	componentStateItem->setText(componentStateText(ComponentState::kUnknown));
	applyComponentStateStyle(componentStateItem, ComponentState::kUnknown);
	ui_->topicStatusTable->item(targetRow, kErrorCodeColumn)->setText(QStringLiteral("0"));
	receptionStates_.insert(topicName, TopicReceptionState::kWaiting);
	componentStates_.insert(topicName, ComponentState::kUnknown);
	updateOverallStatus();
	return targetRow;
}

bool ComponentMonitorDialog::isAttentionRequired(const QString& topicName) const noexcept {
	const TopicReceptionState receptionState = receptionStates_.value(
		topicName,
		TopicReceptionState::kWaiting);
	if (receptionState != TopicReceptionState::kReceiving) {
		return true;
	}

	const ComponentState componentState = componentStates_.value(
		topicName,
		ComponentState::kUnknown);
	return componentState == ComponentState::kUnknown ||
		componentState == ComponentState::kWarning ||
		componentState == ComponentState::kError ||
		componentState == ComponentState::kCritical;
}

void ComponentMonitorDialog::updateOverallStatus() noexcept {
	OverallStatus overallStatus = OverallStatus::kNormal;
	int receivingCount = 0;
	bool hasWaiting = receptionStates_.isEmpty();
	bool hasWarning = false;
	bool hasError = false;

	for (auto iterator = receptionStates_.cbegin(); iterator != receptionStates_.cend(); ++iterator) {
		const TopicReceptionState receptionState = iterator.value();
		const ComponentState componentState = componentStates_.value(
			iterator.key(),
			ComponentState::kUnknown);
		if (receptionState == TopicReceptionState::kReceiving) {
			++receivingCount;
		} else if (receptionState == TopicReceptionState::kWaiting) {
			hasWaiting = true;
		} else {
			hasError = true;
		}

		if (componentState == ComponentState::kError ||
			componentState == ComponentState::kCritical) {
			hasError = true;
		} else if (receptionState == TopicReceptionState::kReceiving &&
			(componentState == ComponentState::kWarning ||
			 componentState == ComponentState::kUnknown)) {
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

	ui_->overallStatusLabel->setText(
		tr("全体状態: %1（受信中 %2/%3）")
			.arg(overallStatusText(overallStatus))
			.arg(receivingCount)
			.arg(receptionStates_.size()));
	ui_->overallStatusLabel->setStyleSheet(overallStatusStyleSheet(overallStatus));
	emit overallStatusChanged(overallStatus, receivingCount, receptionStates_.size());
}

void ComponentMonitorDialog::updateRowVisibility() noexcept {
	const QString filterText = ui_->filterLineEdit->text().trimmed();
	const bool showAttentionOnly = ui_->showAttentionOnlyCheckBox->isChecked();
	for (int row = 0; row < ui_->topicStatusTable->rowCount(); ++row) {
		const auto* topicItem = ui_->topicStatusTable->item(row, kTopicColumn);
		const QString topicName = topicItem ? topicItem->text() : QString();
		bool matchesText = filterText.isEmpty();
		if (!matchesText) {
			for (int column = 0; column < ui_->topicStatusTable->columnCount(); ++column) {
				const auto* item = ui_->topicStatusTable->item(row, column);
				if (item && item->text().contains(filterText, Qt::CaseInsensitive)) {
					matchesText = true;
					break;
				}
			}
		}
		const bool matchesAttention = !showAttentionOnly || isAttentionRequired(topicName);
		ui_->topicStatusTable->setRowHidden(row, !matchesText || !matchesAttention);
	}
}

QString overallStatusText(ComponentMonitorDialog::OverallStatus status) {
	switch (status) {
	case ComponentMonitorDialog::OverallStatus::kWaiting:
		return QStringLiteral("待機中");
	case ComponentMonitorDialog::OverallStatus::kNormal:
		return QStringLiteral("正常");
	case ComponentMonitorDialog::OverallStatus::kWarning:
		return QStringLiteral("警告");
	case ComponentMonitorDialog::OverallStatus::kError:
		return QStringLiteral("異常");
	}
	return QStringLiteral("待機中");
}

QString overallStatusStyleSheet(ComponentMonitorDialog::OverallStatus status) {
	switch (status) {
	case ComponentMonitorDialog::OverallStatus::kWaiting:
		return QStringLiteral("background-color: #E0E0E0; color: #000000;");
	case ComponentMonitorDialog::OverallStatus::kNormal:
		return QStringLiteral("background-color: #C8E6C9; color: #000000;");
	case ComponentMonitorDialog::OverallStatus::kWarning:
		return QStringLiteral("background-color: #FFE082; color: #000000;");
	case ComponentMonitorDialog::OverallStatus::kError:
		return QStringLiteral("background-color: #C62828; color: #FFFFFF;");
	}
	return QStringLiteral("background-color: #E0E0E0; color: #000000;");
}

}  // namespace yds::ros2::widgets
