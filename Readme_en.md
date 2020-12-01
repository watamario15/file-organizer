# File Separation Software w/ Win32 API
この文書には[日本語版](README.md)もあります。

This is a simple free software that separates a massive amount of files into folders by set separation unit.

# System Requirements
## Windows PC
**Microsoft Windows XP Service Pack 3 or later** is recommended.

## Windows CE
I tested on **SHARP Brain PW-SH1 (Windows Embedded CE 6.0, ARMv5TEJ)**.

I compiled for CPUs that the IDE supports. However, since I don't have other devices, I can't test them.

## Batch File
I tested on **Windows PC**.

# How to run
You can get the executable files at **[Releases](https://github.com/watamario15/File-Separation/releases)**.
## Windows PC
"**FileSeparating.exe**" is the executable file.

Some antimalware software like Avast! Antivirus wrongly detects a software compiled by Borland C++ Compiler as a malware. Since this problem, they might detect "**PF_IA-32(OlderWindows).exe**" as malware. In such cases, please restore from the chest and run. You can check the source code if you desire.

## Windows CE
I put executable files in "**Windows CE**".

"**AppMain.exe**" is for ARMv4I devices. If your device is other than that, select one from "**Other CPU**". Then, run it in a way that your device requires.

This software may crash when you have too many files to separate, but the process usually has finished properly and no problem... I believe.

## Batch File
Run "**FileSeparation.bat**" or "**FileSeparation_en.bat**" in a folder you want to process, and then follow the instructions.

# How to use
1. Enter a full path to put files in order. You can also use the "**...**" button to select a folder.
1. Enter a filename with wildcards. For example, if you want to target all files, enter "**\***", if you want to target only mp3 files, enter "**\*.mp3**".
1. Enter a separation unit.
1. Make sure you entered the correct settings, then press the OK button or Enter key to start the process.

This software launches with your OS's UI language. Since some Windows CE doesn't support the function, Windows CE version software always launches with English. You can switch the language at "**Options -> Language**".

Normally, files will be separated in alphabetical order. You can use the Abort button to stop the process, but **this software doesn't have a feature to set back the moved files. Please use the attached "Sandbox" folder (a folder filled with blank files) to test the behavior before using this software**.

# How to install / uninstall
You don't need to install this software. Please run the executable file directly. You can also uninstall by just deleting the file. This software doesn't use registry or such.

# About source codes
Please refer to the readmes in each project folder.

# Notes
**THE AUTHOR OF THIS SOFTWARE WILL NOT TAKE ANY RESPONSIBILITY FOR ANY DAMAGES BY USING THIS SOFTWARE.**

# Rights
This software is licensed under the **MIT License**. Do not forget to read this super short license before you use/redistribute it.

Author: watamario15, otwthutu15(AtSign)gmail.com

# Release notes
## v1.2 (10/31/2020)
Fixed issues around inputting to edit controls.

Fixed other minor issues.

## v1.1 (9/28/2020)
General release

## v1.1 beta (9/19/2020)
Calculation threads' priority is now below normal also on the PC version.

This software now supports English and Japanese. In this work, I moved strings to the String Table.

Fixed other minor issues and usability.

## v1.02 beta (9/2/2020)
Refreshed the code. Readability may have also be improved.

Implemented all changes in the Windows CE version(v1.01) to the PC version.

Now, also the PC version is compiled in Unicode.

## v1.01 (3/26/2020) // Released only for Windows CE
Now you can use the "Browse for Folder" dialog to select a folder(Only if necessary functions are available in ceshell.dll).

Improved to edit controls get focused.

You can now enter numbers with qwerty... keys.

Added a command bar.

You can now move the focus and start the calculation by the Enter key.

Started to build for CPUs other than ARMv4I.

## v1 (4/21/2019)
First release.