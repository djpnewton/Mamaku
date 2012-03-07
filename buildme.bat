:: setup separate object root

set separate_object_root=TRUE
set _NTTREE=%CD%
set OBJECT_ROOT=%CD%\obj

set BINPLACE_EXCLUDE_FILE=%BASEDIR%\bin\symbad.txt
set BINPLACE_LOG=%_NTTREE%\build_logs\binplace.log
set BINPLACE_PDB_DLL=mspdb80.dll

set NO_BINPLACE=

set NTDBGFILES=1
set NTDBGFILES_PRIVATE=0

if Not Exist %_NTTREE% md %_NTTREE%
if Not Exist %_NTTREE%\build_logs md %_NTTREE%\build_logs
if Not Exist %OBJECT_ROOT% md %OBJECT_ROOT%
if Not Exist bin md bin

:: build files
build -wgc

:: extra copy coinstaller from redist
copy %BASEDIR%\redist\wdf\x86\WdfCoInstaller01009.dll bin\

:: create bat file for easier installation of driver
echo %BASEDIR%\tools\devcon\i386\devcon.exe update mamaku.inf bthenum\{00001124-0000-1000-8000-00805f9b34fb} > bin\install_driver.bat
