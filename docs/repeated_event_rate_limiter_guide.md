# 繰り返しイベントの通知抑制ガイド

## 1. 目的

`yds::ros2::RepeatedEventRateLimiter`は、周期処理や受信処理が同じ異常を繰り返したときに、ログやGUI通知が
大量に出力されることを防ぐ共通クラスです。イベントを捨てたままにせず、次に通知可能になった時点で
抑制件数を取得できます。

この処理を「レート制限」と呼びます。最初の異常は直ちに通知し、指定した間隔内の繰り返しだけを抑えます。

## 2. 基本的な使用方法

```cpp
#include <chrono>

#include <yds/ros2/repeated_event_rate_limiter.h>

using namespace std::chrono_literals;

yds::ros2::RepeatedEventRateLimiter errorRateLimiter(10s);

try {
	processEquipment();
	errorRateLimiter.reset();
} catch (const std::exception& exception) {
	const auto result = errorRateLimiter.record();
	if (result.shouldReport) {
		// exception.what()とresult.suppressedCountをログへ記録する
	}
}
```

成功時には`reset()`を呼びます。これにより、復旧後に同じ異常が再発した場合は、通知間隔内であっても
新しい異常として直ちに通知されます。

## 3. Supervisorでの適用

`ros2_qt_gui`のSupervisorは、次の継続的なコールバック失敗へ10秒の通知間隔を適用します。

- ハートビート通知
- ComponentStatus受信処理
- トピック受信状態のGUI通知
- ComponentStatusのGUI通知
- GUIイベント通知

最初の失敗は直ちに出力します。同じ処理経路で失敗が継続した場合は抑制し、10秒後の通知へ
`repeated occurrences suppressed`として抑制件数を付加します。正常なコールバックが一度完了すると
抑制状態を解除します。

通信タイムアウト、復旧、ComponentStatusの品質異常は、もともと状態遷移時だけ通知されるため、この
レート制限の対象外です。
