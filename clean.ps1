$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $root 'build'

if (Test-Path $buildDir) {
    Remove-Item -LiteralPath $buildDir -Recurse -Force
}
