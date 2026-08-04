#pragma once

#include <QtGlobal>
#include <QDateTime>
#include <QMetaType>
#include <QString>

namespace yds::ros2 {

/// @brief 物理設備またはロジック機能の動作状態
enum class ComponentState {
	kUnknown,
	kInitializing,
	kReady,
	kRunning,
	kWarning,
	kError,
	kCritical,
	kStopped,
};

/// @brief コンポーネント状態を表示用文字列へ変換する
/// @param state 変換するコンポーネント状態
/// @return コンポーネント状態の表示用文字列
inline QString componentStateText(ComponentState state) {
	switch (state) {
	case ComponentState::kUnknown:
		return QStringLiteral("UNKNOWN");
	case ComponentState::kInitializing:
		return QStringLiteral("INITIALIZING");
	case ComponentState::kReady:
		return QStringLiteral("READY");
	case ComponentState::kRunning:
		return QStringLiteral("RUNNING");
	case ComponentState::kWarning:
		return QStringLiteral("WARNING");
	case ComponentState::kError:
		return QStringLiteral("ERROR");
	case ComponentState::kCritical:
		return QStringLiteral("CRITICAL");
	case ComponentState::kStopped:
		return QStringLiteral("STOPPED");
	}
	return QStringLiteral("UNKNOWN");
}

/// @brief ROSトピックから受信したコンポーネント状態
struct ComponentStatus {
	QString topicName;
	QString componentId;
	ComponentState state;
	qint32 errorCode;
	QString message;
	QDateTime timestamp;
};

}  // namespace yds::ros2

Q_DECLARE_METATYPE(yds::ros2::ComponentStatus)
