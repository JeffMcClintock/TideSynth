<#
.SYNOPSIS
    BACKLOG R2 -- build TIDE-Rack-Windows.exe and TIDE-Rack-Windows.zip.

.DESCRIPTION
    Produces the two assets docs/distribution.md names for Windows, installing
    to the location it specifies:

        TIDE-Rack.vst3  ->  C:\Program Files\Common Files\VST3\

    THE PAYLOAD IS A VST3 BUNDLE, NOT THE BARE .vst3 THE BUILD EMITS, and that
    is the substantive decision in this script rather than a packaging detail.

    gmpi_plugin.cmake gives the Windows VST3 target a plain `SUFFIX ".vst3"`
    (the `else()` arm at the end of its `FIND_VST3_INDEX` block) -- macOS gets a
    real bundle and Linux gets one assembled by a POST_BUILD copy, Windows gets
    neither. A bare DLL is a legal VST3 and hosts load it, but it has nowhere to
    keep its own data, and TIDE has data it cannot work without: the pin XMLs,
    the default rack and the rack prefabs that SynthEditSem/CMakeLists.txt
    stages into a `Resources` folder. Shipping those loose into the shared Common Files\VST3
    folder is not an option -- they would sit beside every other vendor's
    plug-ins and collide by name.

    The bundle solves it exactly, and the runtime already knows how to read one:
    BundleInfo::pluginIsBundle (BundleInfo.cpp:695) is set by finding
    ".vst3\Contents" in the loaded module's path, and getResourceFolder() then
    returns <bundle>\Contents\Resources\ -- the same layout macOS and Linux
    already use. So the shipped layout is:

        TIDE-Rack.vst3\
            Contents\
                x86_64-win\TIDE-Rack.vst3     <- the DLL the build produced
                Resources\...                 <- the pin XMLs and Prefabs\

    BACKLOG S36, FIXED 2026-08-23: the build tree's own copy of those resources
    used to land where nothing read it -- one directory above the `Release\`
    folder holding the binary, while a non-bundled Windows plug-in resolves its
    resources to the folder the binary is in (no `Resources` subfolder at all;
    `BundleInfo::getResourceFolder()` returns the bare directory verbatim for a
    non-bundle). So in the DEV TREE the pin XMLs,
    `DefaultRack.synthedit` and `Prefabs\` now sit LOOSE in `Release\`, beside
    the binaries -- there is no `Resources` folder to speak of until packaging
    makes one. This script picks those specific, known items out of `Release\`
    rather than copying the whole directory, which also holds every target's
    binaries, PDBs, `.lib`s and `.exp`s and would ship all of it into the
    bundle's `Resources\` otherwise.

    BACKLOG E63: THE XML LIST IS READ OUT OF `SynthEditSem/CMakeLists.txt`'s
    `_tide_xmls`, NOT RESTATED HERE, and a post-staging check asserts that every
    resource the build produced reached the bundle. The previous version kept a
    hand-written copy of that list with a comment saying the two must move
    together; E48 added two XMLs, the copy did not move, and a packaged Windows
    build shipped two modules with no pins and no default rack at all. That
    comment is now a parser and an assertion.

    SIGNING IS NOT DONE WITHOUT CREDENTIALS, and that is deliberate rather than
    unfinished, the same shape scripts/package-macos.sh uses. Azure Trusted
    Signing, under the identity R1(a) settled (SynthEdit Limited), reading the
    values SE16/SynthEdit_store_win.yml:199-211 passes to the ArtifactSigning@1
    task:

        AZURE_TENANT_ID / AZURE_CLIENT_ID / AZURE_CLIENT_SECRET
        TRUSTED_SIGNING_ENDPOINT   default https://eus.codesigning.azure.net/
        TRUSTED_SIGNING_ACCOUNT    default SynthEditTrustedSigning
        TRUSTED_SIGNING_PROFILE    default SynthEditCertificateProfile

    THE SIGNING PATH IS UNVERIFIED. That Azure Pipelines task does not exist for
    a local run or for GitHub Actions, so this drives signtool.exe with the
    Azure Code Signing dlib instead -- which is the documented equivalent, and
    which nobody has run here: the dlib is not installed on this box and no
    credentials are present. R5 owns wiring the secrets, and it should treat
    this block as a starting point to test rather than as working code.

.PARAMETER BuildDir
    A configured-and-built CMake tree, e.g. `build`. Release config.

.PARAMETER OutDir
    Where the two assets land. Defaults to <BuildDir>\package.

.PARAMETER Version
    Cosmetic only -- Add/Remove Programs and the wizard caption. The ASSET NAMES
    carry no version, because R6's download permalinks depend on them being
    constant. Defaults to $env:TIDE_RACK_VERSION, then 0.1.0.

.PARAMETER SelfTest
    After building the assets, prove the installer installs what it claims by
    compiling a SECOND copy of the same .iss redirected at a scratch directory
    and running it silently. See the SelfTest block for the two -- and only two
    -- ways that copy differs from the shipped one.

.EXAMPLE
    pwsh scripts/package-windows.ps1 -BuildDir build -SelfTest
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$BuildDir,
    [string]$OutDir,
    [string]$Version,
    [switch]$SelfTest
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$PRODUCT = 'TiDE Rack'              # display form -- docs/distribution.md
$ASSET_EXE = 'TIDE-Rack-Windows.exe'
$ASSET_ZIP = 'TIDE-Rack-Windows.zip'
$BUNDLE = 'TIDE-Rack.vst3'
$ARCH_DIR = 'x86_64-win'            # the VST3 spec's Windows architecture folder

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $Version) { $Version = $env:TIDE_RACK_VERSION }
if (-not $Version) { $Version = '0.1.0' }

$BuildDir = (Resolve-Path -LiteralPath $BuildDir).Path
if (-not $OutDir) { $OutDir = Join-Path $BuildDir 'package' }

# --- locate what the build produced ---------------------------------------
$binSrc = Join-Path $BuildDir "SynthEditSem\Release\$BUNDLE"
if (-not (Test-Path -LiteralPath $binSrc -PathType Leaf)) {
    throw "no $BUNDLE in $BuildDir\SynthEditSem\Release -- build the Release config first (cmake --build $BuildDir --config Release)"
}

# Where SynthEditSem stages the pin XMLs, DefaultRack.synthedit and Prefabs\ on
# Windows: loose in the Release\ folder, beside the binaries -- BACKLOG S36.
# This is written ONCE, by the TIDE_Rack_stage_resources custom target, not by
# each format target's POST_BUILD -- issue #314.
#
# BACKLOG E63. This list is now DERIVED from SynthEditSem/CMakeLists.txt's
# `_tide_xmls` rather than restated here, because restating it is what broke:
# E48 added EnvelopeAdsr.xml and Oscillator.xml to `_tide_xmls` on 2026-08-28
# and this script's hand-written copy did not move with them, so a packaged
# Windows build shipped `SynthEdit ADSR` and `SE Oscillator` with no pins --
# S21's failure, in the one script whose own comment demanded the two lists
# move together. A comment cannot enforce that; reading the list can.
#
# Windows is the only packaging script that ever maintained a second list --
# package-linux.sh copies the whole staged Resources directory and macOS copies
# assembled bundles -- which is why only this platform could drift.
#
# Still picked out by name rather than copying Release\ wholesale, which also
# holds every target's binaries, PDBs, .libs and .exps. The completeness
# assertion after staging is what makes that safe: it fails if the build staged
# a resource this script did not carry, in either direction.
$resSrc = Join-Path $BuildDir 'SynthEditSem\Release'

# Parse `set(_tide_xmls <dir>/<name>.xml ...)` and keep the basenames -- the
# staging commands there copy each entry to the flat destination, so the
# basename is what lands in Release\ and in Contents\Resources\.
$cmakeLists = Join-Path $repoRoot 'SynthEditSem\CMakeLists.txt'
if (-not (Test-Path -LiteralPath $cmakeLists -PathType Leaf)) {
    throw "cannot read $cmakeLists -- this script derives its resource list from that file's _tide_xmls (BACKLOG E63)"
}
$cmakeText = Get-Content -LiteralPath $cmakeLists -Raw
$m = [regex]::Match($cmakeText, '(?ms)^\s*set\s*\(\s*_tide_xmls\b(.*?)^\s*\)\s*$')
if (-not $m.Success) {
    throw @"
no 'set(_tide_xmls ...)' block found in $cmakeLists

Refusing to package rather than shipping a guess. This script derives the pin
XML list from that block (BACKLOG E63) so the two cannot drift; if the block
was renamed or reshaped, fix this parser rather than restoring a second copy
of the list here.
"@
}
$ResourceXmls = @(
    $m.Groups[1].Value -split "`n" |
        ForEach-Object { ($_ -replace '#.*$', '').Trim() } |
        Where-Object { $_ -match '\.xml$' } |
        ForEach-Object { Split-Path -Leaf $_ }
)
if ($ResourceXmls.Count -eq 0) {
    throw "parsed _tide_xmls out of $cmakeLists but it yielded no .xml entries -- refusing to package a bundle with no pin descriptions"
}

# NOT in `_tide_xmls`: CMake copies it by its own explicit command, and without
# it a first-run user gets `TIDE: no DefaultRack.synthedit in bundle resources
# - starting with an empty rack` (TideApp.cpp:1073) and an empty rack.
$DEFAULT_RACK = 'DefaultRack.synthedit'

$missingXmls = $ResourceXmls | Where-Object { -not (Test-Path -LiteralPath (Join-Path $resSrc $_) -PathType Leaf) }
if ($missingXmls) {
    throw @"
missing from $resSrc : $($missingXmls -join ', ')

Refusing to package: without them TIDE ships with classic controls that have no
pins, and nothing in the plug-in fails loudly enough for a user to know why.
That is BACKLOG S21's failure wearing a different platform. Build the
TIDE_Rack_VST3 target (its POST_BUILD steps stage these) and try again.
"@
}

# E63 -- the same refusal, for the file that decides whether a first-run user
# sees a rack at all. Separate from $missingXmls above because the consequence
# is different: a missing pin XML gives pinless controls, a missing default
# rack gives an empty window.
$defaultRackSrc = Join-Path $resSrc $DEFAULT_RACK
if (-not (Test-Path -LiteralPath $defaultRackSrc -PathType Leaf)) {
    throw @"
missing from $resSrc : $DEFAULT_RACK

Refusing to package: the plug-in would print 'TIDE: no $DEFAULT_RACK in bundle
resources - starting with an empty rack' and a first-run user would get
nothing. SynthEditSem/CMakeLists.txt copies it beside the pin XMLs; build the
TIDE_Rack_VST3 target and try again.
"@
}

$prefabs = Join-Path $resSrc 'Prefabs'
if (-not (Test-Path -LiteralPath $prefabs -PathType Container)) {
    throw "no Prefabs\ folder at $resSrc -- see above, the browser would be empty"
}
$prefabCount = @(Get-ChildItem -LiteralPath $prefabs -Recurse -File |
                 Where-Object { $_.Extension -in '.synthedit', '.syntheditprefab' }).Count

Write-Host "==> $PRODUCT $Version"
Write-Host "    build      : $BuildDir"
Write-Host "    plug-in    : $binSrc"
Write-Host "    resources  : $resSrc  ($prefabCount prefab(s))"
if ($prefabCount -eq 0) {
    throw "the Prefabs\ folder at $resSrc holds no .synthedit files -- the browser would be empty"
}

# --- stage the bundle ------------------------------------------------------
$stage = Join-Path $OutDir 'payload'
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
$contents = Join-Path $stage "$BUNDLE\Contents"
New-Item -ItemType Directory -Force -Path (Join-Path $contents $ARCH_DIR) | Out-Null
Copy-Item -LiteralPath $binSrc -Destination (Join-Path $contents "$ARCH_DIR\$BUNDLE") -Force

# Assemble Contents\Resources\ from the known items picked out of $resSrc
# above, not by copying that directory whole -- see the header and the
# $ResourceXmls comment for why.
$resourcesOut = Join-Path $contents 'Resources'
New-Item -ItemType Directory -Force -Path $resourcesOut | Out-Null
foreach ($xml in $ResourceXmls) {
    Copy-Item -LiteralPath (Join-Path $resSrc $xml) -Destination (Join-Path $resourcesOut $xml) -Force
}
Copy-Item -LiteralPath $defaultRackSrc -Destination (Join-Path $resourcesOut $DEFAULT_RACK) -Force
Copy-Item -LiteralPath $prefabs -Destination (Join-Path $resourcesOut 'Prefabs') -Recurse -Force

Write-Host "==> staged bundle"
Write-Host "    $BUNDLE\Contents\$ARCH_DIR\$BUNDLE"
Write-Host "    $BUNDLE\Contents\Resources\  ($((Get-ChildItem -LiteralPath (Join-Path $contents 'Resources') -Recurse -File).Count) file(s))"

# --- E63: everything the build staged is in the package --------------------
#
# THIS IS THE CHECK THAT WOULD HAVE CAUGHT E63, and it is deliberately not a
# restatement of the list above -- it asks the BUILD TREE what it produced and
# fails on anything the packaging step did not carry. A list compared against
# itself proves nothing; the whole defect was two lists agreeing with their own
# copies and not with each other.
#
# What counts as a resource in Release\: that folder holds this script's inputs
# mixed with every target's build output, so the rule is by extension --
# *.xml and *.synthedit are staged resources, and the binaries, .pdb/.lib/.exp
# and the .vst3/.clap/.gmpi/.exe are not. Prefabs\ is checked as a directory
# because it is copied whole.
$stagedResources = @(Get-ChildItem -LiteralPath $resSrc -File |
                     Where-Object { $_.Extension -in '.xml', '.synthedit' } |
                     ForEach-Object { $_.Name })
$packagedResources = @(Get-ChildItem -LiteralPath $resourcesOut -File |
                       ForEach-Object { $_.Name })
$notPackaged = @($stagedResources | Where-Object { $_ -notin $packagedResources })
if ($notPackaged) {
    throw @"
the build staged resources this package does not carry: $($notPackaged -join ', ')

Refusing to ship an incomplete bundle. This is BACKLOG E63's failure recurring:
$resSrc holds a resource that Contents\Resources\ does not. Either add it to
SynthEditSem/CMakeLists.txt's _tide_xmls (from which the XML list above is
read), or -- if it is genuinely not a shipped resource -- teach this check to
exclude it, in that order.
"@
}
$prefabsOut = Join-Path $resourcesOut 'Prefabs'
$stagedPrefabs = @(Get-ChildItem -LiteralPath $prefabs -Recurse -File).Count
$packagedPrefabs = @(Get-ChildItem -LiteralPath $prefabsOut -Recurse -File).Count
if ($packagedPrefabs -ne $stagedPrefabs) {
    throw "Prefabs\ staged $stagedPrefabs file(s) and the package carries $packagedPrefabs -- refusing to ship a partial browser"
}
Write-Host "    resource check : $($packagedResources.Count) file(s) + $packagedPrefabs prefab file(s), matching $resSrc"

# --- sign the payload, if we were given credentials ------------------------
function Invoke-TrustedSigning {
    param([string[]]$Files, [string]$What)

    if (-not ($env:AZURE_TENANT_ID -and $env:AZURE_CLIENT_ID -and $env:AZURE_CLIENT_SECRET)) {
        Write-Host "==> sign $What SKIPPED (AZURE_TENANT_ID / AZURE_CLIENT_ID / AZURE_CLIENT_SECRET unset)"
        return $false
    }

    # UNVERIFIED -- see the header. signtool + the Azure Code Signing dlib is
    # the documented stand-in for the ArtifactSigning@1 pipeline task, and
    # neither the dlib nor a credential has been present anywhere this has run.
    $dlib = $env:TRUSTED_SIGNING_DLIB
    if (-not $dlib -or -not (Test-Path -LiteralPath $dlib)) {
        throw "credentials are set but TRUSTED_SIGNING_DLIB does not point at Azure.CodeSigning.Dlib.dll -- refusing to claim a signature that did not happen"
    }
    $signtool = Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\bin\*\x64\signtool.exe' |
                Sort-Object FullName | Select-Object -Last 1
    if (-not $signtool) { throw "signtool.exe not found (Windows SDK)" }

    $endpoint = if ($env:TRUSTED_SIGNING_ENDPOINT) { $env:TRUSTED_SIGNING_ENDPOINT } else { 'https://eus.codesigning.azure.net/' }
    $account  = if ($env:TRUSTED_SIGNING_ACCOUNT)  { $env:TRUSTED_SIGNING_ACCOUNT }  else { 'SynthEditTrustedSigning' }
    $profile  = if ($env:TRUSTED_SIGNING_PROFILE)  { $env:TRUSTED_SIGNING_PROFILE }  else { 'SynthEditCertificateProfile' }

    $meta = Join-Path ([System.IO.Path]::GetTempPath()) "tide-signing-$([guid]::NewGuid()).json"
    @{ Endpoint = $endpoint; CodeSigningAccountName = $account; CertificateProfileName = $profile } |
        ConvertTo-Json | Set-Content -LiteralPath $meta -Encoding utf8
    try {
        foreach ($f in $Files) {
            & $signtool.FullName sign /v /fd SHA256 /tr http://timestamp.acs.microsoft.com /td SHA256 `
                /dlib $dlib /dmdf $meta $f
            if ($LASTEXITCODE -ne 0) { throw "signtool failed on $f (exit $LASTEXITCODE)" }
        }
    } finally {
        Remove-Item -LiteralPath $meta -Force -ErrorAction SilentlyContinue
    }
    Write-Host "==> signed $What"
    return $true
}

$signedPayload = Invoke-TrustedSigning -Files @((Join-Path $contents "$ARCH_DIR\$BUNDLE")) -What 'payload'

# --- the readme that ships in both assets ----------------------------------
$readme = Join-Path $OutDir 'README.txt'
@"
TiDE Rack $Version
==================

An open-source, free, Eurorack-style modular synthesizer plug-in.

  https://tidesynth.com/
  https://github.com/JeffMcClintock/TideSynth

INSTALLING FROM THIS ZIP
------------------------
Copy the whole $BUNDLE folder into your VST3 folder:

  C:\Program Files\Common Files\VST3\

Copy the FOLDER, not just the file inside it. $BUNDLE is a VST3 bundle --
the plug-in reads its rack prefabs and its control descriptions out of
$BUNDLE\Contents\Resources, so a copy of the inner .vst3 on its own
loads with an empty module browser.

You will need administrator rights for that folder. $ASSET_EXE does
the same thing with an installer, and adds an uninstall entry.

Then rescan plug-ins in your DAW.

UNINSTALLING
------------
Delete the $BUNDLE folder from the VST3 folder above.

LICENCE
-------
ISC. See the repository.
"@ | Set-Content -LiteralPath $readme -Encoding utf8

# --- the zip ---------------------------------------------------------------
$zipPath = Join-Path $OutDir $ASSET_ZIP
if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }
Compress-Archive -Path (Join-Path $stage '*'), $readme -DestinationPath $zipPath -Force
Write-Host "==> $zipPath"

# --- the installer ---------------------------------------------------------
function Find-Iscc {
    $candidates = @(
        "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
        "$env:ProgramFiles\Inno Setup 6\ISCC.exe"
    )
    foreach ($c in $candidates) { if (Test-Path -LiteralPath $c) { return $c } }
    return $null
}

$iscc = Find-Iscc
$exePath = Join-Path $OutDir $ASSET_EXE
if (-not $iscc) {
    Write-Warning @"
ISCC.exe (Inno Setup 6) not found, so $ASSET_EXE was NOT built. The zip above
is complete and correct. To get the installer:

    winget install --id JRSoftware.InnoSetup --scope user

(--scope user installs to %LOCALAPPDATA%\Programs and needs no administrator.)
"@
    $signedInstaller = $false
} else {
    $iss = Join-Path $repoRoot 'installer\windows\TIDE-Rack.iss'
    Write-Host "==> ISCC $iss"
    & $iscc "/DAppVersion=$Version" "/DPayloadDir=$stage" "/DOutputDirectory=$OutDir" $iss | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "ISCC failed (exit $LASTEXITCODE)" }
    if (-not (Test-Path -LiteralPath $exePath)) { throw "ISCC reported success but $exePath is not there" }
    Write-Host "==> $exePath"
    $signedInstaller = Invoke-TrustedSigning -Files @($exePath) -What 'installer'
}

# --- self-test -------------------------------------------------------------
# Proving an installer installs what it says is awkward on Windows: the
# destination is a fixed machine path under Program Files, so a real run needs
# elevation, which an unattended one does not have. macOS has `installer
# -target <sandbox volume>` and this does not.
#
# So: compile the SAME .iss a second time with the destination redirected at a
# scratch folder, run THAT silently, and compare what lands against the staged
# payload byte for byte. The relocated copy differs from the shipped one in
# exactly two `#define`s, both printed below, and in nothing else -- same
# script, same payload, same [Files] and [UninstallDelete] logic.
if ($SelfTest) {
    if (-not $iscc) { throw "-SelfTest needs ISCC.exe" }

    $sandbox = Join-Path $OutDir 'selftest'
    if (Test-Path -LiteralPath $sandbox) { Remove-Item -LiteralPath $sandbox -Recurse -Force }
    $vst3Dir = Join-Path $sandbox 'VST3'
    $appDir  = Join-Path $sandbox 'App'
    New-Item -ItemType Directory -Force -Path $sandbox | Out-Null

    Write-Host ""
    Write-Host "==> self-test: recompiling the same .iss with two overrides"
    Write-Host "      Vst3Dir          = $vst3Dir     (shipped: {commoncf64}\VST3)"
    Write-Host "      PrivilegesLevel  = lowest       (shipped: admin)"

    $iss = Join-Path $repoRoot 'installer\windows\TIDE-Rack.iss'
    & $iscc "/DAppVersion=$Version" "/DPayloadDir=$stage" "/DOutputDirectory=$sandbox" `
            "/DVst3Dir=$vst3Dir" "/DPrivilegesLevel=lowest" "/DAppDirOverride=$appDir" `
            "/DOutputName=TIDE-Rack-selftest" $iss | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "ISCC failed on the self-test copy (exit $LASTEXITCODE)" }

    $testExe = Join-Path $sandbox 'TIDE-Rack-selftest.exe'
    $log = Join-Path $sandbox 'install.log'
    & $testExe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART "/LOG=$log" | Out-Null
    # Inno's setup process detaches; wait for the uninstaller to appear.
    $deadline = (Get-Date).AddSeconds(90)
    while (-not (Test-Path -LiteralPath (Join-Path $appDir 'unins000.exe')) -and (Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 250
    }

    $installedBundle = Join-Path $vst3Dir $BUNDLE
    if (-not (Test-Path -LiteralPath $installedBundle -PathType Container)) {
        throw "self-test: the installer did not create $installedBundle (log: $log)"
    }

    # Byte-for-byte against the payload, not a file count.
    function Get-TreeHashes([string]$root) {
        $h = @{}
        Get-ChildItem -LiteralPath $root -Recurse -File | ForEach-Object {
            $rel = $_.FullName.Substring($root.Length).TrimStart('\')
            $h[$rel] = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
        }
        return $h
    }
    $want = Get-TreeHashes (Join-Path $stage $BUNDLE)
    $got  = Get-TreeHashes $installedBundle
    $missing = $want.Keys | Where-Object { -not $got.ContainsKey($_) }
    $differs = $want.Keys | Where-Object { $got.ContainsKey($_) -and $got[$_] -ne $want[$_] }
    $extra   = $got.Keys  | Where-Object { -not $want.ContainsKey($_) }
    if ($missing) { throw "self-test: installed bundle is missing $($missing.Count) file(s): $($missing -join ', ')" }
    if ($differs) { throw "self-test: installed bundle differs in $($differs.Count) file(s): $($differs -join ', ')" }
    if ($extra)   { throw "self-test: installed bundle has $($extra.Count) unexpected file(s): $($extra -join ', ')" }
    Write-Host "    installed $($want.Count) file(s), all SHA-256 identical to the payload"

    # And that uninstall takes the bundle with it -- [Files] alone leaves the
    # bundle directories behind, which is what [UninstallDelete] is for.
    $unins = Join-Path $appDir 'unins000.exe'
    if (-not (Test-Path -LiteralPath $unins)) { throw "self-test: no uninstaller at $unins" }
    & $unins /VERYSILENT /SUPPRESSMSGBOXES /NORESTART | Out-Null
    $deadline = (Get-Date).AddSeconds(90)
    while ((Test-Path -LiteralPath $installedBundle) -and (Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 250
    }
    if (Test-Path -LiteralPath $installedBundle) {
        throw "self-test: uninstall left $installedBundle behind"
    }
    Write-Host "    uninstall removed the bundle whole"
    Write-Host "==> self-test PASSED"
}

# --- report ----------------------------------------------------------------
Write-Host ""
Write-Host "==> $OutDir"
Write-Host "    product           : $PRODUCT $Version"
Write-Host "    $ASSET_ZIP  : $(if (Test-Path -LiteralPath $zipPath) { '{0:N1} MB' -f ((Get-Item $zipPath).Length / 1MB) } else { 'NOT BUILT' })"
Write-Host "    $ASSET_EXE  : $(if (Test-Path -LiteralPath $exePath) { '{0:N1} MB' -f ((Get-Item $exePath).Length / 1MB) } else { 'NOT BUILT' })"
Write-Host "    payload signed    : $(if ($signedPayload) { 'yes' } else { 'no' })"
Write-Host "    installer signed  : $(if ($signedInstaller) { 'yes' } else { 'no' })"
if (-not $signedInstaller) {
    Write-Host ""
    Write-Host "    THIS INSTALLER IS NOT SHIPPABLE AS-IS. An unsigned installer for"
    Write-Host "    an audio plug-in draws a SmartScreen 'unrecognised app' warning"
    Write-Host "    and a UAC prompt naming an unknown publisher. It is a correct"
    Write-Host "    installer for testing the layout, and nothing more."
}
