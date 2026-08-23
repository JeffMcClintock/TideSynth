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
    keep its own data, and TIDE has data it cannot work without: the four pin
    XMLs and the rack prefabs that SynthEditSem/CMakeLists.txt stages into a
    `Resources` folder. Shipping those loose into the shared Common Files\VST3
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

    SEPARATELY, AND NOT FIXED HERE: the build tree's own copy of those resources
    lands where nothing reads it. On Windows SynthEditSem/CMakeLists.txt stages
    them to $<TARGET_FILE_DIR>/../Resources -- `build/SynthEditSem/Resources`,
    one directory above the `Release\` folder holding the binary -- while a
    non-bundled Windows plug-in resolves its resources to the folder the binary
    is in. Measured on this box 2026-08-22: run the freshly built standalone and
    it prints "no Prefabs folder in bundle resources"; copy that same Resources
    folder's CONTENTS beside the binary and the identical build prints
    "6 rack prefab(s) seeded from the bundle". Filed as its own BACKLOG row --
    this script reads the staged folder from where CMake actually puts it, so
    the shipped asset is correct either way.

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

$PRODUCT = 'TIDE Rack'              # display form -- docs/distribution.md
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

# Where SynthEditSem stages the pin XMLs and Prefabs\ on Windows. Read from
# CMake's actual destination rather than from where the runtime would look for
# them; see the header.
#
# On Windows this is written ONCE, by the TIDE_Rack_stage_resources custom
# target, not by each format target's POST_BUILD -- issue #314. The destination
# is unchanged, so nothing here had to move; the name is recorded because a
# future edit to that target is an edit to this script's input.
$resSrc = Join-Path $BuildDir 'SynthEditSem\Resources'
if (-not (Test-Path -LiteralPath $resSrc -PathType Container)) {
    throw @"
no staged resources at $resSrc.

Refusing to package: without them TIDE ships with an empty rack module browser
and classic controls that have no pins, and nothing in the plug-in fails loudly
enough for a user to know why. That is BACKLOG S21's failure wearing a
different platform. Build the TIDE_Rack_VST3 target (its POST_BUILD steps stage
this folder) and try again.
"@
}

$prefabs = Join-Path $resSrc 'Prefabs'
if (-not (Test-Path -LiteralPath $prefabs -PathType Container)) {
    throw "staged resources at $resSrc have no Prefabs\ folder -- see above, the browser would be empty"
}
$prefabCount = @(Get-ChildItem -LiteralPath $prefabs -Recurse -File |
                 Where-Object { $_.Extension -in '.synthedit', '.syntheditprefab' }).Count

Write-Host "==> $PRODUCT $Version"
Write-Host "    build      : $BuildDir"
Write-Host "    plug-in    : $binSrc"
Write-Host "    resources  : $resSrc  ($prefabCount prefab(s))"
if ($prefabCount -eq 0) {
    throw "the staged Prefabs\ folder holds no .synthedit files -- the browser would be empty"
}

# --- stage the bundle ------------------------------------------------------
$stage = Join-Path $OutDir 'payload'
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
$contents = Join-Path $stage "$BUNDLE\Contents"
New-Item -ItemType Directory -Force -Path (Join-Path $contents $ARCH_DIR) | Out-Null
Copy-Item -LiteralPath $binSrc -Destination (Join-Path $contents "$ARCH_DIR\$BUNDLE") -Force
Copy-Item -LiteralPath $resSrc -Destination (Join-Path $contents 'Resources') -Recurse -Force

Write-Host "==> staged bundle"
Write-Host "    $BUNDLE\Contents\$ARCH_DIR\$BUNDLE"
Write-Host "    $BUNDLE\Contents\Resources\  ($((Get-ChildItem -LiteralPath (Join-Path $contents 'Resources') -Recurse -File).Count) file(s))"

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
TIDE Rack $Version
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
