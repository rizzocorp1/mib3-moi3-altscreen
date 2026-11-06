param(
  [string]$Container = "$PSScriptRoot/LG-wipe-FAZIT.zip",
  [string]$Output = "$PSScriptRoot/moi3-usb-trigger-probe-experimental.zip"
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$expected = '763A85698771EF2BB69FB29EB6A1E8B0A32B063AEE1910194865F70EA6C1005D'
if ((Get-FileHash -LiteralPath $Container).Hash -ne $expected) { throw 'Unexpected source archive hash' }
$probePath = Join-Path $PSScriptRoot '../recovery/usb-entry.sh'
$probe = [IO.File]::ReadAllText($probePath).Replace("`r`n", "`n")
$utf8 = [Text.UTF8Encoding]::new($false)
if (Test-Path -LiteralPath $Output) { throw 'Output already exists; choose a new output path' }
$sourceZip = [IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $Container))
$outputZip = [IO.Compression.ZipFile]::Open($Output, [IO.Compression.ZipArchiveMode]::Create)
try {
  foreach ($entry in $sourceZip.Entries) {
    if ($entry.FullName -eq 'fix_hw_partnumber.sh') { continue }
    if ($entry.FullName -notmatch '^(Data|Meta)/' -or $entry.FullName -match '(^|/)\.\.(/|$)') { throw 'Unexpected source archive entry' }
    $copy = $outputZip.CreateEntry($entry.FullName)
    $inputStream = $entry.Open(); $outputStream = $copy.Open()
    try { $inputStream.CopyTo($outputStream) } finally { $inputStream.Dispose(); $outputStream.Dispose() }
  }
  $extras = @{
    'fix_hw_partnumber.sh' = $probe
    'PROBE_ONLY.txt' = "USB execution probe only; no AltScreen installation.`n"
    'READ-ME-FIRST.txt' = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'USB-TEST-INSTRUCTIONS.md'))
  }
  foreach ($name in $extras.Keys) {
    $entry = $outputZip.CreateEntry($name)
    $stream = $entry.Open()
    try { $bytes = $utf8.GetBytes($extras[$name]); $stream.Write($bytes, 0, $bytes.Length) } finally { $stream.Dispose() }
  }
} finally { $sourceZip.Dispose(); $outputZip.Dispose() }
Get-FileHash -LiteralPath $Output
