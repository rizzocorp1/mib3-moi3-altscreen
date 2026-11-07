param(
    [Parameter(Mandatory=$true)][string]$Container,
    [Parameter(Mandatory=$true)][string]$Candidate
)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
function Bytes($Zip,[string]$Name) {
    $entry=$Zip.GetEntry($Name);if($null -eq $entry){throw "Missing $Name"}
    $inputStream=$entry.Open();$buffer=[IO.MemoryStream]::new()
    try{$inputStream.CopyTo($buffer);return ,$buffer.ToArray()}finally{$inputStream.Dispose();$buffer.Dispose()}
}
function Digest([byte[]]$Value){[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($Value)).ToLowerInvariant()}
if ((Get-FileHash -LiteralPath $Container).Hash -ne '763A85698771EF2BB69FB29EB6A1E8B0A32B063AEE1910194865F70EA6C1005D'){throw 'Unknown source container'}
$source=[IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $Container))
$test=[IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $Candidate))
try {
    $names=@($test.Entries.FullName)
    if (($names|Select-Object -Unique).Count -ne $names.Count){throw 'Duplicate archive entries'}
    foreach($entry in $source.Entries){
        if($entry.FullName -eq 'fix_hw_partnumber.sh'){continue}
        if((Digest (Bytes $source $entry.FullName)) -ne (Digest (Bytes $test $entry.FullName))){throw "Changed vendor data: $($entry.FullName)"}
    }
    $main=[Text.Encoding]::UTF8.GetString((Bytes $test 'Meta/main.mnf'))|ConvertFrom-Json
    if($main.Release -ne 'MOI3_EU_SE_E9400L' -or $main.Includes.Count -ne 1 -or $main.Includes[0].PackageName -ne 'MIB3OI_POST_SCRIPT'){throw 'Unexpected update components'}
    $manifest=[Text.Encoding]::UTF8.GetString((Bytes $test 'build-manifest.json'))|ConvertFrom-Json
    if($manifest.purpose -ne 'homekit-altscreen-map-test' -or $manifest.clusterOutput -ne $true -or $manifest.nativeEncoderStop -ne $true){throw 'Package is not the reviewed HomeKit map test'}
    if($manifest.format -ne 'moi3-altscreen-map-v6' -or $manifest.software -ne '0841' -or $manifest.unit -ne '3G5035842B'){throw 'Wrong test identity'}
    $required=@('fix_hw_partnumber.sh','payload/oneshot-boot.sh','payload/managed-log-collector.sh','payload/file-guard','payload/carplay-once','payload/altscreen_handler.so','ALTSCREEN_TEST_ONCE.txt','READ-ME-FIRST.txt')
    $declared=@($manifest.files.psobject.Properties.Name)
    $expectedList=($required|Sort-Object) -join "`n"
    $actualList=($declared|Sort-Object) -join "`n"
    if($expectedList -cne $actualList){throw 'Unexpected manifest file list'}
    foreach($property in $manifest.files.psobject.Properties){
        if((Digest (Bytes $test $property.Name)) -cne $property.Value){throw "Payload checksum mismatch: $($property.Name)"}
    }
    $allowed=@($source.Entries.FullName)+$required+@('build-manifest.json')
    foreach($name in $names){if($name -notin $allowed -or $name -match '(^|/)\.\.(/|$)' -or $name.StartsWith('/')){throw "Unexpected candidate path: $name"}}
    $entryText=[Text.Encoding]::UTF8.GetString((Bytes $test 'fix_hw_partnumber.sh'))
    $bootText=[Text.Encoding]::UTF8.GetString((Bytes $test 'payload/oneshot-boot.sh'))
    if($entryText -match '@[A-Z_]+@|/lge/app_ro/bin/ksend' -or $bootText -match '@[A-Z_]+@'){throw 'Unrendered/unreviewed activation content'}
    $collectorText=[IO.File]::ReadAllText((Join-Path $PSScriptRoot '../recovery/map-v6-log-collector.sh')).Replace("`r`n","`n")
    if ($collectorText -cne [Text.Encoding]::UTF8.GetString((Bytes $test 'payload/managed-log-collector.sh'))){throw 'Collector differs from reviewed source'}
    $values=$manifest.factoryHashes
    $recovery=Join-Path $PSScriptRoot '../recovery'
    foreach($item in @(@{template='map-v6-usb.sh.in';name='fix_hw_partnumber.sh'},@{template='map-v6-boot.sh.in';name='payload/oneshot-boot.sh'})){
        $rendered=[IO.File]::ReadAllText((Join-Path $recovery $item.template)).Replace("`r`n","`n")
        foreach($property in $values.psobject.Properties){$rendered=$rendered.Replace("@$($property.Name)@",[string]$property.Value)}
        $rendered=$rendered.Replace('@ROOT@','').Replace('@LOG_ITERATIONS@','150')
        if($rendered -cne [Text.Encoding]::UTF8.GetString((Bytes $test $item.name))){throw "Candidate differs from current source template: $($item.name)"}
    }
    [ordered]@{vendorDataAndMetaUnchanged=$true;updateComponents=@('MIB3OI_POST_SCRIPT');payloadHashesVerified=$required.Count;templatesMatch=$true;hardwareActivationValidated=$false;candidateSha256=(Get-FileHash -LiteralPath $Candidate).Hash.ToLowerInvariant()}|ConvertTo-Json -Depth 4
}finally{$source.Dispose();$test.Dispose()}
