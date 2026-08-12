#Requires -Version 5.1
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$fixtureRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("daylight-dungeon-packaging-{0}" -f [guid]::NewGuid())
$startingLocation = (Get-Location).Path

function Assert-True([bool]$Condition, [string]$Message) {
    if (-not $Condition) {
        throw $Message
    }
}

try {
    $fixtureScripts = Join-Path $fixtureRoot 'scripts'
    $fixtureConfig = Join-Path $fixtureRoot 'config'
    $fixtureBuild = Join-Path $fixtureRoot 'build\release-msvc'
    $fixtureDist = Join-Path $fixtureRoot 'test-dist'

    New-Item -ItemType Directory -Path $fixtureScripts, $fixtureConfig, $fixtureBuild | Out-Null
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'pack.ps1') -Destination $fixtureScripts
    Copy-Item -LiteralPath (Join-Path $repoRoot 'config\FollowLight.ini') -Destination $fixtureConfig

    @(
        'CMAKE_PROJECT_NAME:STATIC=DaylightDungeon'
        'CMAKE_PROJECT_VERSION:STATIC=0.0.1'
        'PLUGIN_CONFIG_FOLDER:STRING=DaylightDungeon'
    ) | Set-Content -LiteralPath (Join-Path $fixtureBuild 'CMakeCache.txt')
    [System.IO.File]::WriteAllBytes((Join-Path $fixtureBuild 'DaylightDungeon.dll'), [byte[]](0x4d, 0x5a))

    & (Join-Path $fixtureScripts 'pack.ps1') -OutputDir $fixtureDist
    Assert-True ((Get-Location).Path -eq $startingLocation) 'pack.ps1 changed the caller working directory.'

    $zipPath = Join-Path $fixtureDist 'DaylightDungeon-0.0.1.zip'
    Assert-True (Test-Path -LiteralPath $zipPath) "Expected package '$zipPath' was not created."

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
    try {
        $entries = @($archive.Entries | ForEach-Object { $_.FullName.Replace('\', '/') })
        Assert-True ($entries -contains 'Data/SKSE/Plugins/DaylightDungeon.dll') 'Package is missing the plugin DLL.'
        Assert-True ($entries -contains 'Data/SKSE/Plugins/DaylightDungeon/FollowLight.ini') 'Config files are not under the runtime DaylightDungeon directory.'
        Assert-True (-not ($entries | Where-Object { $_ -like 'Data/SKSE/Plugins/Template_Plugin/*' })) 'Package still contains the pre-rename Template_Plugin directory.'
    }
    finally {
        $archive.Dispose()
    }

    $unsafeOutputRejected = $false
    try {
        & (Join-Path $fixtureScripts 'pack.ps1') -OutputDir (Join-Path $fixtureRoot 'pack\out')
    }
    catch {
        $unsafeOutputRejected = $_.Exception.Message -match 'must not be the packaging staging directory'
    }
    Assert-True $unsafeOutputRejected 'pack.ps1 did not reject an output directory inside its staging tree.'

    $bashPack = Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'pack.sh')
    Assert-True ($bashPack -match 'get_cache_value "PLUGIN_CONFIG_FOLDER"') 'pack.sh must read the config directory from CMakeCache.txt.'
    Assert-True ($bashPack -notmatch 'CONFIG_FOLDER_NAME="Template_Plugin"') 'pack.sh still hardcodes the pre-rename config directory.'
    Assert-True ($bashPack -match '-h \| --help\) usage 0') 'pack.sh --help must report success.'

    $cmakeLists = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'CMakeLists.txt')
    Assert-True ($cmakeLists -match 'set\(PLUGIN_CONFIG_FOLDER "\$\{PROJECT_NAME\}" CACHE STRING') 'CMake must publish the config directory in CMakeCache.txt.'

    $workflow = Get-Content -Raw -LiteralPath (Join-Path $repoRoot '.github\workflows\build.yml')
    Assert-True ($workflow -notmatch 'TemplatePlugin') 'The release workflow still refers to the pre-rename DLL or artifact.'

    Write-Host 'Packaging contract tests passed.'
}
finally {
    Set-Location -LiteralPath $startingLocation
    if (Test-Path -LiteralPath $fixtureRoot) {
        Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
    }
}
