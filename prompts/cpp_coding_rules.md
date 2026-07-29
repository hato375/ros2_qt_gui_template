# C++ コーディングルール

> 参考：Google C++ Style Guide / Stroustrupスタイル  
> 作成日：2026-06-24  
> 対象：ydsライブラリ および 個人プロジェクト  
> C++バージョン：C++14（Visual Studio 2017）

---

## 1. 命名規則

| 対象 | スタイル | 例 |
|------|----------|----|
| クラス名 | PascalCase | `MyClass` |
| 関数・メソッド名 | camelCase | `doSomething()` |
| ローカル変数・引数 | camelCase | `localVar`, `userId` |
| メンバー変数 | camelCase + 末尾 `_` | `memberValue_` |
| 定数・エニュメレータ | kPascalCase | `kMaxSize` |
| 名前空間 | lowercase 一語 | `myns`, `app` |

### 略語の扱い
- 略語は **先頭のみ大文字**、それ以降は小文字とする

```cpp
// Good
std::string parseHttpResponse();
int userId;
std::string parseUrl();

// Bad
std::string parseHTTPResponse();
int userID;
```

---

## 2. フォーマット

| 項目 | 設定 |
|------|------|
| インデント | タブ |
| ブレース `{` の位置 | K&Rスタイル（すべて同行） |
| 1行の最大文字数 | 120文字 |

### ブレースのスタイル（K&R）

```cpp
void MyClass::doSomething() {
	if (condition_) {
		process();
	} else {
		fallback();
	}
}
```

---

## 3. ヘッダー・インクルード

| 項目 | 設定 |
|------|------|
| ヘッダーガード | `#pragma once` |
| インクルード順序 | 下記参照 |

### インクルード順序

1. 対応するヘッダーファイル
2. C システムヘッダー
3. C++ 標準ライブラリヘッダー
4. その他ライブラリ・プロジェクト内ヘッダー

各グループの間は空行で区切る。

```cpp
#pragma once

#include "my_class.h"           // 1. 対応ヘッダー

#include <cstdint>              // 2. Cシステムヘッダー

#include <string>               // 3. C++標準ライブラリ
#include <vector>

#include <somelib/foo.h>        // 4. その他ライブラリ
#include "yds/log/logger.h"     // 4. プロジェクト内
```

### 禁止事項
- ヘッダーファイル内での `using namespace` は禁止

```cpp
// Bad（ヘッダー内）
using namespace yds::log;

// Good（.cpp や関数スコープ内のみ許可）
void myFunc() {
	using namespace yds::log;
	// ...
}
```

---

## 4. コメント・ドキュメント

| 項目 | 設定 |
|------|------|
| 形式 | `///` + Doxygen（`@param`、`@return` など） |
| 対象 | すべての公開関数・クラスに必須 |
| 記述言語 | 日本語 |

```cpp
/// @brief HTTP レスポンスを解析する
/// @param response 解析対象のレスポンスオブジェクト
/// @return 解析結果の文字列
std::string parseHttpResponse(const HttpResponse& response) noexcept;
```

> 運用してみてコメントが煩雑に感じる場合は、複雑な関数・クラスのみに絞ることを検討する。

---

## 5. スマートポインタ・メモリ管理

| 項目 | 設定 |
|------|------|
| 基本方針 | `unique_ptr` を基本、`shared_ptr` は必要なときのみ |
| 所有権なしの参照 | `const T*` / `const T&` を使ってよい |
| 生成方法 | `make_unique` / `make_shared` を必ず使う |
| `new` / `delete` の直接使用 | 原則禁止 |

```cpp
// Good：所有権あり → make_unique
auto obj = std::make_unique<MyClass>();

// Good：所有権なし・参照渡し → const rawポインタ
void doSomething(const MyClass* obj) noexcept;

// Good：共有所有権が必要な場合のみ
auto shared = std::make_shared<MyClass>();

// Bad
MyClass* obj = new MyClass();
```

---

## 6. クラス設計

| 項目 | 設定 |
|------|------|
| コピーコンストラクタ・代入演算子 | 必要なものだけ定義、不要なものは `= delete` |
| 複数コンストラクタ | 委譲コンストラクタで初期化を一元化 |
| 仮想デストラクタ | 継承を使う場合は必ず定義 |
| `explicit` | 引数が1つのコンストラクタには必ず付ける |

```cpp
class MyClass {
public:
	// すべてのメンバーをここで初期化
	MyClass() : value_(0), name_(""), ready_(false) {}

	// 他のコンストラクタは委譲する
	explicit MyClass(int value) : MyClass() {
		value_ = value;
	}
	explicit MyClass(std::string name) : MyClass() {
		name_ = std::move(name);
	}

	// 継承する場合は仮想デストラクタを必ず定義
	virtual ~MyClass() = default;

	// コピー不要なら明示的に禁止
	MyClass(const MyClass&) = delete;
	MyClass& operator=(const MyClass&) = delete;

private:
	int value_;
	std::string name_;
	bool ready_;
};
```

---

## 7. エラーハンドリング

| 項目 | 設定 |
|------|------|
| 基本方針 | 戻り値でエラーを通知する |
| 例外 | ライブラリが投げる場合はその場で `try/catch` して呼び出し元に逃さない |
| `noexcept` | 例外を投げない関数には必ず付与する |
| 前提条件違反の検出 | `assert` をくどくならない程度に使用 |

```cpp
// Good：戻り値でエラーを通知
int loadConfig(const std::string& path) noexcept;

// Good：ライブラリの例外はその場で吸収
int MyClass::readFile(const std::string& path) noexcept {
	try {
		// 例外を投げる可能性のあるライブラリの呼び出し
		externalLib::read(path);
	} catch (const std::exception& e) {
		// ログ等で記録し、エラーコードを返す
		return kErrorFileRead;
	}
	return kSuccess;
}

// Good：前提条件の検出
void process(int value) noexcept {
	assert(value >= 0);
	// ...
}
```

---

## 8. ソースコードの階層化

| 項目 | 設定 |
|------|------|
| 基本構造 | モジュール単位でディレクトリを分ける |
| ヘッダーとソース | 同一ディレクトリに配置 |
| トップ名前空間 | `src/yds/` を基点とする |
| サブモジュール | 親子関係が明確な場合のみ階層を深くする |

```
src/yds/
├── log/            # ログ関係
├── file/           # ファイル入出力関係
├── math/           # 数学関係
├── graphics2d/     # 2Dグラフィック関係
├── graphics3d/     # 3Dグラフィック関係
└── device/         # HWデバイス関係
    └── camera/     # カメラ関係（deviceの子として明確）
```

---

## 9. 名前空間

| 項目 | 設定 |
|------|------|
| 基本方針 | ディレクトリ構造と完全対応させる |
| 呼び出し側の短縮 | 関数スコープ内の `using` やエイリアスで対応 |
| ヘッダーでの `using namespace` | 禁止 |

```cpp
// ディレクトリ構造と名前空間の対応
// src/yds/log/     → namespace yds::log
// src/yds/device/camera/ → namespace yds::device::camera

// 呼び出し側で短縮したい場合
namespace cam = yds::device::camera;  // エイリアス（ヘッダーでも可）
cam::capture();

void myFunc() {
	using namespace yds::log;  // usingはスコープ内のみ許可
	write("メッセージ");
}
```

---

## 10. ファイル名・ファイル構成

| 項目 | 設定 |
|------|------|
| ファイル名スタイル | snake_case（`my_class.cpp` / `my_class.h`） |
| クラスとファイルの対応 | 1クラス = 1ファイルを基本 |
| 構造体 | 関連するものは1ヘッダーにまとめてOK |
| ヘルパークラス | 主クラスと同一ファイルにまとめてOK |

```cpp
// types.h：関連する構造体はまとめてOK
namespace yds::math {

struct Vector2 { float x, y; };
struct Vector3 { float x, y, z; };
struct Vector4 { float x, y, z, w; };

} // namespace yds::math
```

```cpp
// logger.h：主クラス専用のヘルパークラスは同一ファイルにまとめてOK
namespace yds::log {

// ヘルパークラス（Logger専用）
class LogFormatter {
public:
	std::string format(const std::string& msg) noexcept;
};

// 主クラス
class Logger {
public:
	void log(int level, const std::string& msg) noexcept;
private:
	LogFormatter formatter_;
};

} // namespace yds::log
```

---

## 改訂履歴

| 日付 | 内容 |
|------|------|
| 2026-06-24 | 初版作成 |
