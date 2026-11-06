param(
  [string]$Container = "$PSScriptRoot/LG-wipe-FAZIT.zip",
  [string]$Candidate = "$PSScriptRoot/moi3-usb-trigger-probe-experimental.zip"
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$sha = [Security.Cryptography.SHA256]::Create()
function Read-ZipBytes($zip, $name) {
  $entry = $zip.GetEntry($name)
  if ($null -eq $entry) { throw "Missing entry: $name" }
  $inputStream = $entry.Open(); $buffer = [IO.MemoryStream]::new()
  try { $inputStream.CopyTo($buffer); return ,$buffer.ToArray() } finally { $inputStream.Dispose(); $buffer.Dispose() }
}
function Hash-Bytes([byte[]]$bytes) { [Convert]::ToHexString($sha.ComputeHash($bytes)).ToLowerInvariant() }
function Read-Json($zip, $name) { [Text.Encoding]::UTF8.GetString((Read-ZipBytes $zip $name)) | ConvertFrom-Json }
function Hash-Json($zip, $name) {
  $json = (Read-Json $zip $name) | ConvertTo-Json -Depth 40 -Compress
  Hash-Bytes ([Text.Encoding]::UTF8.GetBytes($json))
}
$sourceZip = [IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $Container))
$candidateZip = [IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $Candidate))
try {
  $manifest = Read-Json $sourceZip 'Meta/main.mnf'
  if ($manifest.Release -ne 'MOI3_EU_SE_E9400L' -or $manifest.Includes.Count -ne 1 -or $manifest.Includes[0].PackageName -ne 'MIB3OI_POST_SCRIPT') { throw 'Unexpected component list' }
  $paths = @('Meta/main.mnf','Meta/MIB3OI_POST_SCRIPT/1.940.2.mnf','Meta/MIB3OI_POST_SCRIPT/POST_SCRIPT/script_installer/1.940.2.mnf')
  $checks = (Read-Json $sourceZip 'Meta/main.mnf.cks').PackageChecksumList
  for ($i = 0; $i -lt $paths.Count; $i++) {
    if ((Hash-Json $sourceZip $paths[$i]) -ne $checks[$i].CheckSum[0]) { throw "Manifest checksum failed: $($paths[$i])" }
  }
  $module = Read-Json $sourceZip $paths[2]
  $installerPath = 'Data/' + $module.HWIndex[0].InstallerFile
  if ((Hash-Json $sourceZip $installerPath) -ne $module.HWIndex[0].CheckSum[0]) { throw 'Installer checksum failed' }
  $installer = Read-Json $sourceZip $installerPath
  $payloadPath = $installerPath.Substring(0,$installerPath.LastIndexOf('/')+1) + $installer.Scripts[0].Path
  $payload = Read-ZipBytes $sourceZip $payloadPath
  if ($payload.Length -ne [int]$installer.Scripts[0].Length -or (Hash-Bytes $payload) -ne $installer.Scripts[0].CheckSum[0]) { throw 'Post-script checksum failed' }
  foreach ($entry in $sourceZip.Entries) {
    if ($entry.FullName -eq 'fix_hw_partnumber.sh') { continue }
    if ((Hash-Bytes (Read-ZipBytes $sourceZip $entry.FullName)) -ne (Hash-Bytes (Read-ZipBytes $candidateZip $entry.FullName))) { throw "Signed payload changed: $($entry.FullName)" }
  }
  $allowed = @($sourceZip.Entries.FullName) + @('PROBE_ONLY.txt','READ-ME-FIRST.txt')
  foreach ($entry in $candidateZip.Entries) { if ($entry.FullName -notin $allowed) { throw "Unexpected candidate file: $($entry.FullName)" } }
  $expected = [IO.File]::ReadAllText((Join-Path $PSScriptRoot '../recovery/usb-entry.sh')).Replace("`r`n","`n")
  if ([Text.Encoding]::UTF8.GetString((Read-ZipBytes $candidateZip 'fix_hw_partnumber.sh')) -cne $expected) { throw 'Probe entry differs from reviewed source' }
  if ([Text.Encoding]::UTF8.GetString((Read-ZipBytes $candidateZip 'fix_hw_partnumber.sh')) -match '/lge/app_ro/bin/ksend') { throw 'Original ksend command retained' }
  [pscustomobject]@{ChecksumRelationshipsVerified=5;DataAndMetaUnchanged=$true;ProbeMatchesReviewedSource=$true;AltScreenPayloadIncluded=$false;VendorSignatureAuthenticated=$false;AtlasHardwareTested=$false;CandidateSHA256=(Get-FileHash -LiteralPath $Candidate).Hash} | ConvertTo-Json
} finally { $sourceZip.Dispose(); $candidateZip.Dispose(); $sha.Dispose() }
