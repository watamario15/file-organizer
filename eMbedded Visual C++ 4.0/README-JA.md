# 超大量ファイル整理 eMbedded Visual C++ 4.0 版ソースコード

[English](README.md) / **日本語**

**Windows CE** 用のソースコードです。

## ビルド時の要件

このプロジェクトは **eMbedded Visual C++ 4.0** でビルド可能です。[Brain Wiki](https://brain.fandom.com/ja/wiki/Microsoft_eMbedded_Visual_C%2B%2B_4.0) などを参考にインストールしてください。

## 注釈

ソースコードは `1.cpp`、リソーススクリプトは `resource.rc`、アイコンは `app.ico` です。eMbedded Visual C++ 4.0 であれば `FileSeparation.vcw` を開くことでプログラムの編集及びビルドが可能です。

実行可能バイナリは、デバッグビルドは `ARMV4IDbg` フォルダ、リリースビルドは `ARMV4IRel` フォルダに `AppMain.exe` として生成されます。その他 CPU 用のバイナリも `(CPU 名)Dbg` や `(CPU 名)Rel` の中に `FS_(CPU 名).exe` として生成されます｡
