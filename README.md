# ROS 2 + Qt GUI

ROS 2ノードとして起動し、Qt 5のGUIを表示する最小構成のアプリケーションです。
Qtのイベントループはメインスレッド、ROS 2 Executorは専用スレッドで実行します。

## 必要な環境

- ROS 2 Humble
- Qt 5 Widgets
- C++17対応コンパイラ

UbuntuではQtの開発パッケージを次のように導入できます。

```bash
sudo apt install qtbase5-dev
```

## ビルド

```bash
source /opt/ros/humble/setup.bash
colcon build --packages-up-to ros2_qt_gui
source install/setup.bash
```

ワークスペースには、GUIアプリケーションの`ros2_qt_gui`、共通ROS 2ライブラリの`yds_ros2`、
再利用可能な監視ダイアログを提供する`yds_ros2_widgets`、共通メッセージ定義の`yds_interfaces`、
およびロジック系サンプルの`sample_processor`が含まれます。
`yds_ros2`の公開APIは
`yds::ros2`名前空間と`yds/ros2`インクルードパスを使用します。
共通ライブラリでもQt Core型を基本とし、ROS APIが標準C++型を要求する境界で必要な変換を行います。
`yds::ros2::TopicReceptionMonitor`はROSメッセージ型に依存せず、camera、PLC、Supervisorなどの
ノードから共通利用できます。
監視ダイアログの組み込み方法は`docs/component_monitor_dialog_guide.md`を参照してください。

## 起動

ROS 2のlaunchファイルから起動します。

```bash
ros2 launch ros2_qt_gui ros2_qt_gui.launch.py
```

または、実行ファイルを直接起動できます。

```bash
ros2 run ros2_qt_gui ros2_qt_gui
```

ウィンドウ内の`ROS heartbeat count`が1秒ごとに増えれば、ROS 2 ExecutorとQt GUIの連携は正常です。
起動や設定、エラーなどの重要イベントは、時刻と重要度とともにウィンドウ内へ表示されます。
表示は最新500件に制限され、古いイベントから自動的に削除されます。

既定では`camera/status`と`plc/status`の`yds_interfaces/msg/ComponentStatus`を監視します。
別のターミナルから次のように送信すると、通信状態、コンポーネント状態、エラーコード、最終受信時刻、
受信件数、およびメッセージが更新されます。

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 topic pub /camera/status yds_interfaces/msg/ComponentStatus \
  "{component_id: camera-1, state: 3, error_code: 0, message: capturing}" --rate 1
```

受信が設定時間以上途切れると`TIMED OUT`になり、再受信すると`RECEIVING`へ復旧します。初回受信、
タイムアウト、および復旧はアプリケーションイベントにも記録されます。

追加の送信コマンドなしで確認する場合は、サンプル処理ノードとGUIを同時に起動できます。

```bash
source /opt/ros/humble/setup.bash
colcon build --packages-up-to sample_processor
source install/setup.bash
ros2 launch sample_processor component_status_demo.launch.py
```

詳しい使い方は`docs/sample_processor_guide.md`を参照してください。

## パラメータ

既定値は`config/ros2_qt_gui.yaml`に定義されています。

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
        timeout_ms: 3000
      plc:
        enabled: true
        display_name: PLC
        status_topic: plc/status
        timeout_ms: 5000
```

`component_monitor_names`へ監視設定名を列挙し、`component_monitors`以下へ同じ名前の設定を記述します。
この例ではcameraを3秒、PLCを5秒の受信タイムアウトで監視します。`enabled: false`の設定は
Subscriptionを作成せず、監視対象から除外されます。

別の設定ファイルを指定できます。

```bash
ros2 launch ros2_qt_gui ros2_qt_gui.launch.py \
  params_file:=/path/to/custom.yaml
```

## テスト

```bash
source /opt/ros/humble/setup.bash
colcon test --packages-select yds_interfaces yds_ros2 yds_ros2_widgets ros2_qt_gui sample_processor
colcon test-result --verbose
```

テストでは、ROSスレッドからQtスレッドへのqueued connection、イベントログの保持件数上限、
およびExecutorの安全な停止を確認します。

## Qt Creator

ビルド後、プロジェクトルートのスクリプトを実行すると、ROS 2とこのワークスペースの環境を設定して
Qt Creatorを起動できます。スクリプトはどのディレクトリからでも実行できます。

```bash
/home/ros/ros2_qt_gui_template/open_qtcreator.sh
```

別のワークスペースで使用する場合も、スクリプトをそのプロジェクトルートへコピーするだけで使用できます。
プロジェクトルートには`.workspace`ファイルが1つだけ存在する必要があります。

Qt Creatorが標準の場所にない場合は、環境変数で実行ファイルを指定できます。

```bash
QT_CREATOR_EXECUTABLE=/path/to/qtcreator ./open_qtcreator.sh
```

## 環境診断

開発環境を変更せずに、ROS 2、Qt、コンパイラ、Qt Creator、ワークスペースなどの状態を確認できます。

```bash
./check_environment.sh
```

必須項目に問題がある場合は終了コード`1`、警告だけの場合は終了コード`0`を返します。

## 初回セットアップ

新しい開発環境では、対話形式のセットアップスクリプトを実行します。

```bash
mv ros2_qt_gui_template ros2_test
cd ros2_test
./setup_dev.sh
```

プロジェクト名には、小文字英字で始まる小文字英数字とアンダースコアのみ使用できます。
スクリプトはフォルダ名をプロジェクト名としてworkspaceと資料を初期化した後、Gitの名前と
メールアドレスをこのリポジトリへローカル設定し、Qt 5、ROS依存関係、初回ビルド、および環境診断を
支援します。依存パッケージのインストールとビルドは実行前に確認します。

## ドキュメント

- [開発環境セットアップガイド](docs/development_setup_guide.md)
- [アーキテクチャ・開発ガイド](docs/architecture_and_development_guidelines.md)
- [ROSトピック受信Supervisor利用ガイド](docs/topic_reception_supervisor_guide.md)
- [AIエージェント向け基本プロンプト](prompts/ros2_qt_gui_agent_prompt.md)
