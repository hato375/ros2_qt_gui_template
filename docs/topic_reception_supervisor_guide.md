# ROSトピック受信Supervisor利用ガイド

## 1. 目的

`ros2_qt_gui`はSupervisorとして複数の`yds_interfaces/msg/ComponentStatus`トピックを購読し、
各ノードから状態が定期的に届いているかを一元監視します。

監視する情報は次の2種類に分かれます。

- 通信状態: `WAITING`、`RECEIVING`、`TIMED OUT`
- コンポーネント状態: `UNKNOWN`、`INITIALIZING`、`READY`、`RUNNING`、`WARNING`、`ERROR`、
  `CRITICAL`、`STOPPED`

`RECEIVING`はメッセージが定期的に届いていることを表し、対象コンポーネントが正常であることまでは
保証しません。例えば、コンポーネントが`ERROR`を通知し続けている間もメッセージが定期的に届いていれば、
通信状態は`RECEIVING`です。

### 1.1 状態監視トピックのQoS

QoSはROS 2メッセージの配送方法を定める設定です。状態監視トピックは送受信ともに`Reliable`、
`Transient Local`を使用します。

- `Reliable`: 一時的なパケット欠落があっても、可能な範囲で再送して確実な配送を試みる
- `Transient Local`: Publisherが最新状態を保持し、後から起動したSupervisorにも渡す

これは、設備ノードを先に起動してSupervisorを後から起動した場合でも、次のheartbeatを待たずに最新状態を
表示するためです。`Volatile`は接続後の新しいメッセージだけを扱う設定であり、正式なComponentStatus
監視トピックには使用しません。実装時は共通の`ComponentStatusPublisher`を使用してください。

QoS設定が一致しないPublisherとは接続できない場合があります。手動確認で`ros2 topic pub`を使う場合は、
3章の例のように`--qos-reliability reliable --qos-durability transient_local`を指定します。

ROS 2のプロセス内通信最適化は`Transient Local`と併用できません。Supervisorは別プロセスの設備ノードを
監視する役割であり、この最適化を使用しません。`NodeOptions`でプロセス内通信を有効にした場合は、
不完全な監視状態で起動を続けず、設定エラーとして起動を中止します。

## 2. 監視対象の設定

`src/ros2_qt_gui/config/ros2_qt_gui.yaml`へ監視対象を列挙します。

```yaml
ros2_qt_gui_node:
  ros__parameters:
    heartbeat_interval_ms: 1000
    gui_status_check_interval_ms: 200
    component_monitor_names:
      - camera
      - plc
    component_monitors:
      camera:
        enabled: true
        display_name: Camera
        status_topic: camera/status
        expected_component_id: camera-1
        timeout_ms: 3000
        maximum_status_age_ms: 0
        maximum_future_skew_ms: 0
      plc:
        enabled: true
        display_name: PLC
        status_topic: plc/status
        expected_component_id: plc-1
        timeout_ms: 5000
        maximum_status_age_ms: 0
        maximum_future_skew_ms: 0
```

`component_monitor_names`へ監視設定名を列挙し、`component_monitors`以下へ同じ名前の設定を記述します。
設定名は英字またはアンダースコアで始め、英数字とアンダースコアを使用できます。設定名は設定内の
識別子であり、ROSトピック名とは分離します。

| 項目 | 説明 |
|---|---|
| `enabled` | `false`の場合はSubscriptionを作成せず、監視対象から除外 |
| `display_name` | GUIに表示するコンポーネント名。空文字は不可 |
| `status_topic` | 購読する`yds_interfaces/msg/ComponentStatus`トピック |
| `expected_component_id` | 期待する送信元ID。空文字で照合無効。空白だけは不可 |
| `timeout_ms` | 受信タイムアウト時間。500～600000ミリ秒 |
| `maximum_status_age_ms` | 状態生成時刻の許容経過時間。0で無効、最大86400000ミリ秒 |
| `maximum_future_skew_ms` | 状態生成時刻の未来方向の許容ずれ。0で無効、最大86400000ミリ秒 |

設定名、トピック名の重複、空のトピック名、範囲外のタイムアウト・時刻許容値、または全設定が無効の場合、
Supervisorは購読を開始せず起動エラーにします。

監視対象を増やす場合は、例えば`component_monitor_names`へ`rear_camera`を追加し、
`component_monitors.rear_camera`以下へ設定を追加します。トピック名とタイムアウトが同じ設定ブロックに
まとまるため、監視対象を並べ替えても対応関係は崩れません。

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
ros2 topic pub /camera/status yds_interfaces/msg/ComponentStatus \
  "{component_id: camera-1, state: 3, error_code: 0, message: capturing}" \
  --qos-reliability reliable --qos-durability transient_local --rate 1
```

さらに別のターミナルからPLC状態を送信します。

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 topic pub /plc/status yds_interfaces/msg/ComponentStatus \
  "{component_id: plc-1, state: 2, error_code: 0, message: connected}" \
  --qos-reliability reliable --qos-durability transient_local --rate 1
```

GUIにはトピックごとに次の情報が表示されます。

- コンポーネント表示名、トピック名、およびコンポーネントID
- 通信状態
- コンポーネント状態とエラーコード
- 最終受信時刻
- 受信件数
- メッセージ

GUI上部には、全監視対象を集約した全体状態と、`RECEIVING`の対象数を表示します。
通常画面には概要だけを表示し、`Show component monitor`ボタンから詳細な監視テーブルを開きます。
詳細ダイアログを閉じても監視は継続し、再度開いたときに最新状態を表示します。
詳細テーブルは列ごとに並び替えでき、文字検索と`要確認のみ表示`で監視対象を絞り込めます。

| 全体状態 | 条件 |
|---|---|
| `異常` | 1件以上が`TIMED OUT`、`ERROR`、または`CRITICAL` |
| `警告` | 異常条件はなく、1件以上が`WARNING`、または受信済みで`UNKNOWN` |
| `待機中` | 異常・警告はないが、未受信の監視対象が存在 |
| `正常` | 全対象が`RECEIVING`で、上記の異常・警告条件がない |

判定は表の上から優先します。`INITIALIZING`、`READY`、`RUNNING`、`STOPPED`は、それだけでは
全体状態を異常にしません。`STOPPED`が異常か正常停止かはSupervisorだけでは判断できないため、
案件固有の運転モードと組み合わせた判定が必要になった時点で追加します。

通信状態とコンポーネント状態は別々のセルを色分けします。通信状態は`WAITING`を灰色、`RECEIVING`を緑、
`TIMED OUT`を赤で表示します。コンポーネント状態は初期化中を青、準備完了・運転中を緑、注意を黄、異常を赤、
停止・不明を灰色で表示します。`CRITICAL`は濃い赤と白文字で強調します。

色だけに依存せず、すべてのセルに状態名も表示します。例えばコンポーネント状態が赤い`ERROR`でも、通信セルが
緑の`RECEIVING`であれば「異常状態のメッセージは継続して受信できている」と判断できます。

cameraの送信コマンドを`Ctrl+C`で停止すると、cameraだけが3秒後に`TIMED OUT`になります。
PLCの送信を停止した場合は5秒後に`TIMED OUT`になります。
他のトピックの受信状態には影響しません。同じコマンドを再実行すると
`RECEIVING`へ復旧します。

## 4. cameraノードやPLCノードからの送信

各ノードは、設定したタイムアウト時間より短い周期で状態をPublishします。
3秒タイムアウトの場合は、1秒周期など十分な余裕を持つ周期を使用します。

`ComponentStatus`のフィールドは次のとおりです。

| フィールド | 用途 |
|---|---|
| `header.stamp` | 状態を生成した時刻。ゼロの場合はSupervisorでの受信時刻を使用 |
| `component_id` | 設備を識別する名前 |
| `state` | 下表のコンポーネント状態 |
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

Supervisorはメッセージを受信した事実と、ComponentStatus値の品質を別々に扱います。次の問題を検出した
場合も通信状態は`RECEIVING`として受信件数を更新しますが、コンポーネント状態は安全側の`UNKNOWN`として
表示し、元のエラーコードとメッセージを保持して警告イベントを記録します。

- Component IDが空または空白だけ
- `expected_component_id`を設定したトピックで、異なるComponent IDを受信
- ROSメッセージの状態値が0～7の定義範囲外
- 正常系状態に非ゼロのエラーコードが設定されている
- `WARNING`、`ERROR`、`CRITICAL`でエラーコードが0、またはメッセージが空
- 有効化した許容時間よりtimestampが古い、または未来にずれている

同じ品質問題のheartbeatでは警告を繰り返しません。正常な値へ戻ると復旧イベントを記録し、本来の
コンポーネント状態表示へ戻します。timestampがゼロの場合は従来どおり受信時刻で補完し、品質問題には
しません。複数装置間で時計同期を保証できない環境では、時刻検証を0のまま無効にしてください。

`expected_component_id`は、トピック名が正しくても別設備のメッセージが誤って流れた場合に、取り違えを
検出するための設定です。例えば`camera/status`へ`plc-1`が届いた場合、通信自体は`RECEIVING`ですが、
設備状態は信頼せず`UNKNOWN`として表示します。受信した実際のComponent IDは原因調査のため保持します。

コンポーネント状態またはエラーコードが変化すると、GUIイベントにも記録されます。`CRITICAL`の通知だけで
Supervisorが自動停止することはありません。安全停止条件と停止対象は、設備仕様に基づいて
呼び出し側で明示的に実装します。

ROS標準の`diagnostic_msgs`はコンポーネントの健全性や診断値の通知に適しています。一方、この
メッセージは`INITIALIZING`、`READY`、`RUNNING`、`STOPPED`という設備の運転ライフサイクルを
型として共有する目的のため、プロジェクト固有の`ComponentStatus`を使用します。必要になった場合は、
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
各ノードはROS API境界で受信メッセージを`yds::ros2::ComponentStatus`や`QString`へ変換し、
返された状態遷移に応じてログ、通知、または停止判断を行います。ROSメッセージ型はノード間通信、
Qt型はアプリケーション内という境界を保ちます。

`ComponentStatus`の送受信変換には、`yds_ros2`の共通APIを使用できます。

```cpp
#include <yds/ros2/component_status_conversion.h>

const auto qtStatus =
	yds::ros2::componentStatusFromRos(QStringLiteral("camera/status"), rosMessage);
const auto publishMessage = yds::ros2::componentStatusToRos(qtStatus);
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
