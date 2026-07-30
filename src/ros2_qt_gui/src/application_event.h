#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>

namespace ros2qtgui {

/// @brief アプリケーションで通知するイベントの重要度
enum class ApplicationEventLevel {
	kInfo,
	kWarning,
	kError,
};

/// @brief GUIへ通知するアプリケーションイベント
struct ApplicationEvent {
	ApplicationEventLevel level;
	QDateTime timestamp;
	QString message;
};

}  // namespace ros2qtgui

Q_DECLARE_METATYPE(ros2qtgui::ApplicationEvent)
