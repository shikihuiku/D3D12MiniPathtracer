param(
    [string]$Configuration = "Debug",
    [string]$Platform = "x64",
    [switch]$Rebuild,
    [switch]$Clean,
    [switch]$Run
)

# Find MSBuild using vswhere
$vswherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswherePath) {
    $msbuildPath = & $vswherePath -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1
} else {
    Write-Error "vswhere not found. Please install Visual Studio 2022."
    exit 1
}

if (-not $msbuildPath -or -not (Test-Path $msbuildPath)) {
    Write-Error "MSBuild not found. Please install Visual Studio 2022."
    exit 1
}

Write-Host "Using MSBuild: $msbuildPath" -ForegroundColor Green

$solution = "D3D12MiniPathtracer.sln"

# Clean build
if ($Clean) {
    Write-Host "Cleaning $Configuration build..." -ForegroundColor Yellow
    & $msbuildPath $solution /t:Clean /p:Configuration=$Configuration /p:Platform=$Platform
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Clean failed!"
        exit $LASTEXITCODE
    }
    if (-not $Rebuild) {
        Write-Host "Clean completed!" -ForegroundColor Green
        exit 0
    }
}

# Build or Rebuild
$target = if ($Rebuild) { "Rebuild" } else { "Build" }
Write-Host "Performing $target for $Configuration|$Platform..." -ForegroundColor Yellow

& $msbuildPath $solution /t:$target /p:Configuration=$Configuration /p:Platform=$Platform /m

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed!"
    exit $LASTEXITCODE
}

Write-Host "Build succeeded!" -ForegroundColor Green
$exePath = "bin\$Configuration\D3D12MiniPathtracer.exe"
Write-Host "Executable: $exePath" -ForegroundColor Cyan

# Run if requested
if ($Run -and (Test-Path $exePath)) {
    Write-Host "`nRunning application..." -ForegroundColor Yellow
    Start-Process $exePath
}
