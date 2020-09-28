#-------------------------------------------------------
# BCC Developer 1.2.21
# Copyright (C) 2003 jun_miura@hi-ho.ne.jp
#-------------------------------------------------------
.autodepend
CC=bcc32
RC=brc32
CFLAG=-W  -3 -O2 -w- -AT -pc -H- -k -b  -WU
OUTDIR=-nRelease
CINCS=
TARGET=Release\FileSeparation.exe
SRC1="D:\Programming\File Separation\BCC Developer\1.cpp"
OBJ1=Release\1.obj
RC1="D:\Programming\File Separation\BCC Developer\resource.rc"
RES1=Release\resource.res
RESINCS=-iC:\Users\watamario\Documents\bcc\borland\bcc55\Include

TARGET: $(TARGET)

$(TARGET): $(OBJ1) $(RES1)
    $(CC) $(CFLAG) -e$(TARGET) $(OBJ1)
    $(RC) $(RES1) $(TARGET)

$(OBJ1): $(SRC1)
    $(CC) $(CFLAG) $(OUTDIR) $(CINCS) -c $(SRC1)

$(RES1): $(RC1)
    $(RC) $(RESINCS) -r -fo$(RES1) $(RC1)
