param(
    [Parameter(Mandatory=$true)][string]$Container,
    [Parameter(Mandatory=$true)][string]$Sysroot,
    [Parameter(Mandatory=$true)][string]$HandlerSource,
    [Parameter(Mandatory=$true)][string]$Zig,
    [Parameter(Mandatory=$true)][string]$Python,
    [Parameter(Mandatory=$true)][string]$Inspector,
    
    [Parameter(Mandatory=$true)][string]$Output
)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$utf8=[Text.UTF8Encoding]::new($false)
function Digest([byte[]]$Bytes) { [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($Bytes)).ToLowerInvariant() }
function File-Digest([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
$containerPath=(Resolve-Path -LiteralPath $Container).Path
if ((File-Digest $containerPath) -ne '763a85698771ef2bb69fb29eb6a1e8b0a32b063aee1910194865f70ea6c1005d') {throw 'Unexpected engineering source container'}
$sysrootPath=(Resolve-Path -LiteralPath $Sysroot).Path
$factoryBinary=Join-Path $sysrootPath 'lge/app_rw/smartlink/carplay/carplay'
$factoryScript=Join-Path $sysrootPath 'lge/app_rw/smartlink/carplay/carplay.sh'
$osRelease=Join-Path $sysrootPath 'etc/os-release'
$expected=@{
    CARPLAY_SHA='d9c06c9f0aff2a098ec9da09a63d4339de40e4f57e7f46a173706ccd04d15e2b'
    STARTUP_SHA='aca72269589a14c38ceef28bd8d1a316964b7baf3490365ea2b85ea2528c7325'
    OS_SHA='97d0b16193a2ef43a0e10138127e05376644ec36be4d199b9c059032eee89284'
}
if ((File-Digest $factoryBinary) -ne $expected.CARPLAY_SHA -or (File-Digest $factoryScript) -ne $expected.STARTUP_SHA -or (File-Digest $osRelease) -ne $expected.OS_SHA) {throw 'Factory baseline differs from the confirmed vehicle probe'}
$outputPath=[IO.Path]::GetFullPath($Output)
if (Test-Path -LiteralPath $outputPath) {throw 'Output exists; do not overwrite a reviewed package'}
$work=Join-Path (Split-Path -Parent $outputPath) ('oneshot-build-'+[Guid]::NewGuid().ToString('N'))
$null=New-Item -ItemType Directory -Path $work
$recovery=Join-Path $PSScriptRoot '../recovery'
$guardSource=(Resolve-Path -LiteralPath (Join-Path $recovery 'file-guard-v31.c')).Path
$guard=Join-Path $work 'file-guard'
$handler=Join-Path $work 'altscreen_handler.so'
$patched=Join-Path $work 'carplay-managed'
& (Join-Path $PSScriptRoot 'build-target-windows.ps1') -Zig $Zig -Source $HandlerSource -Sysroot $Sysroot -Inspector $Inspector -Output $handler
if ($LASTEXITCODE -ne 0) {throw 'Handler build failed'}
$probeInfo=& node $Inspector $handler | ConvertFrom-Json
$mapExports=@('AES_CBCFrame_Init','AirPlayCopyServerInfo','AirPlayReceiverSessionSetHomeKitSecurityContext','AirPlayReceiverSessionSetSecurityInfo','AirPlayReceiverSessionSetup','AirPlayReceiverSessionTearDown','ScreenStreamCreate','ScreenStreamSetWidthHeight','ScreenStreamStop')
if ((@($probeInfo.exports.name | Sort-Object) -join ',') -cne (($mapExports | Sort-Object) -join ',')) {throw 'Unexpected map handler exports'}
& $Zig cc -target aarch64-linux-musl -static -O2 -Wall -Wextra -Werror -fno-sanitize=all $guardSource -o $guard
if ($LASTEXITCODE -ne 0) {throw 'Metadata helper build failed'}
$guardInfo=& node $Inspector $guard | ConvertFrom-Json
if ($LASTEXITCODE -ne 0 -or $guardInfo.architecture -ne 'AArch64' -or @($guardInfo.needed).Count -ne 0) {throw 'Helper is not standalone AArch64'}

$dependency='/lge/app_rw/smartlink/carplay/.moi3-altscreen-managed-v31/altscreen_handler.so'
$patchInfo=& $Python (Join-Path $PSScriptRoot 'add-handler-dependency.py') $factoryBinary $patched $dependency | ConvertFrom-Json
if($LASTEXITCODE -ne 0 -or $patchInfo.sourceSha256 -ne $expected.CARPLAY_SHA){throw 'Executable-copy preparation failed'}
$copyInfo=& node $Inspector $patched | ConvertFrom-Json
if($LASTEXITCODE -ne 0 -or $copyInfo.needed[0] -cne $dependency){throw 'Unexpected executable dependencies'}
$expected.GUARD_SHA=File-Digest $guard
$expected.HANDLER_SHA=File-Digest $handler
$expected.PATCH_SHA=File-Digest $patched
function Render([string]$Name,[hashtable]$Values){
 $text=[IO.File]::ReadAllText((Join-Path $recovery $Name)).Replace("`r`n","`n").Replace('@ROOT@','')
 foreach($key in $Values.Keys){$text=$text.Replace("@$key@",[string]$Values[$key])}
 if($text -match '@[A-Z_]+@'){throw "Unrendered token: $Name"}
 return ,$utf8.GetBytes($text)
}
$controller=Render 'managed-v31-controller.sh.in' $expected
$expected.CONTROLLER_SHA=Digest $controller
$boot=Render 'managed-v31-boot.sh.in' $expected
$expected.BOOT_SHA=Digest $boot
$null=New-Item -ItemType Directory -Path $outputPath
$sourceHashes=[ordered]@{}
foreach($item in @((Get-ChildItem -LiteralPath (Split-Path $HandlerSource) -File), (Get-Item -LiteralPath $guardSource), (Get-Item -LiteralPath (Join-Path $recovery 'file-guard.c')), (Get-Item -LiteralPath (Join-Path $recovery 'resource-audit-v17.h')), (Get-Item -LiteralPath (Join-Path $recovery 'video-export-v16.h')), (Get-ChildItem -LiteralPath $recovery -Filter 'managed-v31-*.sh.in'), (Get-Item -LiteralPath (Join-Path $PSScriptRoot 'add-handler-dependency.py'))) | ForEach-Object {$_}){
 if($item.Extension -in '.c','.h','.in','.py'){$sourceHashes[$item.Name]=File-Digest $item.FullName}
}
foreach($mode in @('install','restore')){
 $label=if($mode -eq 'install'){'INSTALL_ALTSCREEN'}else{'RESTORE_STOCK'}
 $expected.MODE=$mode
 $files=[ordered]@{
  'fix_hw_partnumber.sh'=(Render 'managed-v31-entry.sh.in' $expected)
  'payload/file-guard'=[IO.File]::ReadAllBytes($guard)
  'payload/factory-carplay.sh'=[IO.File]::ReadAllBytes($factoryScript)
  'payload/factory-carplay'=[IO.File]::ReadAllBytes($factoryBinary)
 }
 if($mode -eq 'install'){
  $files['payload/carplay-managed']=[IO.File]::ReadAllBytes($patched)
  $files['payload/altscreen_handler.so']=[IO.File]::ReadAllBytes($handler)
  $files['payload/managed-controller.sh']=$controller
  $files['payload/managed-boot.sh']=$boot
 }
 $marker=if($mode -eq 'install'){'ALTSCREEN_MANAGED_INSTALL.txt'}else{'ALTSCREEN_MANAGED_RESTORE.txt'}
 $files[$marker]=$utf8.GetBytes($label+"`n")
 $instructions=@"
V31 $label - MOI3 LG 0841 only. Private vehicle package; do not publish.

Copy the CONTENTS of this folder to the USB root. Use exactly one operation
marker, never both install and restore. Keep managed-v31-stock-backup safe.
Select only MIB3OI_POST_SCRIPT in the engineering update; complete Resume and
return to normal mode. Keep USB connected through the update's reboot.

INSTALL: Start from stock after a normal reboot with the previous test USB
removed. The installer verifies firmware and originals, saves factory bytes
and metadata on the unit and USB, and enables persistent startup. Reboot
normally to activate; the USB is not required afterward. Check the USB
managed-v31-setup.txt for OPERATION COMPLETE. Validate a second normal boot.
RESTORE: Use this set via the same update process. It disables autostart,
restores the saved factory startup and unmounts our own verified RAM copy.
Complete the update and normal reboot, then verify factory maps, CarPlay and
factory turn-by-turn. Unknown or changed files are preserved; a pending/error
report requires inspection. Verified factory backups remain inactive.

A 12-second renewable RAM permission and independent 20-second watchdog
protect encoder ownership. No ten-minute test cutoff. Diagnostics are bounded
512 KiB per log. No continuous video capture or packet-audit process is added.
Safe area x464,y56,width352,height368; showETA=yes. V29 packet pacing retained.

RESTORATION SCOPE: This removes our managed map integration. It is NOT a full
vehicle/firmware image backup. The signed engineering trigger's previously
accepted four speech-data resets occur before our scripts and are not covered
by the saved originals. Uninstall before changing the factory firmware.
First persistent startup has not yet been validated in the vehicle.
"@
 $files['READ-ME-FIRST.txt']=$utf8.GetBytes($instructions.Replace("`r`n","`n"))
 $hashes=[ordered]@{};foreach($name in $files.Keys){$hashes[$name]=Digest $files[$name]}
 $manifest=[ordered]@{
  format='moi3-altscreen-managed-v31';operation=$mode;software='0841';persistentAutostart=($mode -eq 'install')
  factoryHashes=$expected.Clone();files=$hashes;sourceHashes=$sourceHashes
  layoutPolicy=@{safeX=464;safeY=56;safeWidth=352;safeHeight=368;showETA=$true;factoryGuidanceSuppression=$false}
  transportBaseline='V29';auxiliaryMaxFPS=30;packetBurstPacing=@{bitsPerSecond=40000000;maximumBatchDatagrams=4;maximumAccessUnitSendMicroseconds=75000}
  runtimePolicy=@{leaseSeconds=12;watchdogSeconds=20;fixedSessionCutoff=$false;logBytes=524288;continuousPacketAudit=$false;videoCapture=$false;factoryExecutableOverwritten=$false}
  containerSha256=(File-Digest $containerPath);hardwareValidated=$false;knownSpeechResets=$true;completeVehicleRollback=$false
 }
 $files['build-manifest.json']=$utf8.GetBytes(($manifest|ConvertTo-Json -Depth 10))
 $zipPath=Join-Path $outputPath ($label+'.zip')
 $sourceZip=[IO.Compression.ZipFile]::OpenRead($containerPath);$outZip=[IO.Compression.ZipFile]::Open($zipPath,[IO.Compression.ZipArchiveMode]::Create)
 try{
  foreach($e in $sourceZip.Entries){
   if($e.FullName -eq 'fix_hw_partnumber.sh'){continue}
   if($e.FullName -notmatch '^(Data|Meta)/' -or $e.FullName -match '(^|/)\.\.(/|$)'){throw 'Unexpected carrier path'}
   $target=$outZip.CreateEntry($e.FullName);$a=$e.Open();$b=$target.Open();try{$a.CopyTo($b)}finally{$a.Dispose();$b.Dispose()}
  }
  foreach($name in $files.Keys){$target=$outZip.CreateEntry($name);$stream=$target.Open();try{$stream.Write($files[$name],0,$files[$name].Length)}finally{$stream.Dispose()}}
 }finally{$sourceZip.Dispose();$outZip.Dispose()}
 [IO.Compression.ZipFile]::ExtractToDirectory($zipPath,(Join-Path $outputPath $label))
 Get-FileHash -LiteralPath $zipPath -Algorithm SHA256
}
Write-Output 'Private paired packages built. Verify before USB deployment. No vehicle activation occurred.'
