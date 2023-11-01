# File Organizer Source Code for eMbedded Visual C++ 4.0

**English** / [日本語](README-JA.md)

This source code is for **Windows CE**.

## Requirements

You can build this project with **eMbedded Visual C++ 4.0**.

## Notes

The source code is `1.cpp`, the resource script is `resource.rc`, and the icon is `app.ico`. You can edit and build by opening `FileSeparation.vcw` in eMbedded Visual C++ 4.0.

The Armv4I executable binary file will be generated as `AppMain.exe` in the `ARMV4IDbg` folder for debug builds, and the `ARMV4IRel` folder for release builds. Files for other CPUs will also be generated as `FS_(Target CPU).exe` in `(Target CPU)Dbg` folder for debug builds, and `(Target CPU)Rel` folder for release builds.
