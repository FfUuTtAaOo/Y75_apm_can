# ============================================================
# Y77 一键配置 + 构建脚本 (ARM GCC / MinGW Makefiles)
#
# 用法:
#   .\build.ps1            # 配置(若需要) + 构建
#   .\build.ps1 -Clean     # 删除 build 缓存后从零配置 + 构建
#
# 说明:
#   - 编译器路径/自检模式已在 CMakeLists.txt 中固化, 这里只需指定 -G。
#   - Windows 上不带 -G 时 CMake 默认选 Visual Studio 生成器,
#     无法配合 arm-none-eabi-gcc, 因此统一用 MinGW Makefiles。
# ============================================================
param(
    [switch]$Clean
)
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

if ($Clean -or -not (Test-Path "build/CMakeCache.txt")) {
    Write-Host "==> 删除旧缓存并重新配置..." -ForegroundColor Cyan
    Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
    cmake -S . -B build -G "MinGW Makefiles"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "CMake 配置失败 (exit $LASTEXITCODE)" -ForegroundColor Red
        exit $LASTEXITCODE
    }
}

Write-Host "==> 构建..." -ForegroundColor Cyan
cmake --build build
exit $LASTEXITCODE
