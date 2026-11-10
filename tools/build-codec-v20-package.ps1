param([Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference='Stop'
$workspace=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$utf8=[Text.UTF8Encoding]::new($false)
function Digest([byte[]]$bytes){[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()}
function Hash([string]$path){(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()}
$container=Join-Path $workspace 'firmware-audit/LG-wipe-FAZIT.zip'
if((Hash $container) -ne '763a85698771ef2bb69fb29eb6a1e8b0a32b063aee1910194865f70ea6c1005d'){throw 'Unknown carrier'}
$expected=@{CARPLAY_SHA='d9c06c9f0aff2a098ec9da09a63d4339de40e4f57e7f46a173706ccd04d15e2b';STARTUP_SHA='aca72269589a14c38ceef28bd8d1a316964b7baf3490365ea2b85ea2528c7325';OS_SHA='97d0b16193a2ef43a0e10138127e05376644ec36be4d199b9c059032eee89284'}
$sysroot=Join-Path $workspace 'firmware-audit/atlas-0841-sysroot'
foreach($p in @(@('lge/app_rw/smartlink/carplay/carplay','CARPLAY_SHA'),@('lge/app_rw/smartlink/carplay/carplay.sh','STARTUP_SHA'),@('etc/os-release','OS_SHA'))){if((Hash (Join-Path $sysroot $p[0])) -ne $expected[$p[1]]){throw 'Unknown baseline'}}
$outputPath=[IO.Path]::GetFullPath($Output);if(Test-Path -LiteralPath $outputPath){throw 'Output already exists'}
$work=Join-Path $workspace ('firmware-audit/codec-v20-build-'+[guid]::NewGuid().ToString('N'));New-Item -ItemType Directory -Path $work | Out-Null
$zig=Join-Path $workspace 'tools-local/zig/zig-x86_64-windows-0.17.1/zig.exe'
$env:ZIG_GLOBAL_CACHE_DIR=Join-Path $work '.zig-global-cache';$env:ZIG_LOCAL_CACHE_DIR=Join-Path $work '.zig-local-cache'
$player=Join-Path $work 'synthetic-player';$guard=Join-Path $work 'file-guard'
& $zig cc -target aarch64-linux-musl -static -O2 -Wall -Wextra -Werror -fno-sanitize=all (Join-Path $PSScriptRoot 'synthetic-player-v20.c') -o $player
if($LASTEXITCODE -ne 0){throw 'Player build failed'}
& $zig cc -target aarch64-linux-musl -static -O2 -Wall -Wextra -Werror -fno-sanitize=all (Join-Path $workspace 'recovery/file-guard.c') -o $guard
if($LASTEXITCODE -ne 0){throw 'Metadata helper build failed'}
foreach($elf in @($player,$guard)){$info=& node (Join-Path $workspace 'firmware-audit/inspect-elf.mjs') $elf | ConvertFrom-Json;if($LASTEXITCODE -ne 0 -or $info.architecture -ne 'AArch64' -or @($info.needed).Count){throw 'Invalid standalone helper'}}
$expected.PLAYER_SHA=Hash $player;$expected.GUARD_SHA=Hash $guard
$files=[ordered]@{'payload/synthetic-player'=[IO.File]::ReadAllBytes($player);'payload/file-guard'=[IO.File]::ReadAllBytes($guard)}
$names=@('baseline-ref1-p.synthetic','high-ref1-p.synthetic','high-ref4-p.synthetic','high-intra.synthetic');$i=0
foreach($name in $names){$i++;$bytes=[IO.File]::ReadAllBytes((Join-Path $workspace "firmware-audit/codec-refresh-audit/synthetic-controls-validated/$name"));$files["payload/$name"]=$bytes;$expected["CLIP${i}_SHA"]=Digest $bytes}
function Render([string]$name){$text=[IO.File]::ReadAllText((Join-Path $workspace "recovery/$name")).Replace("`r`n","`n");foreach($key in $expected.Keys){$text=$text.Replace("@$key@",[string]$expected[$key])};$text=$text.Replace('@ROOT@','');if($text -match '@[A-Z0-9_]+@'){throw "Unrendered template: $name"};return ,$utf8.GetBytes($text)}
$files['payload/controller.sh']=Render 'codec-v20-controller.sh.in';$expected.CONTROLLER_SHA=Digest $files['payload/controller.sh']
$files['payload/oneshot-boot.sh']=Render 'codec-v20-boot.sh.in';$expected.BOOT_SHA=Digest $files['payload/oneshot-boot.sh']
$files['fix_hw_partnumber.sh']=Render 'codec-v20-usb.sh.in'
$files['CODEC_TEST_V20.txt']=$utf8.GetBytes("Parked-only synthetic codec comparison. No CarPlay connection required.`n")
$files['READ-ME-FIRST.txt']=$utf8.GetBytes([IO.File]::ReadAllText((Join-Path $workspace 'firmware-audit/CODEC-V20-TEST-INSTRUCTIONS.md')))
$hashes=[ordered]@{};foreach($name in $files.Keys){$hashes[$name]=Digest $files[$name]}
$sources=[ordered]@{player=Hash (Join-Path $PSScriptRoot 'synthetic-player-v20.c');transport=Hash (Join-Path $PSScriptRoot 'synthetic-transport-v20.h');guard=Hash (Join-Path $workspace 'recovery/file-guard.c');resource=Hash (Join-Path $workspace 'recovery/resource-audit-v17.h');export=Hash (Join-Path $workspace 'recovery/video-export-v16.h');controller=Hash (Join-Path $workspace 'recovery/codec-v20-controller.sh.in');boot=Hash (Join-Path $workspace 'recovery/codec-v20-boot.sh.in');entry=Hash (Join-Path $workspace 'recovery/codec-v20-usb.sh.in')}
$manifest=[ordered]@{format='moi3-codec-test-v20';unit='3G5035842B';software='0841';purpose='parked-synthetic-codec-comparison';factoryExecutableChanged=$false;startupRestoresBeforeMonitor=$true;newRawCapture=$false;maximumPlaybackSeconds=32;monitorRuntimeMaxSeconds=180;trigger='USB absent then reinserted after completed updater/Resume';factoryHashes=$expected;sourceHashes=$sources;files=$hashes;hardwareValidated=$false;completeVehicleRollback=$false;knownSpeechResets=$true}
$files['build-manifest.json']=$utf8.GetBytes(($manifest | ConvertTo-Json -Depth 7))
Add-Type -AssemblyName System.IO.Compression.FileSystem
$src=[IO.Compression.ZipFile]::OpenRead($container);$zip=[IO.Compression.ZipFile]::Open($outputPath,[IO.Compression.ZipArchiveMode]::Create)
try{foreach($entry in $src.Entries){if($entry.FullName -eq 'fix_hw_partnumber.sh'){continue};if($entry.FullName -notmatch '^(Data|Meta)/' -or $entry.FullName -match '(^|/)\.\.(/|$)'){throw 'Unexpected carrier path'};$e=$zip.CreateEntry($entry.FullName);$a=$entry.Open();$b=$e.Open();try{$a.CopyTo($b)}finally{$a.Dispose();$b.Dispose()}}
foreach($name in $files.Keys){$e=$zip.CreateEntry($name);$s=$e.Open();try{$s.Write($files[$name],0,$files[$name].Length)}finally{$s.Dispose()}}}finally{$src.Dispose();$zip.Dispose()}
Write-Output "LOCAL PACKAGE: $outputPath";Get-FileHash -LiteralPath $outputPath
