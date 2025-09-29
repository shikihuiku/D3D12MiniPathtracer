# D3D12 Mini Pathtracer

Direct3D 12を使用したミニマルなパストレーサーの実装プロジェクトです。

## 概要

このプロジェクトは、Windows環境でDirect3D 12 APIを使用してパストレーシングレンダラーを実装することを目的としています。現在は基本的なD3D12の初期化とImGUIによるデバッグ情報の表示が実装されています。

## 必要環境

- Windows 10/11
- Visual Studio 2022
- Windows SDK (Direct3D 12対応版)
- DirectX 12対応GPU

## ビルド方法

### Visual Studio 2022を使用する場合

1. `D3D12MiniPathtracer.sln`をVisual Studio 2022で開く
2. ビルド構成（Debug/Release）を選択
3. ビルド実行（F7）

### コマンドラインからビルドする場合

PowerShellで以下のコマンドを実行：

```powershell
# Debugビルド
.\build.ps1 -Configuration Debug

# Releaseビルド
.\build.ps1 -Configuration Release

# クリーンビルド
.\build.ps1 -Configuration Debug -Clean
```

### VSCodeを使用する場合

1. VSCodeでプロジェクトフォルダを開く
2. `Ctrl+Shift+B`でビルドタスクを選択
3. F5で実行（デバッグなし）

## プロジェクト構成

```
D3D12MiniPathtracer/
├── src/                    # ソースコード
│   ├── Application.cpp     # メインアプリケーションクラス
│   ├── Application.h
│   ├── Helper.h           # ユーティリティマクロ
│   ├── ImGuiManager.cpp   # ImGUI管理クラス
│   ├── ImGuiManager.h
│   └── Main.cpp           # エントリーポイント
├── shaders/               # シェーダーファイル（今後実装）
├── assets/                # アセットファイル（今後実装）
├── thirdparty/           # 外部ライブラリ
│   └── imgui/            # ImGUIライブラリ
├── bin/                  # 実行ファイル出力先
│   ├── Debug/
│   └── Release/
└── obj/                  # 中間ファイル
```

## 現在の実装状況

### 完了済み

- [x] D3D12デバイスの初期化
- [x] スワップチェーンの作成（トリプルバッファリング）
- [x] コマンドキューとコマンドリストの作成
- [x] RTVディスクリプタヒープの作成
- [x] フレーム同期オブジェクトの作成
- [x] ウィンドウリサイズへの対応
- [x] ImGUIの統合
- [x] パフォーマンス統計の表示（FPS、フレーム時間など）
- [x] 別スレッドでのウィンドウメッセージ処理

### 今後の実装予定

- [ ] コンピュートシェーダーの実装
- [ ] パストレーシングカーネルの実装
- [ ] シーンデータの管理
- [ ] カメラ制御
- [ ] マテリアルシステム
- [ ] テクスチャサポート
- [ ] 環境マップ
- [ ] デノイザーの実装

## コーディング規約

- 変数名: camelCase
- 関数名: PascalCase
- クラス名: PascalCase
- namespace名: snake_case（小文字）
- 定数名: UPPER_SNAKE_CASE
- ファイル名: PascalCase.cpp/PascalCase.h
- インデント: スペース4つ
- COMオブジェクト: ComPtrで管理
- エラーハンドリング: ThrowIfFailedマクロを使用（例外は使用しない）assertを併用

## デバッグ機能

- D3D12デバッグレイヤー（Debug構成時に自動的に有効化）
- PIXによるGPUキャプチャのサポート
- ImGUIによるリアルタイムパフォーマンス表示
- 200ms以上のGPU待機時のデバッグ出力

## ライセンス

MIT

## 謝辞

- [Dear ImGui](https://github.com/ocornut/imgui) - デバッグUI表示に使用