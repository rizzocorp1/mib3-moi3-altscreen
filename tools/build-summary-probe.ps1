param(
    [string]$Output = "$PSScriptRoot/moi3-summary-hook-probe.zip",
    [string]$Instructions = "$PSScriptRoot/../docs/summary-probe.md"
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
if (Test-Path -LiteralPath $Output) { throw 'Output exists; use a new filename.' }
$source = Join-Path $PSScriptRoot '../recovery/summary-probe.sh'
$script = [IO.File]::ReadAllText((Resolve-Path -LiteralPath $source)).Replace("`r`n","`n")
$readme = [IO.File]::ReadAllText((Resolve-Path -LiteralPath $Instructions))
$archivePath = [IO.Path]::GetFullPath($Output)
$archive = [IO.Compression.ZipFile]::Open($archivePath,[IO.Compression.ZipArchiveMode]::Create)
try {
    $entries = [ordered]@{
        'fix_hw_partnumber.sh' = $script
        'SUMMARY_PROBE_ONLY.txt' = "Report only. No firmware update or AltScreen installation.`n"
        'README.txt' = $readme
    }
    foreach ($name in $entries.Keys) {
        $entry = $archive.CreateEntry($name)
        $writer = [IO.StreamWriter]::new($entry.Open(),[Text.UTF8Encoding]::new($false))
        try { $writer.Write($entries[$name]) } finally { $writer.Dispose() }
    }
} finally { $archive.Dispose() }
Get-FileHash -LiteralPath $archivePath -Algorithm SHA256
