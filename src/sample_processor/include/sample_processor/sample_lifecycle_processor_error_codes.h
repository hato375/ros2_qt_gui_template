#pragma once

#include <QtGlobal>

namespace sampleprocessor::lifecycle_error_code {

/// @brief Lifecycleコールバックまたは状態通知の汎用エラー
inline constexpr qint32 kTransition = 9001;
/// @brief configure時の設備固有処理エラー
inline constexpr qint32 kConfiguration = 9101;
/// @brief activate時の設備固有処理エラー
inline constexpr qint32 kActivation = 9102;
/// @brief deactivate時の設備固有処理エラー
inline constexpr qint32 kDeactivation = 9103;
/// @brief cleanup時の設備固有処理エラー
inline constexpr qint32 kCleanup = 9104;
/// @brief shutdown時の設備固有処理エラー
inline constexpr qint32 kShutdown = 9105;

}  // namespace sampleprocessor::lifecycle_error_code
