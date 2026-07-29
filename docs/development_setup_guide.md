# ROS 2 + Qt GUI 開発環境セットアップガイド

## 1. 目的

本資料は、Ubuntu 22.04とROS 2 Humbleを使用して、Qt GUIアプリケーションを開発するための
共通環境を構築する手順を示します。

このプロジェクトは、次の構成を採用しています。

- ROS 2ノードとしてアプリケーションを起動する
- 起動後にQt WidgetsのGUIを表示する
- Qtイベントループはメインスレッドで実行する
- ROS 2 Executorは専用スレッドで実行する
- GUI終了時とROS 2 shutdown時のどちらでも安全に終了する

## 2. 採用バージョン

| 項目 | バージョン |
|---|---|
| OS | Ubuntu 22.04 |
| ROS 2 | Humble |
| C++ | C++17 |
| GUIライブラリ | Qt 5.15.3 / Qt Widgets |
| IDE | Qt Creator 20.0.0 |
| ビルドシステム | CMake、ament_cmake、colcon |

Ubuntu 22.04ではQt 5.15.3とQt 6.2.4を利用できます。本プロジェクトでは、ROS 2 Humbleとの
互換性、OSパッケージによる管理、および将来のROS GUIコンポーネントとの連携を考慮し、Qt 5を使用します。

Qt Creator自体やQt CreatorのKitがQt 6を使用していても、アプリケーションは
`CMakeLists.txt`の指定に従ってQt 5へリンクされます。

## 3. 必要なソフトウェア

ROS 2 Humbleがインストール済みであることを前提とします。

Qt 5の開発パッケージをインストールします。

```bash
sudo apt update
sudo apt install qtbase5-dev
```

次のコマンドでQt 5 Widgetsを確認できます。

```bash
pkg-config --modversion Qt5Widgets
```

本環境では`5.15.3`と表示されます。

### 3.1 環境診断

プロジェクトに付属するスクリプトで、必要な開発環境とプロジェクトの状態を確認できます。

```bash
cd /home/ros/ros2_qt_gui_template
./check_environment.sh
```

スクリプトは環境を変更せず、結果を`OK`、`WARN`、`ERROR`に分類します。`ERROR`がある場合は
終了コード`1`、警告だけの場合は終了コード`0`を返します。

## 4. 初回ビルド

プロジェクトルートで次のコマンドを実行します。

```bash
cd /home/ros/ros2_qt_gui_template
source /opt/ros/humble/setup.bash
colcon build --packages-select ros2_qt_gui
source install/setup.bash
```

ビルド後、実行ファイルは次の場所に生成されます。

```text
install/ros2_qt_gui/lib/ros2_qt_gui/ros2_qt_gui
```

`install/setup.bash`はビルド後に生成されるため、初回ビルドより前には読み込めません。

## 5. コマンドラインからの起動

launchファイルから起動する方法を標準とします。

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 launch ros2_qt_gui ros2_qt_gui.launch.py
```

実行ファイルを直接起動する場合は次のコマンドを使用します。

```bash
ros2 run ros2_qt_gui ros2_qt_gui
```

GUI上の`ROS heartbeat count`が1秒ごとに増加すれば、QtとROS 2 Executorは正常に連携しています。

## 6. Qt Creatorの起動

Qt Creatorは、ROS 2と現在のプロジェクトの環境を読み込んだ状態で起動する必要があります。
プロジェクトルートにある`open_qtcreator.sh`を使用してください。

```bash
cd /home/ros/ros2_qt_gui_template
./open_qtcreator.sh
```

スクリプトは自身の配置場所からプロジェクトルートを判定するため、別のディレクトリからも実行できます。

```bash
/home/ros/ros2_qt_gui_template/open_qtcreator.sh
```

スクリプトは次の処理を行います。

1. ROS 2 Humbleの`setup.bash`を読み込む
2. 現在のプロジェクトの`install/setup.bash`を読み込む
3. プロジェクトルートの`.workspace`ファイルを検出する
4. Qt Creatorの実行ファイルを検出する
5. プロジェクトルートを作業ディレクトリとしてQt Creatorを起動する

Qt Creatorを明示的に指定する場合は、次の環境変数を使用できます。

```bash
QT_CREATOR_EXECUTABLE=/home/ros/qtcreator-20.0.0/bin/qtcreator \
  ./open_qtcreator.sh
```

すでにQt Creatorが起動している場合は一度終了し、スクリプトから起動し直してください。

## 7. ROS 2プラグインのインストール

Qt CreatorでROS 2ワークスペースを扱うため、`ros_qtc_plugin`をインストールします。

本節は、Zennの記事
[「Ubuntu22.04上でROS2とQtCreatorを連携させる（インストール・準備編）」](https://zenn.dev/yupopoi/articles/a19c592361f31e)
を参考に、本プロジェクト向けに要約・再構成したものです。

参考記事の検証環境はUbuntu 22.04、ROS 2 Humble、Qt Creator 13.0.0です。本プロジェクトでは
Qt Creator 20.0.0を使用するため、必ずQt Creator 20に対応するプラグインを選択してください。
Qt Creatorとプラグインのバージョンが一致しない場合、プラグインを読み込めないことがあります。

### 7.1 プラグインの入手

1. [ros_qtc_pluginのReleases](https://github.com/ros-industrial/ros_qtc_plugin/releases)を開く
2. 使用中のQt Creatorに対応するリリースを選択する
3. Ubuntu用の`Linux-x86_64.zip`をダウンロードする
4. ZIPファイルは展開せず、そのまま保持する

Qt Creatorのバージョンは、メニューの「ヘルプ」から「Qt Creatorについて」を開いて確認できます。

### 7.2 ZIPファイルの配置

参考記事では、ダウンロードしたZIPファイルをQt Creatorのpluginsディレクトリへ移動しています。
Qt公式インストーラーを使用した場合の配置先は、通常、次のディレクトリ以下です。

```text
<Qtインストールディレクトリ>/Tools/QtCreator/lib/qtcreator/plugins
```

本環境でQt Creator 20を展開して使用する場合は、Qt Creatorの実際のインストール先に合わせてください。
現在の環境では次の場所です。

```text
/home/ros/qtcreator-20.0.0/lib/qtcreator/plugins
```

インストール画面から参照できる場所であれば、必ずしもこのディレクトリへ置く必要はありません。

### 7.3 Qt Creatorへのインストール

1. Qt Creatorを起動する
2. 「ヘルプ」から「プラグインについて」を開く
3. ダイアログ下部の「プラグインをインストールする」を選択する
4. ダウンロードしたZIPファイルを選択する
5. インストールウィザードに従って進める
6. 必要に応じてQt Creatorを再起動する

インストール後、プラグイン一覧で`ros`を検索し、`ROSProjectManager`が表示され、有効になっていることを
確認します。

### 7.4 ROS 2ワークスペースの作成

新しいワークスペースを作成する場合は、Qt Creatorの新規プロジェクト画面を開きます。

1. 「他のプロジェクト」から「ROS Workspace」を選択する
2. プロジェクト名を設定する
3. ROSディストリビューションとして`humble`を選択する
4. ビルドシステムとして`colcon`を選択する
5. ワークスペースのプロジェクトパスを指定する

作成された`.workspace`ファイルをプロジェクトルートに配置します。本プロジェクトでは
`ros2_qt_gui_template.workspace`が該当します。

既存の本プロジェクトを使用する場合は、新しいワークスペースを作り直す必要はありません。
`open_qtcreator.sh`から既存の`ros2_qt_gui_template.workspace`を開いてください。

## 8. Qt CreatorのKit設定

Qt Creatorで`.workspace`を開いた後、「プロジェクト」から「ビルドと実行」を開きます。

本環境では次のように設定します。

- `Desktop Qt 6.9.2`のKitを有効にする
- `Python 3.10.12`のKitを無効にする
- C++コンパイラとしてGCCが選択されていることを確認する
- デバッガとしてGDBが選択されていることを確認する

`Python 3.10.12`のKitではC++のCMakeターゲットを適切に実行できません。

Kit名が`Desktop Qt 6.9.2`であっても、本アプリケーションがQt 6へリンクされるわけではありません。
本アプリケーションはCMakeの`find_package(Qt5 ...)`に従ってQt 5を使用します。

## 9. Qt Creatorの実行設定

「プロジェクト」から「実行」を開き、「カスタム実行ファイル」を設定します。

実行ファイル：

```text
/home/ros/ros2_qt_gui_template/install/ros2_qt_gui/lib/ros2_qt_gui/ros2_qt_gui
```

作業ディレクトリ：

```text
/home/ros/ros2_qt_gui_template
```

実行ファイルが未設定の場合は、次のエラーが表示されます。

```text
No executable configured in the custom run configuration.
```

ソースを変更した後は、Qt Creatorでビルドしてから実行します。

## 10. 別プロジェクトへの展開

例えば`/home/ros/test_app`へ本構成を展開する場合は、`open_qtcreator.sh`をプロジェクトルートへ
コピーします。

```bash
cp /home/ros/ros2_qt_gui_template/open_qtcreator.sh /home/ros/test_app/
chmod +x /home/ros/test_app/open_qtcreator.sh
/home/ros/test_app/open_qtcreator.sh
```

スクリプトはコピー先をプロジェクトルートとして扱います。プロジェクトルートには`.workspace`ファイルを
1つだけ配置してください。

## 11. トラブルシューティング

### `install/setup.bash`が存在しない

初回ビルドを実行してください。

```bash
source /opt/ros/humble/setup.bash
colcon build
```

### Qt Creatorが見つからない

実行ファイルを明示します。

```bash
QT_CREATOR_EXECUTABLE=/path/to/qtcreator ./open_qtcreator.sh
```

### `AMENT_TRACE_SETUP_FILES: 未割り当ての変数です`

`set -u`が有効な状態でROS 2の`setup.bash`を読み込むと発生します。本プロジェクトの
`open_qtcreator.sh`は、環境読み込み中だけ`set -u`を無効にして対処しています。

### GUIが表示されない

- GUIセッション上で実行していることを確認する
- `DISPLAY`またはWayland環境が利用可能か確認する
- Qt Creatorをターミナルから起動し、Qtのエラー出力を確認する

### Ctrl+Cで終了しない

本アプリケーションはROS 2 shutdownをQtへ伝播し、Qtイベントループを終了する設計です。
古いビルドが実行されていないか確認し、再ビルドしてください。

```bash
colcon build --packages-select ros2_qt_gui
source install/setup.bash
```
