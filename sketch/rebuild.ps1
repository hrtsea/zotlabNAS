<#
Complete Rebuild Script for ESP32-S3 Project
This script performs a full clean rebuild and upload of the project.
Usage: Run in PowerShell 7 (pwsh) from any location.
#>

# Set environment variable to disable uv (use pip instead)
$env:PLATFORMIO_NO_UV = '1'

# Change to project directory
Set-Location -Path "f:\nasmonitor\TuneBar-main\sketch"

Write-Host "=== Starting Complete Rebuild ===" -ForegroundColor Cyan

# Step 1: Clean build artifacts
Write-Host "Cleaning build artifacts..." -ForegroundColor Yellow
pio run --target clean
if ($LASTEXITCODE -ne 0) {
    Write-Host "Clean failed. Proceeding anyway..." -ForegroundColor Red
}

# Step 2: Optional: Remove .pio directory for full dependency reinstall
# Uncomment if you want to force reinstall of all dependencies
# Write-Host "Removing .pio directory..." -ForegroundColor Yellow
# Remove-Item -Path ".pio" -Recurse -Force -ErrorAction SilentlyContinue

# Step 3: Build the project
Write-Host "Building project..." -ForegroundColor Green
pio run --environment esp32-s3-devkitc1-n16r8
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed! Exiting." -ForegroundColor Red
    exit $LASTEXITCODE
}

# Step 4: Upload to device
Write-Host "Uploading to ESP32-S3 (COM5)..." -ForegroundColor Magenta
pio run --target upload --environment esp32-s3-devkitc1-n16r8
if ($LASTEXITCODE -ne 0) {
    Write-Host "Upload failed!" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "=== Rebuild and Upload Complete ===" -ForegroundColor Green
Write-Host "Device should now be running the new firmware." -ForegroundColor Green