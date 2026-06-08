# TuneBar 编译和烧录脚本
# 使用方法: 在 PowerShell 中运行 .\build_and_upload.ps1

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "TuneBar - 编译和烧录脚本" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 设置工作目录
$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptPath

Write-Host "工作目录: $scriptPath" -ForegroundColor Green
Write-Host ""

# 禁用 uv，使用 pip
$env:PLATFORMIO_NO_UV = "1"
Write-Host "已设置 PLATFORMIO_NO_UV=1" -ForegroundColor Yellow
Write-Host ""

# 尝试使用 platformio 编译
Write-Host "开始编译..." -ForegroundColor Cyan
try {
    python -m platformio run
    Write-Host ""
    Write-Host "✅ 编译成功！" -ForegroundColor Green
} catch {
    Write-Host ""
    Write-Host "❌ 编译失败！" -ForegroundColor Red
    Write-Host "请在 VS Code 中使用 PlatformIO 扩展进行编译" -ForegroundColor Yellow
    exit 1
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "下一步：" -ForegroundColor Yellow
Write-Host "1. 在 VS Code 中打开项目" -ForegroundColor White
Write-Host "2. 点击 PlatformIO 工具栏的 Upload 按钮烧录" -ForegroundColor White
Write-Host "3. 或者在 VS Code 终端运行: platformio run --target upload" -ForegroundColor White
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
