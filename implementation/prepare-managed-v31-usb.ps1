param([ValidateSet('INSTALL_ALTSCREEN','RESTORE_STOCK')][string]$Mode='INSTALL_ALTSCREEN')
$ErrorActionPreference='Stop';Add-Type -AssemblyName System.IO.Compression.FileSystem
$workspace=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'));$usb=[IO.Path]::GetFullPath('D:\')
if($usb -cne 'D:\' -or (Get-Volume -DriveLetter D).FileSystem -ne 'FAT32'){throw 'Unexpected USB target'}
function Hash([string]$p){(Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant()}
$old=Get-Content -LiteralPath (Join-Path $usb 'build-manifest.json') -Raw | ConvertFrom-Json
if($old.format -notin @('moi3-altscreen-map-v30','moi3-altscreen-managed-v31')){throw 'Expected returned V30 or paired V31 USB'}
foreach($p in $old.files.psobject.Properties){if((Hash (Join-Path $usb $p.Name)) -cne $p.Value){throw "Collection payload changed: $($p.Name)"}}
$packageRoot=Join-Path $workspace 'firmware-audit/V31-USB-READY'
$verification=Get-Content -LiteralPath (Join-Path $packageRoot 'verification.json') -Raw | ConvertFrom-Json
$record=@($verification.packages | Where-Object {$_.package -ceq $Mode})
if($record.Count -ne 1 -or !$verification.pairedBaselineIdentical){throw 'Verified paired package record missing'}
$archive=Join-Path $packageRoot ($Mode+'.zip')
if((Hash $archive) -cne $record[0].sha256){throw 'Selected verified package changed'}
$backup=Join-Path $workspace ('firmware-audit/usb-before-v31-'+[guid]::NewGuid().ToString('N'));New-Item -ItemType Directory -Path $backup | Out-Null
$items=@(Get-ChildItem -LiteralPath $usb -Force | Where-Object {$_.Name -notin @('System Volume Information','.Spotlight-V100','.Trashes') -and !$_.Name.StartsWith('._')})
foreach($item in $items){if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Unexpected reparse point on USB'}}
foreach($item in $items){if($item.PSIsContainer){foreach($child in Get-ChildItem -LiteralPath $item.FullName -Force -Recurse){if($child.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Unexpected nested reparse point'}}}}
foreach($item in $items){Copy-Item -LiteralPath $item.FullName -Destination (Join-Path $backup $item.Name) -Recurse}
foreach($item in $items){$files=if($item.PSIsContainer){@(Get-ChildItem -LiteralPath $item.FullName -File -Recurse)}else{@($item)};foreach($f in $files){$rel=$f.FullName.Substring($usb.Length);if((Hash $f.FullName) -cne (Hash (Join-Path $backup $rel))){throw 'Backup hash mismatch'}}}
$zip=[IO.Compression.ZipFile]::OpenRead($archive)
try{
 $new=@($zip.Entries.FullName)
 $remove=@('managed-v31-setup.txt','managed-v31-boot-status.txt','managed-v31-handler.log','ALTSCREEN_TEST_ONCE.txt','ALTSCREEN_MANAGED_INSTALL.txt','ALTSCREEN_MANAGED_RESTORE.txt','altscreen-handler.log','network-status.txt','output-diagnostics.txt','packet-audit.txt','packet-audit-errors.txt','carplay-journal.txt','altscreen-setup.txt','boot-status.txt','codec-player.log','packet-timing.log','v24-recovery-intake.txt','v24-recovered-reports.json','codec-v20-intake.txt','boot-status-v20.txt','codec-v20-journal.txt','codec-player-v20.log','factory-carplay.sh.backup')
 foreach($p in $old.files.psobject.Properties){if($p.Name -notin $new){$remove+=$p.Name}}
 foreach($name in $remove){
  $target=[IO.Path]::GetFullPath((Join-Path $usb $name));$parent=[IO.Path]::GetDirectoryName($target).TrimEnd('\')
  if(!$target.StartsWith($usb,[StringComparison]::OrdinalIgnoreCase) -or $parent -notin @($usb.TrimEnd('\'),(Join-Path $usb 'payload'))){throw 'Removal escaped USB root/payload'}
  if(Test-Path -LiteralPath $target){if((Get-Item -LiteralPath $target).PSIsContainer){throw 'No directory deletion permitted'};Remove-Item -LiteralPath $target}
 }
 [IO.Compression.ZipFile]::ExtractToDirectory($archive,$usb,$true)
 foreach($e in $zip.Entries){if($e.FullName.EndsWith('/')){continue};$s=$e.Open();try{$expected=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($s)).ToLowerInvariant()}finally{$s.Dispose()};if((Hash (Join-Path $usb $e.FullName)) -cne $expected){throw 'USB archive readback mismatch'}}
}finally{$zip.Dispose()}
$manifest=Get-Content -LiteralPath (Join-Path $usb 'build-manifest.json') -Raw | ConvertFrom-Json
$expectedMode=if($Mode -eq 'INSTALL_ALTSCREEN'){'install'}else{'restore'}
$expectedMarker=if($expectedMode -eq 'install'){'ALTSCREEN_MANAGED_INSTALL.txt'}else{'ALTSCREEN_MANAGED_RESTORE.txt'}
if($manifest.format -ne 'moi3-altscreen-managed-v31' -or $manifest.operation -ne $expectedMode){throw 'Wrong V31 operation'}
$markers=@(Get-ChildItem -LiteralPath $usb -File -Filter 'ALTSCREEN*.txt')
if($markers.Count -ne 1 -or $markers[0].Name -cne $expectedMarker){throw 'Mixed/missing USB modes'}
foreach($name in @('COLLECT_MAP_V24.txt','COLLECT_CODEC_V20.txt','CODEC_TEST_V20.txt','CODEC_TEST_V21.txt','CODEC_TEST_V22.txt','CODEC_TEST_V23.txt')){if(Test-Path -LiteralPath (Join-Path $usb $name)){throw 'Old test marker remains'}}
foreach($p in $manifest.files.psobject.Properties){if((Hash (Join-Path $usb $p.Name)) -cne $p.Value){throw 'Final hash mismatch'}}
Write-Output "USB VERIFIED: V31 $Mode; all archive entries and declared files verified"
Write-Output "PREVIOUS USB ARCHIVED AND VERIFIED: $backup"

[ordered]@{operation=$expectedMode;usb='D:';packageSha256=$record[0].sha256;backup=$backup;archiveReadbackVerified=$true;vehicleActivated=$false}|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $workspace 'firmware-audit/V31-USB-READY/usb-ready.json') -Encoding utf8
