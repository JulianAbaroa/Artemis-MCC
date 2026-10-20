# build_watcher.ps1
# Runs MSBuild when Tools/build.py asks for it, so Claude can compile from the device shell (a Linux VM that cannot run MSVC).
# Start it with Tools\build_watcher.bat and leave the window open while working with Claude.
# Exchange folder build\watch\ (gitignored): request.json (id, configuration, platform, target) -> status.json -> errors.log,
# warnings.log, result.json. Files are only overwritten, never deleted (the device shell cannot delete).
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$watch = Join-Path $root 'build\watch'
New-Item -ItemType Directory -Force -Path $watch | Out-Null
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
# The 64-bit MSBuild (Bin\amd64), like Visual Studio. The 32-bit one runs out of memory in the incremental LTCG link.
$found = @(& $vswhere -latest -prerelease -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\**\MSBuild.exe')
$msbuild = @($found | Where-Object { $_ -match '\\amd64\\' }) + @($found) | Select-Object -First 1
if (-not $msbuild) { Write-Host '[build_watcher] ERROR: MSBuild not found.'; Read-Host 'Enter to close'; exit 1 }
# The .vcxproj with the same Solution* properties Visual Studio passes (the .sln cannot be built from the command line: MSB5004,
# two projects named Artemis-MCC). Any difference (even '/' instead of '\' at the end of SolutionDir) changes the compiler command
# lines and makes every build after a Visual Studio build, and the other way round, a full rebuild.
$project = Join-Path $root 'Artemis\Artemis.vcxproj'
$solutionDir = $root.TrimEnd('\') + '\'
# A trailing '\' before the closing quote that PowerShell adds to paths with spaces would escape it: double it in that case.
$solutionDirArgument = if ($solutionDir.Contains(' ')) { $solutionDir + '\' } else { $solutionDir }
$solutionProperties = @("/p:SolutionDir=$solutionDirArgument", "/p:SolutionPath=$($solutionDir)Artemis-MCC.sln",
    '/p:SolutionName=Artemis-MCC', '/p:SolutionFileName=Artemis-MCC.sln', '/p:SolutionExt=.sln')
$request = Join-Path $watch 'request.json'
$errors = Join-Path $watch 'errors.log'
$warnings = Join-Path $watch 'warnings.log'
$scriptTime = (Get-Item $PSCommandPath).LastWriteTime

function Write-Json($name, $object)
{
    $object | ConvertTo-Json -Compress | Set-Content -Path (Join-Path $watch $name) -Encoding UTF8
}

# A request already there when the watcher starts is old: it is not built.
$last = $null
if (Test-Path $request) { try { $last = (Get-Content $request -Raw | ConvertFrom-Json).id } catch { } }
Write-Host "[build_watcher] MSBuild: $msbuild"
Write-Host "[build_watcher] Watching $watch. Leave this window open. Ctrl+C to stop."

while ($true)
{
    Start-Sleep -Milliseconds 700
    # When this script changes, the watcher restarts itself in the same window (no need to close it).
    if ((Get-Item $PSCommandPath).LastWriteTime -ne $scriptTime)
    {
        Write-Host '[build_watcher] Script changed: reloading.'
        & powershell -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath
        exit
    }
    try
    {
        if (-not (Test-Path $request)) { continue }
        $req = Get-Content $request -Raw | ConvertFrom-Json
        if (-not $req.id -or $req.id -eq $last) { continue }
        $last = $req.id
        $configuration = if ($req.configuration) { $req.configuration } else { 'Debug' }
        $platform = if ($req.platform) { $req.platform } else { 'x64' }
        $target = if ($req.target) { $req.target } else { 'Build' }
        Write-Json 'status.json' @{ id = $req.id; state = 'running'; started = (Get-Date).ToString('o') }
        Write-Host ""
        Write-Host "[build_watcher] $(Get-Date -Format HH:mm:ss) $target $configuration|$platform ($($req.id))"
        $watchTime = [Diagnostics.Stopwatch]::StartNew()
        & $msbuild $project "/t:$target" "/p:Configuration=$configuration" "/p:Platform=$platform" @solutionProperties `
            /m /nologo /v:minimal "/flp1:errorsonly;logfile=$errors;encoding=utf-8" "/flp2:warningsonly;logfile=$warnings;encoding=utf-8" 2>&1 |
            Tee-Object -FilePath (Join-Path $watch 'console.log')
        $code = $LASTEXITCODE
        $seconds = [math]::Round($watchTime.Elapsed.TotalSeconds, 1)
        Write-Json 'result.json' @{ id = $req.id; exitCode = $code; seconds = $seconds; configuration = $configuration; platform = $platform; target = $target; msbuild = $msbuild }
        Write-Json 'status.json' @{ id = $req.id; state = 'done' }
        Write-Host "[build_watcher] exit code $code in $seconds s"
    }
    catch
    {
        Write-Host "[build_watcher] ERROR: $($_.Exception.Message)"
        if ($req -and $req.id) { Write-Json 'result.json' @{ id = $req.id; exitCode = -1; seconds = 0; error = $_.Exception.Message } }
    }
}