$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $root 'build'

if (-not (Test-Path (Join-Path $buildDir 'CTestTestfile.cmake'))) {
    Write-Error 'Build the project before running tests: .\build.ps1'
    exit 1
}

ctest --test-dir $buildDir -C Release --output-on-failure
exit $LASTEXITCODE
