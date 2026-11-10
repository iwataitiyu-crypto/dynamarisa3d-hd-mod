# Aim inertia test: start a mission, flick the mouse once, compare frames right after and later.
param([string]$tag, [int]$smoothing)
. "$PSScriptRoot\drive.ps1"
if (-not ("IniX" -as [type])) {
Add-Type @"
using System; using System.Runtime.InteropServices;
public class IniX { [DllImport("kernel32.dll", CharSet=CharSet.Unicode)] public static extern bool WritePrivateProfileString(string s, string k, string v, string f); }
"@
}
$ini = "$GameDir\d3d9mod.ini"
[IniX]::WritePrivateProfileString("Control", "AimSmoothing", "$smoothing", $ini) | Out-Null
[IniX]::WritePrivateProfileString("Debug", "Log", "1", $ini) | Out-Null
Set-Location $GameDir
$p = Start-Process .\dynamarisa.exe -PassThru -WorkingDirectory $PWD
Start-Sleep 18; Focus; Key 0x0D; Start-Sleep 5; Focus; MoveTo 480 437; Click; Start-Sleep 6
Focus; MoveTo 640 400; for ($i = 0; $i -lt 75; $i++) { Click; Start-Sleep -m 600 }
Start-Sleep 2
function Grab($name) {
  $h = GameWnd; $r = New-Object GD+R; [GD]::GetClientRect($h, [ref]$r) | Out-Null
  $pt = New-Object GD+P; [GD]::ClientToScreen($h, [ref]$pt) | Out-Null
  $b = New-Object Drawing.Bitmap 480, 270
  $g = [Drawing.Graphics]::FromImage($b); $g.InterpolationMode = 'HighQualityBilinear'
  $full = New-Object Drawing.Bitmap $r.r, $r.b; [Drawing.Graphics]::FromImage($full).CopyFromScreen($pt.x, $pt.y, 0, 0, $full.Size)
  $g.DrawImage($full, 0, 0, 480, 270); $b.Save("$ShotDir\$name.png"); return $b
}
function Diff($a, $b) {
  $s = 0; for ($y = 20; $y -lt 200; $y += 4) { for ($x = 0; $x -lt 480; $x += 4) { $c1 = $a.GetPixel($x, $y); $c2 = $b.GetPixel($x, $y); $s += [math]::Abs($c1.R - $c2.R) + [math]::Abs($c1.G - $c2.G) } }; return $s
}
$results = @()
for ($t = 0; $t -lt 3; $t++) {
  Focus; Start-Sleep -m 300
  $a = Grab "$tag-a$t"
  [GD]::mouse_event(1, 400, 0, 0, 0)   # one relative flick to the right
  Start-Sleep -m 60
  $b = Grab "$tag-b$t"
  Start-Sleep -m 700
  $c = Grab "$tag-c$t"
  $results += "flick $t : before->after60ms=$(Diff $a $b)  after60ms->after760ms=$(Diff $b $c)"
}
Get-Process dynamarisa | Stop-Process -Force
$results
Select-String -Path d3d9mod.log -Pattern 'aim' | % { $_.Line }
