; Inno Setup script for TiDE Rack -- BACKLOG R2.
;
; Produces the asset docs/distribution.md names:
;
;     TIDE-Rack-Windows.exe   ->  installs TIDE-Rack.vst3 into
;                                 C:\Program Files\Common Files\VST3\
;
; DO NOT RUN ISCC ON THIS FILE BY HAND. It compiles only against a staged
; payload directory, which scripts/package-windows.ps1 builds:
;
;     pwsh scripts/package-windows.ps1 -BuildDir build -SelfTest
;
; Running it directly fails with "PayloadDir is not defined", deliberately --
; the alternative is a default that silently packages whatever happens to be
; lying around in a build tree.
;
; Modelled on SE16/SynthEditCL/installer/SynthEditCL.iss, the closest precedent
; in the family: a real payload installer rather than SynthEdit2's MSIX
; bootstrapper.
;
; THE PAYLOAD IS A VST3 BUNDLE. TIDE-Rack.vst3 is a DIRECTORY --
; Contents\x86_64-win\TIDE-Rack.vst3 alongside Contents\Resources\ -- because
; the plug-in reads its rack prefabs and pin descriptions out of that Resources
; folder and a bare DLL has nowhere to keep them. The packaging script's header
; carries the full reasoning and the measurement behind it.
;
; NAMING, and the three forms must not be mixed (docs/distribution.md):
;   display form   TiDE Rack    -- AppName, wizard title, program group
;   shipped form   TIDE-Rack    -- every filename and every path
;   target form    TIDE_Rack    -- CMake only, and it does not appear here
;
; VERSIONING: the asset name carries no version -- R6's download permalinks
; depend on it being constant. AppVersion is cosmetic (Add/Remove Programs, the
; wizard caption) and defaults to 0.1.0; the release workflow (R5) passes the
; tag through /DAppVersion=.

#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif

#ifndef PayloadDir
  #error PayloadDir is not defined -- build this through scripts/package-windows.ps1
#endif

#ifndef OutputDirectory
  #define OutputDirectory "out"
#endif

#ifndef OutputName
  #define OutputName "TIDE-Rack-Windows"
#endif

; ---------------------------------------------------------------------------
; The next two exist so this script can be PROVEN, not merely compiled.
;
; The shipped installer writes to a fixed machine path under Program Files, so
; running one takes elevation -- which an unattended verification does not
; have, and macOS's `installer -target <sandbox>` has no Windows counterpart.
; scripts/package-windows.ps1 -SelfTest therefore compiles THIS SAME FILE a
; second time with these two overridden, runs that copy silently into a scratch
; folder, and compares every installed file against the payload by SHA-256.
;
; Two overrides, and nothing else differs between the two compilations: same
; [Files], same [UninstallDelete], same [Code]. A release build passes neither
; and gets the defaults below.
; ---------------------------------------------------------------------------
#ifndef Vst3Dir
  #define Vst3Dir "{commoncf64}\VST3"
#endif

#ifndef PrivilegesLevel
  #define PrivilegesLevel "admin"
#endif

#ifndef AppDirOverride
  #define AppDirOverride "{autopf}\TIDE Rack"
#endif

[Setup]
AppName=TiDE Rack
AppVersion={#AppVersion}
AppVerName=TiDE Rack {#AppVersion}
AppPublisher=SynthEdit Limited
AppPublisherURL=https://tidesynth.com/
AppSupportURL=https://tidesynth.com/
; R1(a), 2026-08-13: TIDE ships under the existing SynthEdit Limited signing
; identity rather than a second Azure certificate profile, so that is the
; publisher name the UAC prompt will carry. Naming it here too means the wizard
; agrees with the prompt instead of looking like a different product.

; The plug-in goes to the VST3 spec's fixed location, not here. This directory
; holds the uninstaller and the readme only, which is why the directory page is
; off: offering to "install" a VST3 somewhere else produces a plug-in no host
; scans.
DefaultDirName={#AppDirOverride}
DefaultGroupName=TiDE Rack
DisableDirPage=yes
DisableProgramGroupPage=yes

OutputDir={#OutputDirectory}
OutputBaseFilename={#OutputName}
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
; Common Files\VST3 lives under Program Files, so a shipped install cannot be
; per-user without putting the plug-in somewhere no host looks.
PrivilegesRequired={#PrivilegesLevel}
UninstallDisplayName=TiDE Rack {#AppVersion}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; The staged payload, copied into the VST3 folder as-is.
;
; Spelled as a recursive copy of the payload DIRECTORY rather than naming the
; bundle explicitly, so the [Files] entry does not encode an assumption about
; the layout: the packaging script decides what the bundle contains, asserts it,
; and prints it. Copying the inner DLL alone would ship a plug-in with an empty
; module browser and pinless controls, which is BACKLOG S21's failure on
; another platform.
Source: "{#PayloadDir}\*"; DestDir: "{#Vst3Dir}"; \
    Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#PayloadDir}\..\README.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Uninstall TiDE Rack"; Filename: "{uninstallexe}"

[UninstallDelete]
; [Files] entries are removed one by one, which leaves the now-empty bundle
; directories behind. Remove the bundle whole.
Type: filesandordirs; Name: "{#Vst3Dir}\TIDE-Rack.vst3"

[Code]
{
  A TIDE-Rack.vst3 left by a hand-copied build, or by the .zip, is not tracked
  by this installer: it would be overwritten silently and then only partly
  removed on uninstall. Say so rather than doing it quietly -- a bundle whose
  Resources do not match its DLL is exactly the failure that reads as "the
  plug-in is broken" to a user. Suppressible, so /SUPPRESSMSGBOXES silent runs
  (and the self-test) are unaffected.
}
function InitializeSetup(): Boolean;
var
  Existing: string;
begin
  Existing := ExpandConstant('{#Vst3Dir}\TIDE-Rack.vst3');
  if DirExists(Existing) or FileExists(Existing) then
    SuppressibleMsgBox(
      'An existing TIDE-Rack.vst3 was found in the VST3 folder:'#13#10#13#10 +
      Existing + #13#10#13#10 +
      'Setup will replace it.',
      mbInformation, MB_OK, IDOK);
  Result := True;
end;
