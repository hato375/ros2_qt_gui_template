#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>

namespace yds::ros2 {

/// @brief アプリケーションで通知するイベントの重要度
enum class ApplicationEventLevel {
	kInfo,
	kWarning,
	kError,
	kCritical,
};

/// @brief GUIへ通知するアプリケーションイベント
struct ApplicationEvent {
	ApplicationEventLevel level;
	QDateTime timestamp;
	QString message;
};

/// @brief アプリケーションイベントの重要度を文字列へ変換する
/// @param level 変換する重要度
/// @return 重要度を表す文字列
inline QString eventLevelText(ApplicationEventLevel level) noexcept {
	switch (level) {
	case ApplicationEventLevel::kInfo:
		return QStringLiteral("INFO");
	case ApplicationEventLevel::kWarning:
		return QStringLiteral("WARN");
	case ApplicationEventLevel::kError:
		return QStringLiteral("ERROR");
	case ApplicationEventLevel::kCritical:
		return QStringLiteral("CRITICAL");
	}
	return QStringLiteral("UNKNOWN");
}

}  // namespace yds::ros2

Q_DECLARE_METATYPE(yds::ros2::ApplicationEvent)
