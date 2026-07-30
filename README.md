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
colcon build --packages-select ros2_qt_gui
source install/setup.bash
```

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

## パラメータ

既定値は`config/ros2_qt_gui.yaml`に定義されています。

```yaml
ros2_qt_gui_node:
  ros__parameters:
    heartbeat_interval_ms: 1000
    gui_status_check_interval_ms: 200
```

別の設定ファイルを指定できます。

```bash
ros2 launch ros2_qt_gui ros2_qt_gui.launch.py \
  params_file:=/path/to/custom.yaml
```

## テスト

```bash
source /opt/ros/humble/setup.bash
colcon test --packages-select ros2_qt_gui
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
- [AIエージェント向け基本プロンプト](prompts/ros2_qt_gui_agent_prompt.md)
