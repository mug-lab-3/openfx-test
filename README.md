# openfx-test

OpenFX 開発用のテストプラグインプロジェクトです。
Linux 環境から Windows 向けにクロスコンパイルを行い、ビルドの高速化（ccache + Ninja）に対応しています。

## 動作環境

- **Build Host**: Linux (Ubuntu 24.04 推奨)
- **Target OS**: Windows (x64)
- **Plugin Format**: OpenFX (.ofx)

## 必要ツール

ビルドには以下のツール一式が必要です：

- **LLVM/Clang**: クロスコンパイル用コンパイラ
- **CMake**: ビルドシステム生成
- **Ninja**: 高速なビルド実行エンジン
- **ccache**: コンパイル結果のキャッシュ（再ビルドの高速化）

### インストールコマンド (Ubuntu)

```bash
sudo apt update
sudo apt install -y clang lld cmake ninja-build ccache
```

## SDK のセットアップ

このリポジトリには OpenFX SDK 本体は含まれていません。ビルドの前に以下の準備が必要です。

1. [OpenFX 公式リポジトリ](https://github.com/ofxa/openfx) 等から SDK をダウンロードします。
2. ダウンロードした SDK を、フォルダ名を **`openfx-OFX_Release_1.5.1`** に変更した上で、このプロジェクトのルートディレクトリに配置してください。

ディレクトリ構造のイメージ：
```text
openfx-test/
├── openfx-OFX_Release_1.5.1/  <-- ここに配置
│   ├── include/
│   └── Support/
├── main.cpp
└── ...
```

プロジェクトルートにある `build.sh` を実行するだけで、自動的に CMake の設定とビルドが行われます。

```bash
chmod +x build.sh
./build.sh
```

ビルドが成功すると、`build/MugPlugin.ofx` が生成されます。

## プロジェクト構成

- `main.cpp`: プラグインのメインソースコード
- `CMakeLists.txt`: ビルド設定
- `openfx-OFX_Release_1.5.1/`: OpenFX SDK (Support Library を含む)
- `build.sh`: ビルド実行用ラッパースクリプト

## 高速化の仕組み

- **Ninja**: Makefile よりも高速に依存関係をチェックし、並列ビルドを行います。
- **ccache**: 一度コンパイルした結果をキャッシュします。`rm -rf build` でディレクトリを削除しても、2回目以降のコンパイルは一瞬で終わります。
