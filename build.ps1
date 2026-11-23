$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $root 'build'

cmake -S $root -B $buildDir -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

cmake --build $buildDir --config Release
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

ctest --test-dir $buildDir -C Release --output-on-failure
exit $LASTEXITCODE
