# Builds the Clip Inbox Windows installer from an installed plugin folder (the output of cmake --install).
[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $Version,
    [Parameter(Mandatory)]
    [string] $PluginDir,
    [Parameter(Mandatory)]
    [string] $OutputDir,
    [Parameter(Mandatory)]
    [string] $OutputName
)

$ErrorActionPreference = 'Stop'

# Inno Setup can live in Program Files or, for a per-user install, in LocalAppData.
$Candidates = @(
    (Get-Command iscc -ErrorAction SilentlyContinue).Source
    "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
    "${env:ProgramFiles}\Inno Setup 6\ISCC.exe"
    "${env:LOCALAPPDATA}\Programs\Inno Setup 6\ISCC.exe"
)
$Compiler = $Candidates | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1

if ( -not $Compiler ) {
    if ( $env:CI ) {
        choco install innosetup --no-progress -y | Out-Host
        $Compiler = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
    } else {
        throw 'Inno Setup 6 is not installed. Install it with: winget install JRSoftware.InnoSetup'
    }
}

$Script = Join-Path $PSScriptRoot 'clip-inbox.iss'
& $Compiler /Qp "/DAppVersion=${Version}" "/DPluginDir=${PluginDir}" "/DOutputDir=${OutputDir}" "/DOutputName=${OutputName}" $Script

if ( $LASTEXITCODE -ne 0 ) {
    throw "Inno Setup failed with exit code ${LASTEXITCODE}"
}
