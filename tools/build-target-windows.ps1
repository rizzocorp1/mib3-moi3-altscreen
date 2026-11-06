param(
    [Parameter(Mandatory=$true)][string]$Zig,
    [Parameter(Mandatory=$true)][string]$Source,
    [Parameter(Mandatory=$true)][string]$Sysroot,
    [Parameter(Mandatory=$true)][string]$Inspector,
    [Parameter(Mandatory=$true)][string]$Output
)
$ErrorActionPreference = 'Stop'
$zigPath = (Resolve-Path -LiteralPath $Zig).Path
$headers = Join-Path (Split-Path -Parent $zigPath) 'lib/libc/include'
$sourcePath = (Resolve-Path -LiteralPath $Source).Path
$sysrootPath = (Resolve-Path -LiteralPath $Sysroot).Path
$libraries = @('libdl-2.23.so','libpthread-2.23.so','libc-2.23.so','ld-2.23.so') | ForEach-Object {
    (Resolve-Path -LiteralPath (Join-Path $sysrootPath "lib64/$_")).Path
}
foreach ($library in $libraries) {
    $info = & node $Inspector $library | ConvertFrom-Json
    if ($LASTEXITCODE -ne 0 -or $info.architecture -ne 'AArch64') { throw "Not an AArch64 factory library: $library" }
}
$outputPath = [IO.Path]::GetFullPath($Output)
$buildDirectory = Split-Path -Parent $outputPath
if (-not (Test-Path -LiteralPath $buildDirectory)) { throw 'Output directory must already exist.' }
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $buildDirectory '.zig-global-cache'
$env:ZIG_LOCAL_CACHE_DIR = Join-Path $buildDirectory '.zig-local-cache'
$arguments = @('cc','-target','aarch64-linux-gnu.2.23','-shared','-fPIC','-O2','-fno-stack-protector','-fno-sanitize=all','-nostdlib')
foreach ($include in @('aarch64-linux-gnu','generic-glibc','aarch64-linux-any','any-linux-any')) {
    $arguments += @('-isystem',(Join-Path $headers $include))
}
$arguments += @('-Wl,-z,defs','-Wl,--hash-style=both',$sourcePath) + $libraries + @('-o',$outputPath)
& $zigPath @arguments
if ($LASTEXITCODE -ne 0) { throw 'Target compiler failed.' }
$candidate = & node $Inspector $outputPath | ConvertFrom-Json
if ($LASTEXITCODE -ne 0 -or $candidate.architecture -ne 'AArch64') { throw 'Invalid target output.' }
foreach ($version in $candidate.versions) {
    if ($version.version -like 'GLIBC_*' -and ($version.version -eq 'GLIBC_PRIVATE' -or [version]$version.version.Substring(6) -gt [version]'2.23')) {
        throw "Unexpected runtime requirement: $($version.version)"
    }
}
$inputs = @($sourcePath,$zigPath) + $libraries | ForEach-Object {
    $hash = Get-FileHash -LiteralPath $_ -Algorithm SHA256
    @{file=$hash.Path; sha256=$hash.Hash.ToLowerInvariant()}
}
@{compilerVersion=(& $zigPath version); inputs=@($inputs); output=$candidate; vehicleValidated=$false} |
    ConvertTo-Json -Depth 12 | Set-Content -LiteralPath "$outputPath.build.json" -Encoding utf8
Write-Output "Built AArch64 candidate: $outputPath"
Write-Output 'No vehicle deployment or execution occurred. Installer, native channel ownership and on-car behavior remain unverified.'
