# openfx-test

OpenFX 開発用のテストプラグインプロジェクトです。
Linux 環境から Windows 向けにクロスコンパイルを行い、ビルドの高速化（ccache + Ninja）に対応しています。

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
