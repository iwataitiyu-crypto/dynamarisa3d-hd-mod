# Test helper: send keys / mouse to the game window and take screenshots (dot-source it)
Add-Type -AssemblyName System.Drawing
if (-not ("GD" -as [type])) {
Add-Type @"
using System; using System.Runtime.InteropServices;
public class GD {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte sc, int f, IntPtr e);
  [DllImport("user32.dll")] public static extern void mouse_event(int f, int x, int y, int d, IntPtr e);
  [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint c, uint t);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out R r);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref P p);
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
  public struct R { public int l,t,r,b; } public struct P { public int x,y; }
}
"@
}
[GD]::SetProcessDPIAware() | Out-Null
# the repository is expected to be a subfolder of the game folder
$GameDir = (Resolve-Path "$PSScriptRoot\..\..\..").Path
$ShotDir = "$env:TEMP\dynamarisa_test"; New-Item -ItemType Directory -Force $ShotDir | Out-Null
function GameWnd { (Get-Process dynamarisa -ErrorAction SilentlyContinue | select -First 1).MainWindowHandle }
function Focus { [GD]::SetForegroundWindow((GameWnd)) | Out-Null; Start-Sleep -m 200 }
function Key($vk, $hold = 80) { $sc = [GD]::MapVirtualKey($vk, 0); [GD]::keybd_event($vk, $sc, 0, 0); Start-Sleep -m $hold; [GD]::keybd_event($vk, $sc, 2, 0); Start-Sleep -m 250 }
function AltEnter { [GD]::keybd_event(0x12, 0x38, 0, 0); [GD]::keybd_event(0x0D, 0x1C, 0, 0); Start-Sleep -m 60; [GD]::keybd_event(0x0D, 0x1C, 2, 0); [GD]::keybd_event(0x12, 0x38, 2, 0); Start-Sleep -m 500 }
# move the mouse to game coordinates (1280x720 space)
function MoveTo($gx, $gy) {
  $h = GameWnd; $r = New-Object GD+R; [GD]::GetClientRect($h, [ref]$r) | Out-Null
  $p = New-Object GD+P; $p.x = [int]($gx * $r.r / 1280); $p.y = [int]($gy * $r.b / 720)
  [GD]::ClientToScreen($h, [ref]$p) | Out-Null; [GD]::SetCursorPos($p.x, $p.y) | Out-Null; Start-Sleep -m 150
}
function Click { [GD]::mouse_event(2, 0, 0, 0, 0); Start-Sleep -m 60; [GD]::mouse_event(4, 0, 0, 0, 0); Start-Sleep -m 250 }
function Shot($name, [switch]$small) {
  $h = GameWnd; $r = New-Object GD+R; [GD]::GetClientRect($h, [ref]$r) | Out-Null
  $p = New-Object GD+P; [GD]::ClientToScreen($h, [ref]$p) | Out-Null
  $bmp = New-Object Drawing.Bitmap $r.r, $r.b
  $g = [Drawing.Graphics]::FromImage($bmp); $g.CopyFromScreen($p.x, $p.y, 0, 0, $bmp.Size)
  $out = "$ShotDir\$name.png"; $bmp.Save($out)
  if ($small -and $r.r -gt 1280) {
    $s = New-Object Drawing.Bitmap 1280, 720; $g2 = [Drawing.Graphics]::FromImage($s)
    $g2.InterpolationMode = 'HighQualityBicubic'; $g2.DrawImage($bmp, 0, 0, 1280, 720); $s.Save("$ShotDir\$name-small.png")
  }
  "client $($r.r)x$($r.b) at $($p.x),$($p.y)"
}
function Crop($name, $x, $y, $w, $hgt) {
  $b = [Drawing.Bitmap]::FromFile("$ShotDir\$name.png")
  $c = $b.Clone([Drawing.Rectangle]::new($x, $y, $w, $hgt), $b.PixelFormat); $c.Save("$ShotDir\$name-crop.png"); $b.Dispose()
}
