# Building Nexo Desk on Windows x64

Based on `docs/building-win.md` from AyuGram, with the steps actually used.

## Requirements

- Windows 10/11 x64, about 80 GB free disk space.
- Visual Studio 2026 Build Tools with: MSVC v14.44 toolset (x64), ATL and MFC
  v14.44, Windows 11 SDK 10.0.26100. Installed with:

  ```
  winget install --id Microsoft.VisualStudio.BuildTools --override "--quiet --wait --norestart --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.14.44.17.14.x86.x64 --add Microsoft.VisualStudio.Component.VC.14.44.17.14.ATL --add Microsoft.VisualStudio.Component.VC.14.44.17.14.MFC --add Microsoft.VisualStudio.Component.Windows11SDK.26100 --add Microsoft.VisualStudio.Component.VC.CMake.Project --includeRecommended"
  ```
- Git, Python 3.10+ (3.12 works).
- Your own `api_id` and `api_hash` from https://my.telegram.org.

## Folder layout

```
C:\TBuild\
  tdesktop\                 this repository (cloned with --recursive)
  ThirdParty\  Libraries\   created by the prepare step
  api_credentials.local     TDESKTOP_API_ID=... and TDESKTOP_API_HASH=..., never committed
```

## Steps

All commands run in a terminal initialized with:

```
"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" -vcvars_ver=14.44
```

1. Build third-party libraries (several hours the first time):

   ```
   cd /d C:\TBuild
   tdesktop\Telegram\build\prepare\win.bat silent
   ```

   `C:\TBuild\run_prepare.cmd` does the same with the right environment.

2. Configure, with auto-update off (see `network.md`):

   ```
   cd /d C:\TBuild\tdesktop\Telegram
   configure.bat x64 -D TDESKTOP_API_ID=<id> -D TDESKTOP_API_HASH=<hash> -D DESKTOP_APP_DISABLE_AUTOUPDATE=ON -D CMAKE_MSVC_DEBUG_INFORMATION_FORMAT=
   ```

   The last option (empty value) turns off debug info in Release. With the
   default (`/Z7`, embedded in every `.obj`) the build folder grew past
   33 GB and filled the disk; with `ProgramDatabase` the shared compiler
   PDB hit its size limit (error C1090, code 12). Release builds made this
   way have no symbols for crash reports.

3. Build:

   ```
   cmake --build ..\out --config Release --target Telegram
   ```

   The result is `C:\TBuild\tdesktop\out\Release\NexoDesk.exe`.

## Known build issues

- Release debug info does not fit: `/Z7` objects take 20-350 MB each and
  `/Zi` overflows the compiler PDB. Configure with
  `-D CMAKE_MSVC_DEBUG_INFORMATION_FORMAT=` (see step 2).

- MSYS2 no longer has `mingw-w64-x86_64-*` packages used by AyuGram's
  `prepare.py`; fixed in this repository by switching to `ucrt64`, same as
  upstream Telegram Desktop.

## Regenerating icons

```
cd Telegram\Resources\art\nexo
python make_icons.py
```
