# Simple Serial Bridge for Arduino Mega and ESP32-CAM
# Usage: .\simple_bridge.ps1 COM11 COM7

param(
    [string]$ArduinoPort = "COM11",
    [string]$ESP32Port = "COM7"
)

Write-Host "=== Arduino Mega + ESP32-CAM Bridge ===" -ForegroundColor Green
Write-Host "Arduino Mega: $ArduinoPort" -ForegroundColor Yellow
Write-Host "ESP32-CAM: $ESP32Port" -ForegroundColor Yellow
Write-Host "Press Ctrl+C to stop" -ForegroundColor Red
Write-Host ""

# Check if ports exist
$ports = Get-PnpDevice -Class "Ports" | Where-Object { $_.FriendlyName -like "*$ArduinoPort*" -or $_.FriendlyName -like "*$ESP32Port*" }
if ($ports.Count -lt 2) {
    Write-Host "Error: One or both COM ports not found!" -ForegroundColor Red
    Write-Host "Available ports:" -ForegroundColor Yellow
    Get-PnpDevice -Class "Ports" | ForEach-Object { Write-Host "  $($_.FriendlyName)" }
    exit 1
}

# Function to extract command from Arduino output
function ExtractCommand($line) {
    if ($line -match "\[To ESP32-CAM\]: (.+)") {
        return $matches[1]
    }
    return $null
}

# Simple message forwarding
Write-Host "Starting bridge..." -ForegroundColor Green
Write-Host "Copy messages from Arduino Mega Serial Monitor to ESP32-CAM Serial Monitor" -ForegroundColor Cyan
Write-Host ""

Write-Host "Instructions:" -ForegroundColor Yellow
Write-Host "1. Open Arduino Mega Serial Monitor (COM11, 9600 baud)" -ForegroundColor White
Write-Host "2. Open ESP32-CAM Serial Monitor (COM7, 115200 baud)" -ForegroundColor White
Write-Host "3. Copy messages starting with '[To ESP32-CAM]:' from Arduino to ESP32" -ForegroundColor White
Write-Host "4. Test your sensors and watch the communication!" -ForegroundColor White
Write-Host ""

Write-Host "Example messages to copy:" -ForegroundColor Cyan
Write-Host "  [To ESP32-CAM]: FINGERPRINT_READY" -ForegroundColor Gray
Write-Host "  [To ESP32-CAM]: VIBRATION_ALERT" -ForegroundColor Gray
Write-Host "  [To ESP32-CAM]: ACCESS_GRANTED:1" -ForegroundColor Gray
Write-Host "  [To ESP32-CAM]: DOOR_UNLOCKED" -ForegroundColor Gray
Write-Host ""

Write-Host "Bridge is ready! Start testing your sensors..." -ForegroundColor Green
Write-Host "Press any key to exit..." -ForegroundColor Yellow
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown") 