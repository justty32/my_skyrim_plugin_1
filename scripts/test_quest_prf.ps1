[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot
$Compiler = (Get-Command g++ -ErrorAction Stop).Source

$OutputDir = Join-Path $RepoRoot 'build/portable-tests-mingw'
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$Executable = Join-Path $OutputDir 'quest_prf_test.exe'

$Arguments = @(
    '-std=c++23', '-O2', '-Wall', '-Wextra', '-pedantic-errors',
    '-finput-charset=UTF-8', '-fexec-charset=UTF-8',
    ('-I' + (Join-Path $RepoRoot 'src/core')),
    (Join-Path $RepoRoot 'src/core/DeterministicRandom.cpp'),
    (Join-Path $RepoRoot 'tools/quest_prf_test.cpp'),
    '-o', $Executable
)

& $Compiler @Arguments
if ($LASTEXITCODE -ne 0) { throw "g++ failed with exit code $LASTEXITCODE" }

& $Executable
if ($LASTEXITCODE -ne 0) { throw "quest_prf_test failed with exit code $LASTEXITCODE" }
