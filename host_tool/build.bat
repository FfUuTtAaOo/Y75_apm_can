@echo off
chcp 65001 >nul
cd /d %~dp0
echo ================================================
echo   六维力上位机 打包脚本 (PyInstaller)
echo ================================================

echo [1/3] 安装依赖（清华镜像加速）...
pip install -i https://pypi.tuna.tsinghua.edu.cn/simple PySide6==6.11.2 pyqtgraph==0.14.0 pyinstaller
if errorlevel 1 (
    echo 依赖安装失败，请检查网络后重试。
    pause
    exit /b 1
)

echo [2/3] 打包 exe ...
if exist ControlCAN.dll (
    pyinstaller --noconfirm --clean --onefile --windowed ^
        --name SixAxisForce_Host ^
        --collect-all pyqtgraph ^
        --add-binary "ControlCAN.dll;." ^
        main.py
) else (
    pyinstaller --noconfirm --clean --onefile --windowed ^
        --name SixAxisForce_Host ^
        --collect-all pyqtgraph ^
        main.py
)
if errorlevel 1 (
    echo 打包失败，请把上面错误信息发给开发者。
    pause
    exit /b 1
)

echo [3/3] 复制说明文件 ...
copy README.md dist\README.md >nul 2>&1

echo.
echo ================================================
echo   完成！可执行文件: dist\SixAxisForce_Host.exe
echo   使用方式：
echo     1. 将 ControlCAN.dll 放到 exe 同目录（若本目录有会自动复制）
echo     2. 双击运行，无硬件可先选"模拟模式"
echo ================================================
pause
