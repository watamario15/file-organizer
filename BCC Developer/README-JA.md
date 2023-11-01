# 超大量ファイル整理 BCC Developer 版ソースコード

[English](README.md) / **日本語**

**Windows PC** 用のソースコードです。

## ビルド時の要件

このプロジェクトは **BCC Developer** 用で、コンパイラは **Borland C++ Compiler 5.5** を想定しています。「**プロジェクト設定->リソース**」のインクルードパスを環境に合わせて書き換えて使用してください。

## 注釈

ソースコードは `1.cpp`、リソーススクリプトは `resource.rc`、アイコンは `app.ico` です。BCC Developer であれば `FileSeparation.bdp` を開くことでプログラムの編集及びビルドが可能です。日本語を含むパスに配置するとビルド時に問題が発生することがあるので、極力避けてください。

実行可能バイナリは、デバッグビルドは `Debug` フォルダ、リリースビルドは `Release` フォルダに `FileSeparation.exe` として生成されます。
