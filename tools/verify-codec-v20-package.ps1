param([Parameter(Mandatory=$true)][string]$Candidate)
$ErrorActionPreference='Stop';Add-Type -AssemblyName System.IO.Compression.FileSystem
$workspace=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
function Hash([byte[]]$b){[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($b)).ToLowerInvariant()}
function Bytes($zip,[string]$name){$e=$zip.GetEntry($name);if(!$e){throw "Missing $name"};$s=$e.Open();$m=[IO.MemoryStream]::new();try{$s.CopyTo($m);return ,$m.ToArray()}finally{$s.Dispose();$m.Dispose()}}
$original=[IO.Compression.ZipFile]::OpenRead((Join-Path $workspace 'firmware-audit/LG-wipe-FAZIT.zip'))
$zip=[IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $Candidate))
try{
 $required=@('payload/synthetic-player','payload/file-guard','payload/baseline-ref1-p.synthetic','payload/high-ref1-p.synthetic','payload/high-ref4-p.synthetic','payload/high-intra.synthetic','payload/controller.sh','payload/oneshot-boot.sh','fix_hw_partnumber.sh','CODEC_TEST_V20.txt','READ-ME-FIRST.txt')
 $names=@($zip.Entries.FullName);if(($names|Select-Object -Unique).Count -ne $names.Count){throw 'Duplicate entries'}
 foreach($e in $original.Entries){if($e.FullName -eq 'fix_hw_partnumber.sh'){continue};if((Hash (Bytes $original $e.FullName)) -cne (Hash (Bytes $zip $e.FullName))){throw 'Vendor carrier altered'}}
 $manifest=[Text.Encoding]::UTF8.GetString((Bytes $zip 'build-manifest.json'))|ConvertFrom-Json
 if($manifest.format -ne 'moi3-codec-test-v20' -or $manifest.software -ne '0841' -or $manifest.factoryExecutableChanged -ne $false -or $manifest.maximumPlaybackSeconds -ne 32 -or $manifest.monitorRuntimeMaxSeconds -ne 180 -or $manifest.newRawCapture -ne $false){throw 'Wrong test scope'}
 if((@($manifest.files.psobject.Properties.Name|Sort-Object)-join "`n") -cne (($required|Sort-Object)-join "`n")){throw 'Unexpected payload list'}
 $allowed=@($original.Entries.FullName)+$required+@('build-manifest.json')
 foreach($name in $names){if($name -notin $allowed -or $name -match '(^|/)\.\.(/|$)' -or $name.StartsWith('/')){throw 'Unexpected path'}}
 foreach($p in $manifest.files.psobject.Properties){if((Hash (Bytes $zip $p.Name)) -cne $p.Value){throw "Payload hash mismatch: $($p.Name)"}}
 $sources=@{player='implementation/synthetic-player-v20.c';transport='implementation/synthetic-transport-v20.h';guard='recovery/file-guard.c';resource='recovery/resource-audit-v17.h';export='recovery/video-export-v16.h';controller='recovery/codec-v20-controller.sh.in';boot='recovery/codec-v20-boot.sh.in';entry='recovery/codec-v20-usb.sh.in'}
 foreach($key in $sources.Keys){$actual=(Get-FileHash -LiteralPath (Join-Path $workspace $sources[$key])).Hash.ToLowerInvariant();if($actual -cne $manifest.sourceHashes.$key){throw "Source mismatch: $key"}}
 foreach($item in @(@('codec-v20-controller.sh.in','payload/controller.sh'),@('codec-v20-boot.sh.in','payload/oneshot-boot.sh'),@('codec-v20-usb.sh.in','fix_hw_partnumber.sh'))){
  $rendered=[IO.File]::ReadAllText((Join-Path $workspace "recovery/$($item[0])")).Replace("`r`n","`n")
  foreach($p in $manifest.factoryHashes.psobject.Properties){$rendered=$rendered.Replace("@$($p.Name)@",[string]$p.Value)}
  $rendered=$rendered.Replace('@ROOT@','');if($rendered -match '@[A-Z0-9_]+@' -or $rendered -cne [Text.Encoding]::UTF8.GetString((Bytes $zip $item[1]))){throw 'Rendered template differs'}
 }
 $meta=[Text.Encoding]::UTF8.GetString((Bytes $zip 'Meta/main.mnf'))|ConvertFrom-Json
 if($meta.Release -ne 'MOI3_EU_SE_E9400L' -or $meta.Includes.Count -ne 1 -or $meta.Includes[0].PackageName -ne 'MIB3OI_POST_SCRIPT'){throw 'Wrong update components'}
 foreach($name in @('baseline-ref1-p.synthetic','high-ref1-p.synthetic','high-ref4-p.synthetic','high-intra.synthetic')){
  $expected=[IO.File]::ReadAllBytes((Join-Path $workspace "firmware-audit/codec-refresh-audit/synthetic-controls-validated/$name"));if((Hash $expected) -cne (Hash (Bytes $zip "payload/$name"))){throw 'Synthetic clip differs from validated source'}
 }
 [ordered]@{vendorDataAndMetaUnchanged=$true;payloadHashesVerified=$required.Count;sourceAndTemplatesMatch=$true;onCarValidated=$false;sha256=(Get-FileHash -LiteralPath $Candidate).Hash.ToLowerInvariant()}|ConvertTo-Json
}finally{$original.Dispose();$zip.Dispose()}
