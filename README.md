# openfx-test

> [!NOTE]
> このリポジトリの内容（ソースコード、ビルド設定、ドキュメント等）のほぼすべては、AI（Antigravity / Google DeepMind）との対話を通じて作成されました。

OpenFX 開発用のテストプラグインプロジェクトです。
Linux 環境から Windows 向けにクロスコンパイルを行い、ビルドの高速化（ccache + Ninja）に対応しています。

## プロジェクトの目的

- **最速の環境構築**: OpenFX 開発において最もハードルが高い「ビルド環境の構築」を、コマンド数行で完結させることを目的としています。
- **AI 駆動開発のベース**: 本プロジェクトには詳細なマニュアルはありません。その代わりに、**AI（ChatGPT, Claude, Gemini 等）にこのリポジトリの構成を読み込ませ、実装の相談や修正の補助を受けること**を前提としたモダンな開発スタイルを推奨しています。

## サンプルの内容

このプロジェクトには、OpenFX プラグイン開発の基礎となる以下の実装が含まれています：

- **画像処理 (Image Processing)**: `OFX::ImageProcessor` を使用した、マルチスレッド対応のピクセル操作（例：グリーンチャンネルの減衰処理）。
- **パラメータ定義 (Parameters)**: スライダーなどの UI コントロールをホスト（DaVinci Resolve 等）に表示させる方法。
- **インタラクト (Interacts)**: ビューア上でのカスタムオーバーレイ表示や、マウス操作によるパラメータ制御の基礎。
- **マルチプラットフォーム対応**: CMake による、環境に依存しないビルド定義。

## 動作環境

- **Build Host**: Linux (Ubuntu 24.04 on WSL2 等で動作確認済み)
- **Target OS**: Windows (x64)
- **Plugin Format**: OpenFX (.ofx)

## 必要ツール

ビルドには以下のツール一式が必要です：

- **LLVM/Clang**: クロスコンパイル用コンパイラ（Clang, LLD, llvm-ar をフル活用）
- **MinGW-w64**: Windows 用のヘッダーおよび標準ライブラリ（ビルド時に Clang が参照）
- **CMake**: ビルドシステム生成
- **Ninja**: 高速なビルド実行エンジン
- **ccache**: コンパイル結果のキャッシュ（再ビルドの高速化）

### 推奨エディタ設定 (VS Code / Antigravity)

快適な開発（精度の高い補完や自動整形）のために、以下の拡張機能の使用を強く推奨します：

- **[clangd](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd)**:
  - Microsoft C/C++ 拡張よりも高速で正確なコード解析・補完を提供します。
  - プロジェクトルートに生成される `.clang-format` を読み込み、Google スタイルでの自動整形を行います。
  - 初回起動時に `clangd` バイナリのダウンロードを求められた場合は `Install` を選択してください。

### インストールコマンド (Ubuntu)

```bash
## 🚀 開発環境 (Docker)

本プロジェクトは Docker を使用したクロスコンパイル環境を提供しています。SDK やコンパイラの個別インストールは不要です。

### 1. 準備
- [Docker](https://docs.docker.com/get-docker/) がインストールされていること。

### 2. ビルド
以下のスクリプトを実行するだけで、Windows 用の `.ofx` バイナリが生成されます。

```bash
./docker-build.sh
```

イメージ名: `resolve-ofx-win64-builder`

## 🛠 手動ビルド (Advanced)
Docker を使用しない場合は、以下の環境が必要です。

- LLVM 18+ (clang, lld)
- MinGW-w64
- OpenFX SDK ( `/opt/ofx-sdk` またはプロジェクト直下)
- Blend2D & AsmJit ( `/opt/3rdparty` またはプロジェクト直下)

```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake
cmake --build build
```

## SDK のセットアップ

このリポジトリには OpenFX SDK 本体は含まれていません。ビルドの前に以下のコマンドを実行して SDK を取得してください。

```bash
# プロジェクトルートで実行
git clone -b OFX_Release_1.5.1 --depth 1 https://github.com/ofxa/openfx.git openfx-OFX_Release_1.5.1
```

ディレクトリ構造が以下のようになれば準備完了です：
```text
openfx-test/
├── openfx-OFX_Release_1.5.1/
│   ├── include/
│   └── Support/
├── main.cc
└── ...
```

プロジェクトルートにある `build.sh` を実行するだけで、自動的に CMake の設定とビルドが行われます。

```bash
chmod +x build.sh
./build.sh
```

ビルドが成功すると、`build/MugPlugin.ofx` が生成されます。

## ビルド成果物とインストール

### 生成ファイル
- **`build/MugPlugin.ofx`**

### インストール方法 (Windows)
生成された `.ofx` ファイルを、Windows 側の以下のディレクトリにコピーしてください（ディレクトリがない場合は作成してください）：

`C:\Program Files\Common Files\OFX\Plugins\MugPlugin.ofx.bundle\Contents\Win64`

> [!IMPORTANT]
> **ディレクトリ名の整合性について**
>
> ディレクトリ名 `MugPlugin.ofx.bundle` の `MugPlugin` の部分は、その中に配置するバイナリファイル名（`MugPlugin.ofx`）と必ず一致させてください。ここが一致していないと、ホストアプリケーションがプラグインを正しく認識できない原因となります。

コピー後、DaVinci Resolve などの OpenFX 対応ホストを起動（または再起動）すると、プラグインが読み込まれます。

## プロジェクト構成

- `main.cc`: プラグインのメインソースコード
- `CMakeLists.txt`: ビルド設定
- `toolchain.cmake`: LLVM クロスコンパイル用設定ファイル
- `3rdparty/`: 外部ライブラリ（Blend2D, AsmJit）のソースコード
- `openfx-OFX_Release_1.5.1/`: OpenFX SDK (Support Library を含む)
- `build.sh`: ビルド実行用ラッパースクリプト

## 高速化の仕組み

- **Ninja (Optional)**: Makefile よりも高速に依存関係をチェックし、並列ビルドを行います。
- **ccache**: 一度コンパイルした結果をキャッシュします。
- **Local 3rdparty**: 外部ライブラリをプロジェクト内に内包することで、ネットワーク環境や CMake のバージョンに左右されない安定したビルドを実現しています。
