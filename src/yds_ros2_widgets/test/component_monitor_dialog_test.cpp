#include <QApplication>
#include <QCheckBox>
#include <QColor>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QTableWidget>

#include <gtest/gtest.h>

#include <yds/ros2/widgets/component_monitor_dialog.h>

namespace {

TEST(ComponentMonitorDialogTest, DisplaysMultipleComponentStatuses) {
	yds::ros2::widgets::ComponentMonitorDialog dialog;
	auto* table = dialog.findChild<QTableWidget*>(QStringLiteral("topicStatusTable"));
	ASSERT_NE(table, nullptr);
	EXPECT_EQ(table->horizontalHeaderItem(0)->text(), QStringLiteral("表示名"));
	EXPECT_EQ(table->horizontalHeaderItem(3)->text(), QStringLiteral("通信状態"));
	EXPECT_EQ(table->horizontalHeaderItem(4)->text(), QStringLiteral("動作状態"));
	EXPECT_EQ(table->horizontalHeaderItem(6)->text(), QStringLiteral("最終受信日時"));
	EXPECT_EQ(table->horizontalHeaderItem(7)->text(), QStringLiteral("受信回数"));
	EXPECT_EQ(table->horizontalHeader()->sectionResizeMode(1), QHeaderView::Interactive);
	EXPECT_EQ(table->horizontalHeader()->sectionResizeMode(8), QHeaderView::Stretch);

	dialog.setComponentDisplayName(QStringLiteral("camera/status"), QStringLiteral("Front camera"));
	dialog.setComponentDisplayName(QStringLiteral("plc/status"), QStringLiteral("Main PLC"));
	dialog.setTopicReceptionStatus({
		QStringLiteral("camera/status"),
		yds::ros2::TopicReceptionState::kReceiving,
		QDateTime::currentDateTime(),
		2,
		QStringLiteral("capturing")});
	dialog.setTopicReceptionStatus({
		QStringLiteral("plc/status"),
		yds::ros2::TopicReceptionState::kTimedOut,
		QDateTime::currentDateTime(),
		1,
		QStringLiteral("connected")});
	dialog.setComponentStatus({
		QStringLiteral("camera/status"),
		QStringLiteral("camera-1"),
		yds::ros2::ComponentState::kRunning,
		0,
		QStringLiteral("capturing"),
		QDateTime::currentDateTime()});
	dialog.setComponentStatus({
		QStringLiteral("plc/status"),
		QStringLiteral("plc-1"),
		yds::ros2::ComponentState::kError,
		1001,
		QStringLiteral("connection failed"),
		QDateTime::currentDateTime()});

	ASSERT_EQ(table->rowCount(), 2);
	EXPECT_EQ(table->item(0, 0)->text(), QStringLiteral("Front camera"));
	EXPECT_EQ(table->item(0, 1)->text(), QStringLiteral("camera/status"));
	EXPECT_EQ(table->item(0, 2)->text(), QStringLiteral("camera-1"));
	EXPECT_EQ(table->item(0, 3)->text(), QStringLiteral("RECEIVING"));
	EXPECT_EQ(table->item(0, 4)->text(), QStringLiteral("RUNNING"));
	EXPECT_EQ(table->item(1, 3)->text(), QStringLiteral("TIMED OUT"));
	EXPECT_EQ(table->item(1, 4)->text(), QStringLiteral("ERROR"));
	EXPECT_EQ(table->item(1, 5)->text(), QStringLiteral("1001"));
	EXPECT_EQ(table->item(1, 8)->text(), QStringLiteral("connection failed"));
	EXPECT_EQ(table->item(1, 3)->background().color(), QColor(QStringLiteral("#C62828")));
	EXPECT_EQ(table->item(1, 4)->background().color(), QColor(QStringLiteral("#EF9A9A")));
}

TEST(ComponentMonitorDialogTest, AggregatesOverallStatusAndNotifiesConsumers) {
	yds::ros2::widgets::ComponentMonitorDialog dialog;
	auto* label = dialog.findChild<QLabel*>(QStringLiteral("overallStatusLabel"));
	ASSERT_NE(label, nullptr);
	yds::ros2::widgets::ComponentMonitorDialog::OverallStatus notifiedStatus =
		yds::ros2::widgets::ComponentMonitorDialog::OverallStatus::kWaiting;
	int notifiedReceivingCount = -1;
	int notifiedTotalCount = -1;
	QObject::connect(
		&dialog,
		&yds::ros2::widgets::ComponentMonitorDialog::overallStatusChanged,
		[&](
			yds::ros2::widgets::ComponentMonitorDialog::OverallStatus status,
			int receivingCount,
			int totalCount) {
			notifiedStatus = status;
			notifiedReceivingCount = receivingCount;
			notifiedTotalCount = totalCount;
		});

	dialog.setComponentDisplayName(QStringLiteral("camera/status"), QStringLiteral("Camera"));
	dialog.setComponentDisplayName(QStringLiteral("plc/status"), QStringLiteral("PLC"));
	dialog.setTopicReceptionStatus({
		QStringLiteral("camera/status"),
		yds::ros2::TopicReceptionState::kReceiving,
		QDateTime::currentDateTime(),
		1,
		QString()});
	EXPECT_EQ(notifiedStatus, yds::ros2::widgets::ComponentMonitorDialog::OverallStatus::kWarning);

	dialog.setComponentStatus({
		QStringLiteral("camera/status"),
		QStringLiteral("camera-1"),
		yds::ros2::ComponentState::kRunning,
		0,
		QString(),
		QDateTime::currentDateTime()});
	dialog.setTopicReceptionStatus({
		QStringLiteral("plc/status"),
		yds::ros2::TopicReceptionState::kReceiving,
		QDateTime::currentDateTime(),
		1,
		QString()});
	dialog.setComponentStatus({
		QStringLiteral("plc/status"),
		QStringLiteral("plc-1"),
		yds::ros2::ComponentState::kReady,
		0,
		QString(),
		QDateTime::currentDateTime()});
	EXPECT_EQ(notifiedStatus, yds::ros2::widgets::ComponentMonitorDialog::OverallStatus::kNormal);
	EXPECT_EQ(notifiedReceivingCount, 2);
	EXPECT_EQ(notifiedTotalCount, 2);
	EXPECT_EQ(label->text(), QStringLiteral("全体状態: 正常（受信中 2/2）"));

	dialog.setComponentStatus({
		QStringLiteral("camera/status"),
		QStringLiteral("camera-1"),
		yds::ros2::ComponentState::kCritical,
		2001,
		QString(),
		QDateTime::currentDateTime()});
	EXPECT_EQ(notifiedStatus, yds::ros2::widgets::ComponentMonitorDialog::OverallStatus::kError);

	dialog.setComponentStatus({
		QStringLiteral("camera/status"),
		QStringLiteral("camera-1"),
		yds::ros2::ComponentState::kStopped,
		0,
		QString(),
		QDateTime::currentDateTime()});
	EXPECT_EQ(notifiedStatus, yds::ros2::widgets::ComponentMonitorDialog::OverallStatus::kNormal);
}

TEST(ComponentMonitorDialogTest, SortsAndFiltersRowsSafely) {
	yds::ros2::widgets::ComponentMonitorDialog dialog;
	auto* table = dialog.findChild<QTableWidget*>(QStringLiteral("topicStatusTable"));
	auto* filterLineEdit = dialog.findChild<QLineEdit*>(QStringLiteral("filterLineEdit"));
	auto* attentionOnlyCheckBox = dialog.findChild<QCheckBox*>(
		QStringLiteral("showAttentionOnlyCheckBox"));
	ASSERT_NE(table, nullptr);
	ASSERT_NE(filterLineEdit, nullptr);
	ASSERT_NE(attentionOnlyCheckBox, nullptr);
	EXPECT_TRUE(table->isSortingEnabled());

	dialog.setComponentDisplayName(QStringLiteral("camera/status"), QStringLiteral("Zebra camera"));
	dialog.setComponentDisplayName(QStringLiteral("plc/status"), QStringLiteral("Alpha PLC"));
	EXPECT_EQ(table->item(0, 1)->text(), QStringLiteral("plc/status"));
	dialog.setTopicReceptionStatus({
		QStringLiteral("camera/status"),
		yds::ros2::TopicReceptionState::kReceiving,
		QDateTime::currentDateTime(),
		1,
		QString()});
	dialog.setComponentStatus({
		QStringLiteral("camera/status"),
		QStringLiteral("camera-1"),
		yds::ros2::ComponentState::kRunning,
		0,
		QStringLiteral("capturing"),
		QDateTime::currentDateTime()});
	dialog.setTopicReceptionStatus({
		QStringLiteral("plc/status"),
		yds::ros2::TopicReceptionState::kReceiving,
		QDateTime::currentDateTime(),
		1,
		QString()});
	dialog.setComponentStatus({
		QStringLiteral("plc/status"),
		QStringLiteral("plc-1"),
		yds::ros2::ComponentState::kReady,
		0,
		QStringLiteral("connected"),
		QDateTime::currentDateTime()});

	filterLineEdit->setText(QStringLiteral("camera"));
	EXPECT_TRUE(table->isRowHidden(0));
	EXPECT_FALSE(table->isRowHidden(1));
	filterLineEdit->clear();
	attentionOnlyCheckBox->setChecked(true);
	EXPECT_TRUE(table->isRowHidden(0));
	EXPECT_TRUE(table->isRowHidden(1));

	dialog.setComponentStatus({
		QStringLiteral("camera/status"),
		QStringLiteral("camera-1"),
		yds::ros2::ComponentState::kWarning,
		1001,
		QStringLiteral("処理時間が上限に近づいています"),
		QDateTime::currentDateTime()});
	EXPECT_TRUE(table->isRowHidden(0));
	EXPECT_FALSE(table->isRowHidden(1));
	EXPECT_EQ(table->item(1, 1)->text(), QStringLiteral("camera/status"));
	EXPECT_EQ(table->item(1, 4)->text(), QStringLiteral("WARNING"));
}

}  // namespace

int main(int argc, char* argv[]) {
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QApplication application(argc, argv);
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
