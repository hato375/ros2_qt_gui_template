#pragma once

#include <memory>

#include <QDialog>
#include <QHash>

#include <yds/ros2/component_status.h>
#include <yds/ros2/topic_reception_status.h>

namespace Ui {
class ComponentMonitorDialog;
}

namespace yds::ros2::widgets {

/// @brief 複数コンポーネントの通信状態と動作状態を表示するダイアログ
class ComponentMonitorDialog final : public QDialog {
	Q_OBJECT

public:
	/// @brief 全監視対象を集約した状態
	enum class OverallStatus {
		kWaiting,
		kNormal,
		kWarning,
		kError,
	};
	Q_ENUM(OverallStatus)

	/// @brief コンポーネント監視ダイアログを生成する
	/// @param parent 親ウィジェット
	explicit ComponentMonitorDialog(QWidget* parent = nullptr);
	/// @brief コンポーネント監視ダイアログを破棄する
	~ComponentMonitorDialog() override;

	/// @brief 監視トピックに対応するコンポーネント表示名を設定する
	/// @param topicName 監視するROSトピック名
	/// @param displayName GUIに表示するコンポーネント名
	void setComponentDisplayName(
		const QString& topicName,
		const QString& displayName) noexcept;

	/// @brief ROSトピックの受信状況を更新する
	/// @param status 表示する受信状況
	void setTopicReceptionStatus(const TopicReceptionStatus& status) noexcept;

	/// @brief コンポーネント状態を更新する
	/// @param status 表示するコンポーネント状態
	void setComponentStatus(const ComponentStatus& status) noexcept;

signals:
	/// @brief 全監視対象の集約状態が更新されたことを通知する
	/// @param status 集約状態
	/// @param receivingCount 受信中の監視対象数
	/// @param totalCount 全監視対象数
	void overallStatusChanged(
		OverallStatus status,
		int receivingCount,
		int totalCount);

private:
	int findOrCreateTopicRow(const QString& topicName) noexcept;
	bool isAttentionRequired(const QString& topicName) const noexcept;
	void updateOverallStatus() noexcept;
	void updateRowVisibility() noexcept;

	std::unique_ptr<Ui::ComponentMonitorDialog> ui_;
	QHash<QString, TopicReceptionState> receptionStates_;
	QHash<QString, ComponentState> componentStates_;
};

/// @brief 集約状態を表示用文字列へ変換する
/// @param status 集約状態
/// @return 集約状態の表示用文字列
QString overallStatusText(ComponentMonitorDialog::OverallStatus status);

/// @brief 集約状態に対応するスタイルシートを取得する
/// @param status 集約状態
/// @return QLabelへ設定できるスタイルシート
QString overallStatusStyleSheet(ComponentMonitorDialog::OverallStatus status);

}  // namespace yds::ros2::widgets

Q_DECLARE_METATYPE(yds::ros2::widgets::ComponentMonitorDialog::OverallStatus)
