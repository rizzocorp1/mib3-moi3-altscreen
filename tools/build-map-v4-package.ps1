param(
    [Parameter(Mandatory=$true)][string]$Container,
    [Parameter(Mandatory=$true)][string]$Sysroot,
    [Parameter(Mandatory=$true)][string]$HandlerSource,
    [Parameter(Mandatory=$true)][string]$Zig,
    [Parameter(Mandatory=$true)][string]$Python,
    [Parameter(Mandatory=$true)][string]$Inspector,
    [Parameter(Mandatory=$true)][string]$Instructions,
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
$guardSource=(Resolve-Path -LiteralPath (Join-Path $recovery 'file-guard.c')).Path
$guard=Join-Path $work 'file-guard'
$handler=Join-Path $work 'altscreen_handler.so'
$patched=Join-Path $work 'carplay-once'
& (Join-Path $PSScriptRoot 'build-target-windows.ps1') -Zig $Zig -Source $HandlerSource -Sysroot $Sysroot -Inspector $Inspector -Output $handler
if ($LASTEXITCODE -ne 0) {throw 'Handler build failed'}
$probeInfo=& node $Inspector $handler | ConvertFrom-Json
$mapExports=@('AES_CBCFrame_Init','AirPlayCopyServerInfo','AirPlayReceiverSessionSetHomeKitSecurityContext','AirPlayReceiverSessionSetSecurityInfo','AirPlayReceiverSessionSetup','AirPlayReceiverSessionTearDown','ScreenStreamCreate','ScreenStreamSetWidthHeight','ScreenStreamStop')
if ((@($probeInfo.exports.name | Sort-Object) -join ',') -cne (($mapExports | Sort-Object) -join ',')) {throw 'Unexpected map handler exports'}
& $Zig cc -target aarch64-linux-musl -static -O2 -Wall -Wextra -Werror -fno-sanitize=all $guardSource -o $guard
if ($LASTEXITCODE -ne 0) {throw 'Metadata helper build failed'}
$guardInfo=& node $Inspector $guard | ConvertFrom-Json
if ($LASTEXITCODE -ne 0 -or $guardInfo.architecture -ne 'AArch64' -or @($guardInfo.needed).Count -ne 0) {throw 'Helper is not standalone AArch64'}
$dependency='/lge/app_rw/smartlink/carplay/.moi3-altscreen-map-v4/altscreen_handler.so'
$patchInfo=& $Python (Join-Path $PSScriptRoot 'add-handler-dependency.py') $factoryBinary $patched $dependency | ConvertFrom-Json
if ($LASTEXITCODE -ne 0 -or $patchInfo.sourceSha256 -ne $expected.CARPLAY_SHA) {throw 'Executable-copy preparation failed'}
$copyInfo=& node $Inspector $patched | ConvertFrom-Json
if ($LASTEXITCODE -ne 0 -or $copyInfo.architecture -ne 'AArch64' -or $copyInfo.needed[0] -ne $dependency) {throw 'Unexpected copied-executable dependencies'}
$expected.GUARD_SHA=File-Digest $guard
$expected.HANDLER_SHA=File-Digest $handler
$expected.PATCH_SHA=File-Digest $patched
function Render([string]$Path,[hashtable]$Values) {
    $text=[IO.File]::ReadAllText($Path).Replace("`r`n","`n")
    foreach($key in $Values.Keys){$text=$text.Replace("@$key@",[string]$Values[$key])}
    $text=$text.Replace('@ROOT@','').Replace('@LOG_ITERATIONS@','150')
    if($text -match '@[A-Z_]+@'){throw "Unrendered token in $Path"}
    return ,$utf8.GetBytes($text)
}
$collector=$utf8.GetBytes([IO.File]::ReadAllText((Join-Path $recovery 'map-v4-log-collector.sh')).Replace("`r`n","`n"))
$expected.COLLECTOR_SHA=Digest $collector
$boot=Render (Join-Path $recovery 'map-v4-boot.sh.in') $expected
$expected.BOOT_SHA=Digest $boot
$entry=Render (Join-Path $recovery 'map-v4-usb.sh.in') $expected
$files=[ordered]@{
    'fix_hw_partnumber.sh'=$entry
    'payload/managed-log-collector.sh'=$collector
    'payload/oneshot-boot.sh'=$boot
    'payload/file-guard'=[IO.File]::ReadAllBytes($guard)
    'payload/carplay-once'=[IO.File]::ReadAllBytes($patched)
    'payload/altscreen_handler.so'=[IO.File]::ReadAllBytes($handler)
    'ALTSCREEN_TEST_ONCE.txt'=$utf8.GetBytes("HomeKit-capable AltScreen map attempt. Native encoder stops for one session.`n")
    'READ-ME-FIRST.txt'=$utf8.GetBytes([IO.File]::ReadAllText((Resolve-Path -LiteralPath $Instructions)))
}
$fileHashes=[ordered]@{};foreach($name in $files.Keys){$fileHashes[$name]=Digest $files[$name]}
$manifest=[ordered]@{
    format='moi3-altscreen-map-v4';unit='3G5035842B';hardware='H56';software='0841'
    purpose='homekit-altscreen-map-test';clusterOutput=$true;nativeEncoderStop=$true
    factoryHashes=$expected;files=$fileHashes
    sourceHashes=@{handler=File-Digest $HandlerSource;homekitCrypto=File-Digest (Join-Path (Split-Path $HandlerSource) 'homekit-screen-crypto.h');fileGuard=File-Digest $guardSource;patcher=File-Digest (Join-Path $PSScriptRoot 'add-handler-dependency.py')}
    compilerVersion=(& $Zig version);containerSha256=File-Digest $containerPath
    hardwareValidated=$false;knownSpeechResets=$true;completeVehicleRollback=$false
}
$files['build-manifest.json']=$utf8.GetBytes(($manifest|ConvertTo-Json -Depth 8))
$sourceZip=[IO.Compression.ZipFile]::OpenRead($containerPath)
$outZip=[IO.Compression.ZipFile]::Open($outputPath,[IO.Compression.ZipArchiveMode]::Create)
try {
    foreach($sourceEntry in $sourceZip.Entries){
        if($sourceEntry.FullName -eq 'fix_hw_partnumber.sh'){continue}
        if($sourceEntry.FullName -notmatch '^(Data|Meta)/' -or $sourceEntry.FullName -match '(^|/)\.\.(/|$)'){throw 'Unexpected source entry'}
        $targetEntry=$outZip.CreateEntry($sourceEntry.FullName)
        $inputStream=$sourceEntry.Open();$outputStream=$targetEntry.Open()
        try{$inputStream.CopyTo($outputStream)}finally{$inputStream.Dispose();$outputStream.Dispose()}
    }
    foreach($name in $files.Keys){
        $targetEntry=$outZip.CreateEntry($name);$stream=$targetEntry.Open()
        try{$stream.Write($files[$name],0,$files[$name].Length)}finally{$stream.Dispose()}
    }
}finally{$sourceZip.Dispose();$outZip.Dispose()}
Get-FileHash -LiteralPath $outputPath -Algorithm SHA256
Write-Output "Experimental package built locally: $outputPath"
Write-Output 'Do not publish the ZIP: it contains a copy derived from factory firmware. Verify it before vehicle use.'
