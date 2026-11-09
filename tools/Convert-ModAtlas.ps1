<#
.SYNOPSIS
  Patches Mod Atlas (hand-rolled SKSE C-ABI plugin, no game addresses) and builds
  one DLL per Skyrim runtime plus a FOMOD install zip that picks the right one.

.DESCRIPTION
  Variants (each declares exactly one runtime in SKSEPlugin_Version.compatibleVersions):
    SE-1.5.97     0x01050610   SKSE 2.0.20
    AE-1.6.1170   0x01064920   SKSE 2.2.6 (Steam)
    GOG-1.6.1179  0x010649B0   SKSE 2.2.6 (GOG)
    AE-1.7.104    0x01070680   SKSE 2.3.x
    Universal     (fallback)   version-independent; installed first, overridden by exact match

  The zip uses a FOMOD (MO2 / Vortex) with gameDependency patterns and priorities,
  so the highest matching version's DLL wins. No wizard choices for the user.
  Manual install: see README-INSTALL.txt inside the zip.

  Patching is idempotent and refuses to run if the code uses game addresses.

.EXAMPLE
  .\Convert-ModAtlas.ps1 -SourceDir C:\src\Mod-Atlas
  .\Convert-ModAtlas.ps1 -SourceDir C:\src\Mod-Atlas -PatchOnly
#>
[CmdletBinding()]
param(
  [Parameter(Mandatory)][string]$SourceDir,
  [string]$OutDir = (Join-Path $PWD 'dist'),
  [string]$PackageVersion = '0.4.1-multi',
  [switch]$PatchOnly
)
$ErrorActionPreference = 'Stop'
$SourceDir = (Resolve-Path $SourceDir).Path
$file = Join-Path $SourceDir 'src\Plugin.cpp'
if (-not (Test-Path $file)) { throw "src\Plugin.cpp not found in $SourceDir" }

# name, packed runtime (major<<24 | minor<<16 | build<<4), FOMOD game version ('' = fallback), priority
$Variants = @(
  @{ Name='Universal';     Packed=0;          Game='';         Prio=0 }
  @{ Name='SE-1.5.97';     Packed=0x01050610; Game='1.5.97';   Prio=1 }
  @{ Name='AE-1.6.1170';   Packed=0x01064920; Game='1.6.1170'; Prio=2 }
  @{ Name='GOG-1.6.1179';  Packed=0x010649B0; Game='1.6.1179'; Prio=3 }
  @{ Name='AE-1.7.104';    Packed=0x01070680; Game='1.7.104';  Prio=4 }
)

# ---- Safety: only valid for address-free plugins ----
$bad = Select-String -Path (Join-Path $SourceDir 'src\*.cpp'),(Join-Path $SourceDir 'src\*.h') `
        -Pattern 'REL::|RelocationID|0x14[0-9A-Fa-f]{7}|CommonLibSSE|RE::(?!InputEvent)' |
        Where-Object { $_.Line -notmatch 'namespace RE \{ class InputEvent' -and $_.Line -notmatch '^\s*//' }
if ($bad) { $bad | ForEach-Object { Write-Warning "$($_.Path):$($_.LineNumber) $($_.Line.Trim())" }
            throw 'Found game-address / CommonLib usage; a version declaration alone is not enough.' }

# ---- Patch (idempotent, two stages) ----
$s  = [IO.File]::ReadAllText($file)
$nl = if ($s.Contains("`r`n")) { "`r`n" } else { "`n" }
function Apply($old, $new) {
  $o = $old -replace "`r?`n", $nl; $n = $new -replace "`r?`n", $nl
  if (-not $script:s.Contains($o)) { throw "Patch anchor not found (source changed?): $($old.Split("`n")[0])" }
  $script:s = $script:s.Replace($o, $n)
}
if ($s -notmatch 'SKSEPlugin_Version') {
  Apply 'constexpr std::uint32_t runtime_1597 = 0x01050610;
}' @'
// SKSEPluginVersionData (SKSE 2.2.x+; needed to load on 1.6.x / 1.7.x / GOG). Layout is 848 bytes.
struct PluginVersionData {
    std::uint32_t dataVersion;
    std::uint32_t pluginVersion;
    char name[256];
    char author[256];
    char supportEmail[252];
    std::uint32_t versionIndependenceEx;
    std::uint32_t versionIndependence;
    std::uint32_t compatibleVersions[16];
    std::uint32_t seVersionRequired;
};
static_assert(sizeof(PluginVersionData) == 848);
constexpr std::uint32_t kVersionIndependent_Signatures = 1u << 1;
constexpr std::uint32_t kVersionIndependentEx_NoStructUse = 1u << 0;
}
'@
  Apply 'extern "C" __declspec(dllexport) bool SKSEPlugin_Query(' @'
// No game addresses, signatures or engine structs are used, so the plugin is runtime independent.
extern "C" __declspec(dllexport) constexpr skse_abi::PluginVersionData SKSEPlugin_Version = [] {
    skse_abi::PluginVersionData v{};
    v.dataVersion = 1;
    v.pluginVersion = 6;
    constexpr char name[] = "Mod Atlas";
    for (std::size_t i = 0; i < sizeof(name); ++i) v.name[i] = name[i];
    v.versionIndependence = skse_abi::kVersionIndependent_Signatures;
    v.versionIndependenceEx = skse_abi::kVersionIndependentEx_NoStructUse;
    return v;
}();

extern "C" __declspec(dllexport) bool SKSEPlugin_Query(
'@.TrimEnd("`r","`n")
  Apply 'return !skse->isEditor && skse->runtimeVersion == skse_abi::runtime_1597;' 'return !skse->isEditor;'
  Apply 'if (!skse || skse->isEditor || skse->runtimeVersion != skse_abi::runtime_1597 ||' 'if (!skse || skse->isEditor ||'
  Apply 'Log("Mod Atlas 0.4.1 loading for Skyrim 1.5.97");' 'Log(std::format("Mod Atlas 0.4.1 loading, runtime {}.{}.{}", skse->runtimeVersion >> 24, (skse->runtimeVersion >> 16) & 0xFF, (skse->runtimeVersion >> 4) & 0xFFF));'
  Write-Host 'Stage 1 patched (SKSEPlugin_Version, runtime gate removed).'
}
if ($s -notmatch 'MA_COMPAT_VERSION') {
  Apply '    v.versionIndependence = skse_abi::kVersionIndependent_Signatures;' @'
#if MA_COMPAT_VERSION
    v.compatibleVersions[0] = MA_COMPAT_VERSION;   // exact-runtime build
#else
    v.versionIndependence = skse_abi::kVersionIndependent_Signatures;   // universal fallback build
#endif
'@.TrimEnd("`r","`n")
  Apply '// No game addresses, signatures or engine structs are used' @'
#ifndef MA_COMPAT_VERSION
#define MA_COMPAT_VERSION 0
#endif
// No game addresses, signatures or engine structs are used
'@.TrimEnd("`r","`n")
  Write-Host 'Stage 2 patched (per-runtime MA_COMPAT_VERSION).'
}
[IO.File]::WriteAllText($file, $s, (New-Object Text.UTF8Encoding($false)))
if ($PatchOnly) { return }

# ---- Build one DLL per variant ----
$stage = Join-Path $OutDir 'stage'
Remove-Item $stage -Recurse -Force -ErrorAction SilentlyContinue
$useNinja = [bool](Get-Command ninja -ErrorAction SilentlyContinue)
$dumpbin = Get-ChildItem "${env:ProgramFiles}\Microsoft Visual Studio","${env:ProgramFiles(x86)}\Microsoft Visual Studio" `
           -Recurse -Filter dumpbin.exe -ErrorAction SilentlyContinue | Where-Object FullName -match 'Hostx64\\x64' | Select-Object -First 1
foreach ($v in $Variants) {
  Write-Host "`n== Build $($v.Name) ==" -ForegroundColor Cyan
  $bd = Join-Path $SourceDir "build-$($v.Name)"
  Remove-Item $bd -Recurse -Force -ErrorAction SilentlyContinue
  $env:CXXFLAGS = "/DMA_COMPAT_VERSION=0x{0:X8}" -f $v.Packed
  try {
    if ($useNinja) { cmake -S $SourceDir -B $bd -G Ninja -DCMAKE_BUILD_TYPE=Release }
    else           { cmake -S $SourceDir -B $bd -G 'Visual Studio 17 2022' -A x64 }
    if ($LASTEXITCODE) { throw "Configure failed ($($v.Name))" }
    cmake --build $bd --config Release --target SkyrimTutorials
    if ($LASTEXITCODE) { throw "Build failed ($($v.Name))" }
  } finally { Remove-Item Env:CXXFLAGS -ErrorAction SilentlyContinue }
  $dll = Get-ChildItem $bd -Recurse -Filter SkyrimTutorials.dll | Sort-Object LastWriteTime -Desc | Select-Object -First 1
  if (-not $dll) { throw "DLL not found ($($v.Name))" }
  if ($dumpbin) {
    $ex = & $dumpbin.FullName /exports $dll.FullName | Out-String
    foreach ($n in 'SKSEPlugin_Load','SKSEPlugin_Query','SKSEPlugin_Version') { if ($ex -notmatch $n) { throw "$($v.Name): missing export $n" } }
  }
  # Confirm the baked-in compatible version really is in the data blob.
  $bytes = [IO.File]::ReadAllBytes($dll.FullName)
  $needle = [BitConverter]::GetBytes([uint32]$v.Packed)
  $found = $false
  if ($v.Packed -ne 0) {
    for ($i = 0; $i -le $bytes.Length - 4; $i += 4) { if ($bytes[$i] -eq $needle[0] -and $bytes[$i+1] -eq $needle[1] -and $bytes[$i+2] -eq $needle[2] -and $bytes[$i+3] -eq $needle[3]) { $found = $true; break } }
    if (-not $found) { throw "$($v.Name): compatible version constant not found in DLL" }
  }
  $dest = Join-Path $stage "$($v.Name)\SKSE\Plugins"
  New-Item -ItemType Directory -Force $dest | Out-Null
  Copy-Item $dll.FullName $dest
  Write-Host "OK   $($v.Name)" -ForegroundColor Green
}

# ---- Docs + FOMOD + README ----
$docs = Join-Path $stage 'Docs\MiF'; New-Item -ItemType Directory -Force $docs | Out-Null
foreach ($f in 'AUTHORING.md','example-note.txt','example-pack.json') {
  $p = Join-Path $SourceDir $f; if (Test-Path $p) { Copy-Item $p $docs }
}
$fomod = Join-Path $stage 'fomod'; New-Item -ItemType Directory -Force $fomod | Out-Null
$dst = 'SKSE\Plugins\SkyrimTutorials.dll'
$patterns = foreach ($v in $Variants | Where-Object { $_.Game }) {
@"
    <pattern>
      <dependencies><gameDependency version="$($v.Game)"/></dependencies>
      <files><file source="$($v.Name)\$dst" destination="$dst" priority="$($v.Prio)"/></files>
    </pattern>
"@ }
@"
<config xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://qconsulting.ca/fo3/ModConfig5.0.xsd">
  <moduleName>Mod Atlas</moduleName>
  <requiredInstallFiles>
    <file source="Universal\$dst" destination="$dst" priority="0"/>
    <folder source="Docs" destination="Docs"/>
  </requiredInstallFiles>
  <installSteps order="Explicit">
    <installStep name="Runtime">
      <optionalFileGroups order="Explicit">
        <group name="Detected Skyrim runtime" type="SelectAll">
          <plugins order="Explicit">
            <plugin name="Automatic">
              <description>Installs the Mod Atlas DLL built for your Skyrim version (1.5.97, 1.6.1170, GOG 1.6.1179 or 1.7.104). If your version cannot be detected, a version-independent build is used.</description>
              <typeDescriptor><type name="Required"/></typeDescriptor>
            </plugin>
          </plugins>
        </group>
      </optionalFileGroups>
    </installStep>
  </installSteps>
  <conditionalFileInstalls>
    <patterns>
$($patterns -join "`r`n")
    </patterns>
  </conditionalFileInstalls>
</config>
"@ | Set-Content (Join-Path $fomod 'ModuleConfig.xml') -Encoding UTF8
[xml](Get-Content (Join-Path $fomod 'ModuleConfig.xml') -Raw) | Out-Null   # well-formedness check
@"
<fomod><Name>Mod Atlas</Name><Version>$PackageVersion</Version>
<Description>In-game guide browser and notes (SKSE Menu Framework). Multi-runtime build.</Description></fomod>
"@ | Set-Content (Join-Path $fomod 'info.xml') -Encoding UTF8
@'
Mod Atlas - multi-runtime build

Mod manager (MO2 / Vortex): install this zip. It picks the right DLL for your game version automatically.
Version-independent fallback is used if the manager cannot report your version.

Manual install: copy ONE of these folders' "SKSE" folder into your Skyrim "Data" folder, plus "Docs":
  SE-1.5.97     Skyrim SE 1.5.97   (SKSE 2.0.20)
  AE-1.6.1170   Skyrim AE 1.6.1170 (SKSE 2.2.6, Steam)
  GOG-1.6.1179  Skyrim GOG 1.6.1179 (GOG SKSE)
  AE-1.7.104    Skyrim 1.7.104     (SKSE 2.3.x)
  Universal     any of the above, if your version is not listed
Requires SKSE Menu Framework, which must support your runtime separately.
Untested on a runtime = unsupported. Attach Data/SKSE/Plugins/SkyrimTutorials.log to reports.
'@ | Set-Content (Join-Path $stage 'README-INSTALL.txt')

New-Item -ItemType Directory -Force $OutDir | Out-Null
$zip = Join-Path $OutDir "Mod-Atlas-$PackageVersion.zip"
Remove-Item $zip -ErrorAction SilentlyContinue
Compress-Archive -Path "$stage\*" -DestinationPath $zip
Write-Host "`nDone: $zip" -ForegroundColor Green
