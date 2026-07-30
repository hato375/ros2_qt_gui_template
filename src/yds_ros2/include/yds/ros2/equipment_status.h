#pragma once

#include <QtGlobal>
#include <QDateTime>
#include <QMetaType>
#include <QString>

namespace yds::ros2 {

/// @brief 設備の動作状態
enum class EquipmentState {
	kUnknown,
	kInitializing,
	kReady,
	kRunning,
	kWarning,
	kError,
	kCritical,
	kStopped,
};

/// @brief 設備状態を表示用文字列へ変換する
/// @param state 変換する設備状態
/// @return 設備状態の表示用文字列
inline QString equipmentStateText(EquipmentState state) {
	switch (state) {
	case EquipmentState::kUnknown:
		return QStringLiteral("UNKNOWN");
	case EquipmentState::kInitializing:
		return QStringLiteral("INITIALIZING");
	case EquipmentState::kReady:
		return QStringLiteral("READY");
	case EquipmentState::kRunning:
		return QStringLiteral("RUNNING");
	case EquipmentState::kWarning:
		return QStringLiteral("WARNING");
	case EquipmentState::kError:
		return QStringLiteral("ERROR");
	case EquipmentState::kCritical:
		return QStringLiteral("CRITICAL");
	case EquipmentState::kStopped:
		return QStringLiteral("STOPPED");
	}
	return QStringLiteral("UNKNOWN");
}

/// @brief ROSトピックから受信した設備状態
struct EquipmentStatus {
	QString topicName;
	QString equipmentId;
	EquipmentState state;
	qint32 errorCode;
	QString message;
	QDateTime timestamp;
};

}  // namespace yds::ros2

Q_DECLARE_METATYPE(yds::ros2::EquipmentStatus)
