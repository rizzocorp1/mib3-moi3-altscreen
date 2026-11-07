param(
    [Parameter(Mandatory=$true)][string]$Archive,
    [Parameter(Mandatory=$true)][string]$SevenZip,
    [Parameter(Mandatory=$true)][string]$ReportDirectory
)
$ErrorActionPreference = 'Stop'
$archiveFile = Get-Item -LiteralPath $Archive
if ($archiveFile.PSIsContainer -or $archiveFile.Extension -ne '.7z') {
    throw 'Select a completed .7z archive, not a browser partial download.'
}
$sevenZipFile = Get-Item -LiteralPath $SevenZip
$stream = [IO.File]::OpenRead($archiveFile.FullName)
try {
    $header = New-Object byte[] 32
    if ($stream.Read($header, 0, 32) -ne 32) { throw 'Truncated 7z header.' }
    if ([BitConverter]::ToString($header[0..5]) -ne '37-7A-BC-AF-27-1C') { throw 'Not a 7z archive.' }
    $expectedLength = [decimal]32 + [BitConverter]::ToUInt64($header,12) + [BitConverter]::ToUInt64($header,20)
    if ($archiveFile.Length -ne $expectedLength) {
        throw "Incomplete or unexpected archive length: $($archiveFile.Length), expected $expectedLength."
    }
} finally { $stream.Dispose() }
if (Test-Path -LiteralPath $ReportDirectory) { throw 'Use a new report directory; existing reports are preserved.' }
$null = New-Item -ItemType Directory -Path $ReportDirectory
$reportRoot = (Resolve-Path -LiteralPath $ReportDirectory).Path
$digest = Get-FileHash -LiteralPath $archiveFile.FullName -Algorithm SHA256
[ordered]@{
    file = $archiveFile.Name
    bytes = $archiveFile.Length
    sha256 = $digest.Hash.ToLowerInvariant()
    inspectedUtc = [DateTime]::UtcNow.ToString('o')
    compatibilityVerified = $false
    vendorSignatureVerified = $false
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $reportRoot 'archive.json') -Encoding utf8
& $sevenZipFile.FullName l -slt -- $archiveFile.FullName 2>&1 | Set-Content -LiteralPath (Join-Path $reportRoot 'archive-list.txt') -Encoding utf8
if ($LASTEXITCODE -ne 0) { throw '7-Zip could not list the archive. See archive-list.txt.' }
& $sevenZipFile.FullName t -- $archiveFile.FullName 2>&1 | Set-Content -LiteralPath (Join-Path $reportRoot 'archive-test.txt') -Encoding utf8
if ($LASTEXITCODE -ne 0) { throw '7-Zip integrity test failed. See archive-test.txt.' }
Write-Output "Archive integrity checked. Reports: $reportRoot"
Write-Output 'This does not verify compatibility or vendor authenticity. No firmware was executed or installed.'
