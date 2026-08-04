# ロジック系サンプルノード利用ガイド

## 1. 目的

`sample_processor`は、物理設備を持たないロジック系ROSノードで
`yds::ros2::ComponentStatusPublisher`を利用する例です。通常の`rclcpp::Node`を継承し、
コンポーネント状態Publisherをメンバーとして所有します。

サンプルは次の順に状態を通知します。

1. 起動直後は`INITIALIZING`
2. 最初の処理タイマーで`READY`
3. 以降の処理タイマーで`RUNNING`

`RUNNING`のメッセージには完了した処理回数を設定します。状態変更時の即時通知に加えて、
`ComponentStatusPublisher`が最新状態を設定周期で再通知します。

## 2. ビルド

```bash
source /opt/ros/humble/setup.bash
colcon build --packages-up-to sample_processor
source install/setup.bash
```

## 3. サンプルノードだけを起動する

```bash
ros2 launch sample_processor sample_processor.launch.py
```

別のターミナルから状態を確認できます。

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 topic echo /sample_processor/status yds_interfaces/msg/ComponentStatus
```

## 4. Supervisor GUIと同時に起動する

```bash
ros2 launch sample_processor component_status_demo.launch.py
```

このlaunchファイルは次の2ノードを起動します。

- `sample_processor_node`
- `ros2_qt_gui_node`

GUIは`sample_processor/status`だけを監視するデモ専用設定で起動します。GUI上で通信状態が
`RECEIVING`、コンポーネント状態が`READY`から`RUNNING`へ変化することを確認できます。

### 4.1 正常状態を確認する

起動後、GUIの監視テーブルが次の表示になることを確認します。

| 項目 | 期待値 |
|---|---|
| Component | `Sample processor` |
| Topic | `sample_processor/status` |
| Component ID | `sample-processor-1` |
| Communication | `RECEIVING` |
| Component state | 起動直後の`READY`から`RUNNING`へ遷移 |
| Error code | `0` |
| Message | `Processed count: ...` |

### 4.2 WARNING、ERROR、復旧を確認する

別のターミナルからテスト用Serviceを1つずつ呼び出します。各コマンドの実行後にGUIを確認してから、
次のコマンドへ進みます。

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash

ros2 service call /sample_processor_node/set_warning std_srvs/srv/Trigger {}
ros2 service call /sample_processor_node/set_error std_srvs/srv/Trigger {}
ros2 service call /sample_processor_node/recover std_srvs/srv/Trigger {}
```

| Service | Component state | Error code | Message |
|---|---|---:|---|
| `set_warning` | `WARNING` | 1001 | `Test warning requested` |
| `set_error` | `ERROR` | 2001 | `Test error requested` |
| `recover` | `RUNNING` | 0 | `Test state recovered` |

各状態でも定期通知が続くため、Communicationは`RECEIVING`のままです。状態セルの色と文字、エラーコード、
メッセージ、およびGUI下部のイベントログが切り替わることを確認します。これらはデモ専用Serviceであり、
実案件の状態変更を外部から自由に操作するための共通APIではありません。

### 4.3 日本語メッセージ、タイムアウト、再受信を確認する

サンプルノードと送信元が競合しないよう、デモを終了してからSupervisor GUIだけをデモ設定で起動します。

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 launch ros2_qt_gui ros2_qt_gui.launch.py \
  params_file:="$(ros2 pkg prefix sample_processor)/share/sample_processor/config/component_status_demo_gui.yaml"
```

別のターミナルで、文字コードがUTF-8であることを確認して日本語の状態を定期送信します。

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
locale charmap
ros2 topic pub /sample_processor/status yds_interfaces/msg/ComponentStatus \
  "{component_id: '画像処理ノード1', state: 4, error_code: 1001, message: '処理時間が上限に近づいています'}" \
  --rate 1
```

`locale charmap`が`UTF-8`と表示される環境を使用します。GUIのComponent IDとMessageに日本語が
文字化けせず表示され、Component stateが`WARNING`、Communicationが`RECEIVING`になることを確認します。
送信を`Ctrl+C`で止めると、3秒後に`TIMED OUT`へ変化します。同じ送信コマンドを再実行すると、
`RECEIVING`へ復旧することも確認できます。

## 5. パラメータ

| パラメータ | 既定値 | 範囲 | 用途 |
|---|---:|---:|---|
| `processing_interval_ms` | 1000 | 100～600000 | サンプル処理周期 |
| `component_status.component_id` | `sample-processor-1` | 空文字不可 | コンポーネントID |
| `component_status.status_topic` | `sample_processor/status` | 空文字不可 | 状態通知トピック |
| `component_status.publish_interval_ms` | 1000 | 100～600000 | 最新状態の定期通知周期 |

既定値は`src/sample_processor/config/sample_processor.yaml`にあります。

## 6. 実案件へ適用する場合

実際の画像処理や点群処理ノードでは、サンプルのタイマー処理を実処理の完了通知へ置き換えます。

- 初期化開始時: `INITIALIZING`
- 初期化成功時: `READY`
- 処理中または正常稼働時: `RUNNING`
- 処理を継続できる注意状態: `WARNING`
- 処理失敗時: `ERROR`
- 安全上直ちに対応が必要な状態: `CRITICAL`

状態通知は安全停止を代行しません。必要な停止、リトライ、復旧処理は利用するノードに実装します。
