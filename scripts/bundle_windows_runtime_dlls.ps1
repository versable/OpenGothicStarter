param(
  [Parameter(Mandatory = $true)]
  [string]$BuildDir,
  [Parameter(Mandatory = $true)]
  [string]$VcpkgRoot
)

$ErrorActionPreference = "Stop"

$buildDirAbs = (Resolve-Path $BuildDir).Path
$exePath = Join-Path $buildDirAbs "OpenGothicStarter.exe"
if (-not (Test-Path $exePath)) {
  throw "Launcher executable not found: $exePath"
}

$candidateRuntimeDirs = @(
  (Join-Path $buildDirAbs "vcpkg_installed/x64-windows/bin"),
  (Join-Path $VcpkgRoot "installed/x64-windows/bin")
)

if ($env:GITHUB_WORKSPACE) {
  $candidateRuntimeDirs += (Join-Path $env:GITHUB_WORKSPACE "vcpkg_installed/x64-windows/bin")
}

$runtimeDirs = $candidateRuntimeDirs | Where-Object { $_ -and (Test-Path $_) } | Select-Object -Unique
if (-not $runtimeDirs) {
  throw "Could not find any vcpkg runtime directories. Checked: $($candidateRuntimeDirs -join ', ')"
}

function Get-DllDependents {
  param([string]$BinaryPath)

  $output = & dumpbin /dependents $BinaryPath 2>$null
  if ($LASTEXITCODE -ne 0) {
    throw "dumpbin failed for '$BinaryPath'"
  }

  return $output |
    ForEach-Object { $_.Trim() } |
    Where-Object { $_ -match '^[A-Za-z0-9._-]+\.dll$' } |
    Sort-Object -Unique
}

function Find-RuntimeDllPath {
  param(
    [string]$DllName,
    [string[]]$SearchDirs
  )

  foreach ($dir in $SearchDirs) {
    $candidate = Join-Path $dir $DllName
    if (Test-Path $candidate) {
      return $candidate
    }
  }

  return $null
}

$seenDllNames = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$queuedBinaries = [System.Collections.Generic.Queue[string]]::new()
$queuedBinaries.Enqueue($exePath)
$bundledCount = 0
$bundledDllNames = [System.Collections.Generic.List[string]]::new()

while ($queuedBinaries.Count -gt 0) {
  $binary = $queuedBinaries.Dequeue()
  foreach ($dllName in Get-DllDependents -BinaryPath $binary) {
    if (-not $seenDllNames.Add($dllName)) {
      continue
    }

    $sourceDll = Find-RuntimeDllPath -DllName $dllName -SearchDirs $runtimeDirs
    if (-not $sourceDll) {
      continue
    }

    $destDll = Join-Path $buildDirAbs $dllName
    Copy-Item $sourceDll $destDll -Force
    $bundledCount += 1
    $bundledDllNames.Add($dllName)
    $queuedBinaries.Enqueue($destDll)
  }
}

if ($bundledCount -eq 0) {
  throw "No vcpkg runtime DLLs were bundled for '$exePath'."
}

$bundledDllNames.Sort()
Write-Host "Bundled $bundledCount runtime DLL(s) into '$buildDirAbs':"
foreach ($dllName in $bundledDllNames) {
  Write-Host " - $dllName"
}
