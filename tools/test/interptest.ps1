# Interpolation test: enter the first mission, then rotate the camera with the mouse while pressing F12
# (the mod saves the next 24 frames to capture*, see d3d9mod.ini [Debug] Log=1)
. "$PSScriptRoot\drive.ps1"
Set-Location $GameDir
Remove-Item -Recurse -Force capture* -ErrorAction SilentlyContinue
$p = Start-Process .\dynamarisa.exe -PassThru -WorkingDirectory $PWD
Start-Sleep 18; Focus; Key 0x0D; Start-Sleep 5; Focus; MoveTo 480 437; Click; Start-Sleep 6
Focus; MoveTo 640 400; for ($i = 0; $i -lt 75; $i++) { Click; Start-Sleep -m 600 }
Start-Sleep 2; Focus
function Turn($ms, $dx, $f12) {
  $end = (Get-Date).AddMilliseconds($ms); $pressed = $false
  while ((Get-Date) -lt $end) {
    [GD]::mouse_event(1, $dx, 0, 0, 0)
    if ($f12 -and -not $pressed -and ((Get-Date) -gt $end.AddMilliseconds(-$ms + 300))) { Key 0x7B 30; $pressed = $true }
    Start-Sleep -m 4
  }
}
Turn 1500 6 $true      # slow pan right
Start-Sleep -m 500
Turn 1500 -14 $true    # faster pan left
Start-Sleep -m 500
Turn 1500 3 $true      # very slow pan
Shot interp_game | Out-Null
Get-Process dynamarisa | Stop-Process -Force
Get-ChildItem -Directory capture* | % { "$($_.Name): $((Get-ChildItem $_.FullName).Count) frames" }
