# openfx-test (Docker-based Cross-Compilation)

> [!NOTE]
> このリポジトリのほぼすべては、AI（Antigravity / Google DeepMind）との対話を通じて作成されました。

OpenFX 開発用のテストプラグインプロジェクトです。
Docker コンテナを使用して、Linux (WSL2) 上から Windows (x64) 向けのクロスコンパイルをコマンド一つで行うことができます。

## 🚀 特徴
- **環境構築が不要**: SDK やコンパイラ、ライブラリはすべて Docker イメージ内に完結。
- **高速ビルド**: Ninja + ccache による高速なインクリメンタルビルドに対応。
- **エディタ支援**: コンテナ内のヘッダーをローカルに同期し、IntelliSense (clangd) をフル活用可能。

---

## 📝 サンプルの実装内容
本リポジトリには、すぐに開発を始められるよう以下の実装が含まれています。

- **OpenFX Support Library (C++)**: 
  - C++ でプラグインを開発するための標準的なライブラリを統合。
  - パラメータ定義、UI 表示、ホストとの対話処理の実装例。
- **Blend2D によるグラフィックス描画**:
  - 高速な 2D 描画エンジン `Blend2D` を内包。
  - 出力映像に対して、アンチエイリアスの効いた高品質な図形やテキストを描画する実装例。
- **ラスタ処理の基礎**:
  - Support Library を使用した効率的なピクセル操作（ラスタ処理）の実装サンプル。

---

## 🛠 1. 環境構築 (WSL2 / Linux)

### Docker のインストール
WSL2 (Ubuntu) 上で直接 Docker エンジンを動かすのが最も高速で安定します。

```bash
# 1. Docker のインストール
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# 2. sudo なしで Docker を実行するための設定 (現在のユーザーを docker グループに追加)
sudo usermod -aG docker $USER

# 3. 反映のために一度 WSL を再起動するか、以下のコマンドを実行
newgrp docker
```

### 推奨エディタ設定 (VS Code 系)
快適な AI 駆動開発（補完やジャンプ機能）のために、以下を推奨します。

1.  **VS Code 系のエディタ**（VS Code, Antigravity 等）のインストール。
2.  **[clangd 拡張機能](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd)** のインストール。
    *   ※ AI エディタを使用する場合、`.clangd` によるパス解決が正確なコード生成の助けになります。
    *   ※ Microsoft C/C++ 拡張機能が有効な場合は、`clangd` 優先にするために無効化することをお勧めします。

---

## 🏗 2. ビルド手順

リポジトリをクローンした後、スクリプトを実行するだけでビルドが完了します。

```bash
# 1. ビルド実行 (イメージの作成〜バイナリ生成まで自動)
./scripts/docker-build.sh

# クリーンビルドしたい場合
./scripts/docker-build.sh --clean
```

ビルドが成功すると、**`build/MugPlugin.ofx`** が生成されます。

---

## 💻 3. エディタ設定 (IntelliSense / 補完)

Docker 方式を維持しつつ、VS Code 系エディタ + clangd で補完やエラーチェックを有効にするための設定です。

```bash
# 1. コンテナ内から SDK ヘッダーをローカル (.sdk/) に吸い出し
./scripts/setup-editor.sh
```

**メリット:**
- **補完が効く**: `ofxsCore.h` や `blend2d.h` などの定義が参照可能になります。
- **定義ジャンプ**: SDK 内部のコードまでジャンプして実装を確認できます。
- **エラー解消**: 正しいパス設定（`.clangd`）により、エディタ上の赤線が消えます。

---

## 📂 プロジェクト構成

```text
.
├── CMakeLists.txt        # プロジェクト設定
├── cmake/
│   └── toolchain.cmake    # クロスコンパイル定義
├── docker/
│   ├── Dockerfile         # ビルド環境（SDK/ツール一式）
│   └── entrypoint.sh      # ビルド実行スクリプト
├── scripts/
│   ├── docker-build.sh    # メインビルドスクリプト
│   ├── rebuild_docker_image.sh # 環境リセット用
│   └── setup-editor.sh    # エディタ支援用（ヘッダー同期）
└── src/
    └── main.cc            # プラグインのソースコード
```

---

## 📦 インストール方法 (Windows)

生成された `build/MugPlugin.ofx` を、Windows 側の以下のディレクトリに配置してください。

`C:\Program Files\Common Files\OFX\Plugins\MugPlugin.ofx.bundle\Contents\Win64\`

> [!IMPORTANT]
> ディレクトリ名 `MugPlugin.ofx.bundle` の `MugPlugin` 部分は、バイナリ名（`MugPlugin.ofx`）と一致させる必要があります。

---

## 🛠 開発用コマンド

- **イメージの完全作り直し**:
  `./scripts/rebuild_docker_image.sh`
  (ネットワークエラーなどでイメージが壊れた時や、SDKを更新したい時に使用)
