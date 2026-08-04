# ROSトピック受信Supervisor利用ガイド

## 1. 目的

`ros2_qt_gui`はSupervisorとして複数の`yds_interfaces/msg/EquipmentStatus`トピックを購読し、
各ノードから状態が定期的に届いているかを一元監視します。

監視する情報は次の2種類に分かれます。

- 通信状態: `WAITING`、`RECEIVING`、`TIMED OUT`
- 設備状態: `UNKNOWN`、`INITIALIZING`、`READY`、`RUNNING`、`WARNING`、`ERROR`、
  `CRITICAL`、`STOPPED`

`RECEIVING`はメッセージが定期的に届いていることを表し、設備が正常であることまでは
保証しません。例えば、設備が`ERROR`を通知し続けている間もメッセージが定期的に届いていれば、
通信状態は`RECEIVING`です。

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
source install/setup.bash
ros2 topic pub /camera/status yds_interfaces/msg/EquipmentStatus \
  "{equipment_id: camera-1, state: 3, error_code: 0, message: capturing}" --rate 1
```

さらに別のターミナルからPLC状態を送信します。

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 topic pub /plc/status yds_interfaces/msg/EquipmentStatus \
  "{equipment_id: plc-1, state: 2, error_code: 0, message: connected}" --rate 1
```

GUIにはトピックごとに次の情報が表示されます。

- トピック名と設備ID
- 通信状態
- 設備状態とエラーコード
- 最終受信時刻
- 受信件数
- メッセージ

送信コマンドを`Ctrl+C`で停止すると、そのトピックだけが3秒後に`TIMED OUT`になります。
他のトピックの受信状態には影響しません。同じコマンドを再実行すると
`RECEIVING`へ復旧します。

## 4. cameraノードやPLCノードからの送信

各ノードは、設定したタイムアウト時間より短い周期で状態をPublishします。
3秒タイムアウトの場合は、1秒周期など十分な余裕を持つ周期を使用します。

`EquipmentStatus`のフィールドは次のとおりです。

| フィールド | 用途 |
|---|---|
| `header.stamp` | 状態を生成した時刻。ゼロの場合はSupervisorでの受信時刻を使用 |
| `equipment_id` | 設備を識別する名前 |
| `state` | 下表の設備状態 |
| `error_code` | 正常時は0、異常時は設備ごとに定義したコード |
| `message` | 運用者向けの補足 |

| 値 | 定数 | 意味 |
|---:|---|---|
| 0 | `STATE_UNKNOWN` | 状態不明 |
| 1 | `STATE_INITIALIZING` | 初期化中 |
| 2 | `STATE_READY` | 運転準備完了 |
| 3 | `STATE_RUNNING` | 運転中 |
| 4 | `STATE_WARNING` | 継続可能な注意状態 |
| 5 | `STATE_ERROR` | 処理失敗または復旧可能な異常 |
| 6 | `STATE_CRITICAL` | 安全な継続が困難な重大異常 |
| 7 | `STATE_STOPPED` | 停止中 |

設備状態またはエラーコードが変化すると、GUIイベントにも記録されます。`CRITICAL`の通知だけで
Supervisorが自動停止することはありません。安全停止条件と停止対象は、設備仕様に基づいて
呼び出し側で明示的に実装します。

ROS標準の`diagnostic_msgs`はコンポーネントの健全性や診断値の通知に適しています。一方、この
メッセージは`INITIALIZING`、`READY`、`RUNNING`、`STOPPED`という設備の運転ライフサイクルを
型として共有する目的のため、プロジェクト固有の`EquipmentStatus`を使用します。必要になった場合は、
同じノードから標準diagnosticsも併せてPublishできます。

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
各ノードはROS API境界で受信メッセージを`yds::ros2::EquipmentStatus`や`QString`へ変換し、
返された状態遷移に応じてログ、通知、または停止判断を行います。ROSメッセージ型はノード間通信、
Qt型はアプリケーション内という境界を保ちます。

`EquipmentStatus`の送受信変換には、`yds_ros2`の共通APIを使用できます。

```cpp
#include <yds/ros2/equipment_status_conversion.h>

const auto qtStatus =
	yds::ros2::equipmentStatusFromRos(QStringLiteral("camera/status"), rosMessage);
const auto publishMessage = yds::ros2::equipmentStatusToRos(qtStatus);
```

ROSメッセージの`header.stamp`がゼロの場合、変換後の`timestamp`は無効な`QDateTime`になります。
Supervisorはこれを受信時刻で補完します。送信ノードでは、可能な限り状態を生成した時刻を設定します。

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
