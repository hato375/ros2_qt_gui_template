# ROSトピック受信Supervisor利用ガイド

## 1. 目的

`ros2_qt_gui`はSupervisorとして複数の`std_msgs/msg/String`トピックを購読し、各ノードから状態が
定期的に届いているかを一元監視します。

監視する情報は次の2種類に分かれます。

- 通信状態: `WAITING`、`RECEIVING`、`TIMED OUT`
- 設備状態: `ready`、`running`、`error`など、受信したメッセージの内容

`RECEIVING`はメッセージが定期的に届いていることを表し、設備が正常であることまでは
保証しません。設備状態の判定は、メッセージ内容または将来追加する型付きステータスメッセージで
行います。

## 2. 監視対象の設定

`src/ros2_qt_gui/config/ros2_qt_gui.yaml`へ監視対象を列挙します。

```yaml
ros2_qt_gui_node:
  ros__parameters:
    heartbeat_interval_ms: 1000
    gui_status_check_interval_ms: 200
    monitored_topics:
      - camera/status
      - plc/status
    topic_reception_timeout_ms: 3000
```

`monitored_topics`に空文字列、重複したトピック、または空のリストは指定できません。
現在は全トピックへ共通のタイムアウト時間を適用します。

## 3. 起動と動作確認

Supervisor GUIを起動します。

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 launch ros2_qt_gui ros2_qt_gui.launch.py
```

別のターミナルからcamera状態を1秒周期で送信します。

```bash
source /opt/ros/humble/setup.bash
ros2 topic pub /camera/status std_msgs/msg/String "{data: capturing}" --rate 1
```

さらに別のターミナルからPLC状態を送信します。

```bash
source /opt/ros/humble/setup.bash
ros2 topic pub /plc/status std_msgs/msg/String "{data: connected}" --rate 1
```

GUIにはトピックごとに次の情報が表示されます。

- 受信状態
- 最終受信時刻
- 受信件数
- 最新メッセージ

送信コマンドを`Ctrl+C`で停止すると、そのトピックだけが3秒後に`TIMED OUT`になります。
他のトピックの受信状態には影響しません。同じコマンドを再実行すると
`RECEIVING`へ復旧します。

## 4. cameraノードやPLCノードからの送信

各ノードは、設定したタイムアウト時間より短い周期で状態をPublishします。
3秒タイムアウトの場合は、1秒周期など十分な余裕を持つ周期を使用します。

メッセージ例:

```text
camera/status: ready、capturing、error
plc/status: connected、running、error
```

現在のSupervisorは文字列を表示するだけで、`error`を受信しても自動的な停止判断は
行いません。安全停止や設備異常の判定では、自由形式文字列ではなく、
状態値とエラーコードを持つ型付きメッセージを使用してください。

## 5. `TopicReceptionMonitor`の共通利用

GUIを持たないノードでも`yds::ros2::TopicReceptionMonitor`を利用できます。

```cpp
#include <chrono>

#include <yds/ros2/topic_reception_monitor.h>

yds::ros2::TopicReceptionMonitor monitor(
	QStringLiteral("camera/status"),
	std::chrono::milliseconds(3000));

const auto transition = monitor.recordReception(QStringLiteral("capturing"));
```

ROSタイマーなどから定期的に`checkTimeout()`を呼びます。

```cpp
const auto transition = monitor.checkTimeout();
const auto status = monitor.takeStatusUpdate();
```

モニターはROSメッセージ型、ROSログ、および安全停止処理に依存しません。
各ノードはROS API境界で受信メッセージを`QString`へ変換し、
返された状態遷移に応じてログ、通知、または停止判断を行います。

## 6. 状態遷移

```text
WAITING
  └─ 初回受信 → RECEIVING

RECEIVING
  └─ タイムアウト → TIMED OUT

TIMED OUT
  └─ 再受信 → RECEIVING
```

初回受信、タイムアウト、および復旧はROSログとGUIイベントログへ記録されます。
メッセージを受信するたびにINFOログを出力することはありません。
