@echo off
set QT_DIR=D:\Users\angel.guan\Qt\6.10.2\msvc2022_64\bin
set TARGET_DIR=d:\Users\angel.guan\github\other\vnote\build\bin\Release

echo Copying required Qt DLLs...

copy "%QT_DIR%\Qt6Core.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6Core5Compat.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6Gui.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6Widgets.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6Network.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6Sql.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6Svg.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6PrintSupport.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6WebChannel.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6WebEngineCore.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6WebEngineQuick.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6WebEngineWidgets.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6WebEngineQuickDelegatesQml.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6WebSockets.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6Qml.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6Quick.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6QuickWidgets.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6OpenGL.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6OpenGLWidgets.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6Positioning.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6Concurrent.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6LabsFolderListModel.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6LabsQmlModels.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6QmlModels.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6LabsSettings.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6LabsPlatform.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6SvgWidgets.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6QmlCore.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6QmlCompiler.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6QmlWorkerScript.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6QmlLocalStorage.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6QmlXmlListModel.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6QmlMeta.dll" "%TARGET_DIR%\" /Y >nul
copy "%QT_DIR%\Qt6QmlNetwork.dll" "%TARGET_DIR%\" /Y >nul

echo Copying QtWebEngineProcess.exe...
copy "%QT_DIR%\QtWebEngineProcess.exe" "%TARGET_DIR%\" /Y >nul

echo Copying platforms plugin...
if not exist "%TARGET_DIR%\platforms" mkdir "%TARGET_DIR%\platforms"
copy "%QT_DIR%\..\plugins\platforms\qwindows.dll" "%TARGET_DIR%\platforms\" /Y >nul

echo Copying imageformats plugins...
if not exist "%TARGET_DIR%\imageformats" mkdir "%TARGET_DIR%\imageformats"
copy "%QT_DIR%\..\plugins\imageformats\*.dll" "%TARGET_DIR%\imageformats\" /Y >nul 2>nul

echo Copying sqldrivers plugins...
if not exist "%TARGET_DIR%\sqldrivers" mkdir "%TARGET_DIR%\sqldrivers"
copy "%QT_DIR%\..\plugins\sqldrivers\*.dll" "%TARGET_DIR%\sqldrivers\" /Y >nul 2>nul

echo Copying tls plugins...
if not exist "%TARGET_DIR%\tls" mkdir "%TARGET_DIR%\tls"
copy "%QT_DIR%\..\plugins\tls\*.dll" "%TARGET_DIR%\tls\" /Y >nul 2>nul

echo Copying styles plugins...
if not exist "%TARGET_DIR%\styles" mkdir "%TARGET_DIR%\styles"
copy "%QT_DIR%\..\plugins\styles\*.dll" "%TARGET_DIR%\styles\" /Y >nul 2>nul

echo Copying iconengines plugins...
if not exist "%TARGET_DIR%\iconengines" mkdir "%TARGET_DIR%\iconengines"
copy "%QT_DIR%\..\plugins\iconengines\*.dll" "%TARGET_DIR%\iconengines\" /Y >nul 2>nul

echo Done!
