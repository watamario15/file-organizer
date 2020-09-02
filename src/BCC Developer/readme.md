# 超大量ファイル整理 Ver. 1.02β BCC Developer版ソースコード
Windows PC用のソースコードです。

# ビルド時の要件
このプロジェクトは、**BCC Developer**で作成されています。また、製作者はコンパイラとして**Borland C++ Compiler 5.5**を使用しています。

なお、このコンパイラは超古いのでWindows XPにも普通に対応している他、64bitプログラムのビルドは行えません。

# 注釈
ソースコードは、このファイルと同階層にある**main.cpp**です。

実行可能バイナリは、デバッグビルドは**Debug**フォルダ、リリースビルドは**Release**フォルダに**FileSeparating.exe**として生成されます。

なお、プロジェクトはUnicodeビルドを行う設定になっていますが、wWinMainをWinMainに変更してプロジェクトの設定を変更すれば、ANSIビルドも可能です(ANSIビルドならextern "C"は抜いてもかまいません)。