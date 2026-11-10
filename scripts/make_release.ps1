# Build the mod and create the distribution zip (release\DynamarisaHD_v<Version>.zip).
# The zip contains only the files to drop into the game folder, at its root:
#   d3d9.dll, dynamarisa_hd_config.exe, d3d9mod.ini, README_HDMOD.txt
param([string]$Version = "1.0.0")
$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot\..").Path

# clean build so the zip always matches the current source
Remove-Item -Recurse -Force "$root\out" -ErrorAction SilentlyContinue
cmd /c "`"$root\build.bat`""
if ($LASTEXITCODE -ne 0) { throw "build failed" }

$stage = "$root\release\stage"
Remove-Item -Recurse -Force $stage -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item "$root\out\d3d9.dll", "$root\out\dynamarisa_hd_config.exe" $stage
# the ini stays Shift-JIS: the game reads and the config tool writes it through the Windows profile API
Copy-Item "$root\dist\d3d9mod.ini" $stage
# the readme gets a UTF-8 BOM so that every Notepad version opens it correctly
$text = [IO.File]::ReadAllText("$root\dist\README_HDMOD.txt", [Text.Encoding]::UTF8)
[IO.File]::WriteAllText("$stage\README_HDMOD.txt", $text, (New-Object Text.UTF8Encoding $true))

$zip = "$root\release\DynamarisaHD_v$Version.zip"
Remove-Item -Force $zip -ErrorAction SilentlyContinue
Compress-Archive -Path "$stage\*" -DestinationPath $zip
Remove-Item -Recurse -Force $stage
Get-Item $zip | Select-Object FullName, Length
