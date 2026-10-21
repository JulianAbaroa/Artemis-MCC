# game_watcher.ps1
# Deploys the Artemis DLL, launches and closes MCC and watches it for crashes, when Tools/game.py asks for it (Diseno/11, D129).
# Start it with Tools\game_watcher.bat and leave the window open while working with Claude. It does nothing to MCC on its own.
# Exchange folder build\game\ (gitignored):
#   request.json  {id, kind: deploy|launch|kill|status, launch: bool}   written by game.py
#   result.json   {id, ok, message}                                     answer to the last request
#   watcher.json  {heartbeat, mccRunning, pid, launchedAt, artemisLoaded, crashes, mccDir}   refreshed every second
#   Artemis.txt   copy of the Artemis log (next to the DLL in Binaries\Win64), refreshed when it changes
#   crashes\<time>\   Artemis log, MCC crash files and state.json of a run that ended without a kill request
#   config.json   optional overrides: {mccDir, launchMode: steam|exe, launchUrl}
# Files are only overwritten, never deleted.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$game = Join-Path $root 'build\game'
New-Item -ItemType Directory -Force -Path $game | Out-Null
$request = Join-Path $game 'request.json'
$scriptTime = (Get-Item $PSCommandPath).LastWriteTime
$processName = 'MCC-Win64-Shipping'
$appId = '976730'

function Write-Json($name, $object)
{
    $object | ConvertTo-Json -Compress -Depth 4 | Set-Content -Path (Join-Path $game $name) -Encoding UTF8
}

function Read-Config
{
    $path = Join-Path $game 'config.json'
    if (Test-Path $path) { try { return Get-Content $path -Raw | ConvertFrom-Json } catch { } }
    return [pscustomobject]@{}
}

# Binaries\Win64 of MCC: config.json, or the Steam libraries listed in libraryfolders.vdf.
function Find-MccDir($config)
{
    if ($config.mccDir) { return $config.mccDir }
    $steam = (Get-ItemProperty 'HKCU:\Software\Valve\Steam' -ErrorAction SilentlyContinue).SteamPath
    if (-not $steam) { return $null }
    $libraries = @($steam)
    $vdf = Join-Path $steam 'steamapps\libraryfolders.vdf'
    if (Test-Path $vdf)
    {
        foreach ($match in [regex]::Matches((Get-Content $vdf -Raw), '"path"\s+"([^"]+)"')) { $libraries += $match.Groups[1].Value.Replace('\\', '\') }
    }
    foreach ($library in $libraries)
    {
        $dir = Join-Path $library 'steamapps\common\Halo The Master Chief Collection\MCC\Binaries\Win64'
        if (Test-Path $dir) { return $dir }
    }
    return $null
}

function Get-Mcc { return Get-Process -Name $processName -ErrorAction SilentlyContinue | Select-Object -First 1 }

function Stop-Mcc
{
    $process = Get-Mcc
    if (-not $process) { return 'not running' }
    $script:killRequested = $true
    $process.CloseMainWindow() | Out-Null
    if (-not $process.WaitForExit(8000)) { Stop-Process -Id $process.Id -Force; $process.WaitForExit(10000) | Out-Null }
    Start-Sleep -Milliseconds 1500
    # The process is gone on purpose: the loop must not take it for a crash if a launch follows right away.
    $script:wasRunning = $false
    return 'closed'
}

function Start-Mcc($config)
{
    if (Get-Mcc) { return 'already running' }
    $script:killRequested = $false
    $script:launchedAt = Get-Date
    $mode = if ($config.launchMode) { $config.launchMode } else { 'steam' }
    if ($mode -eq 'exe')
    {
        # Direct start: steam_appid.txt keeps the exe from restarting itself through Steam (Steam must be running).
        Set-Content -Path (Join-Path $mccDir 'steam_appid.txt') -Value $appId -NoNewline
        Start-Process -FilePath (Join-Path $mccDir "$processName.exe") -WorkingDirectory $mccDir
        return 'started exe'
    }
    $url = if ($config.launchUrl) { $config.launchUrl } else { "steam://launch/$appId/dialog" }
    Start-Process $url
    return "opened $url"
}

function Save-Crash
{
    $stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
    $dir = Join-Path $game "crashes\$stamp"
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    $log = Join-Path $mccDir 'Artemis.txt'
    if (Test-Path $log) { Copy-Item $log (Join-Path $dir 'Artemis.txt') -Force }
    $state = Join-Path $game 'state.json'
    if (Test-Path $state) { Copy-Item $state (Join-Path $dir 'state.json') -Force }
    $since = if ($script:launchedAt) { $script:launchedAt } else { (Get-Date).AddHours(-1) }
    $places = @((Join-Path $env:LOCALAPPDATA 'MCC\Saved\Crashes'), (Join-Path $env:LOCALAPPDATA 'CrashDumps'),
        (Join-Path $env:LOCALAPPDATA 'MCC\Saved\Logs'), (Join-Path ($env:USERPROFILE) 'AppData\LocalLow\MCC\Temporary'))
    $copied = @()
    foreach ($place in $places)
    {
        if (-not (Test-Path $place)) { continue }
        foreach ($file in Get-ChildItem $place -Recurse -File -ErrorAction SilentlyContinue | Where-Object { $_.LastWriteTime -ge $since -and $_.Length -lt 300MB })
        {
            Copy-Item $file.FullName (Join-Path $dir $file.Name) -Force
            $copied += $file.FullName
        }
    }
    Write-Json "crashes\$stamp\files.json" @{ copied = $copied; launchedAt = "$since" }
    $script:crashes += 1
    $script:lastCrash = $stamp
    Write-Host "[game_watcher] $(Get-Date -Format HH:mm:ss) MCC ended without a kill request: saved crashes\$stamp ($($copied.Count) files)"
}

$config = Read-Config
$mccDir = Find-MccDir $config
if (-not $mccDir) { Write-Host '[game_watcher] ERROR: MCC Binaries\Win64 not found. Put {"mccDir": "..."} in build\game\config.json.'; Read-Host 'Enter to close'; exit 1 }
$killRequested = $true
$launchedAt = $null
$crashes = 0
$lastCrash = $null
$wasRunning = [bool](Get-Mcc)
$logTime = $null
$beat = 0
$last = $null
if (Test-Path $request) { try { $last = (Get-Content $request -Raw | ConvertFrom-Json).id } catch { } }
Write-Host "[game_watcher] MCC: $mccDir"
Write-Host "[game_watcher] Watching $game. Leave this window open. Ctrl+C to stop."

while ($true)
{
    Start-Sleep -Milliseconds 500
    if ((Get-Item $PSCommandPath).LastWriteTime -ne $scriptTime)
    {
        Write-Host '[game_watcher] Script changed: reloading.'
        & powershell -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath
        exit
    }
    try
    {
        $process = Get-Mcc
        if ($wasRunning -and -not $process -and -not $killRequested) { Save-Crash }
        $wasRunning = [bool]$process

        $log = Join-Path $mccDir 'Artemis.txt'
        if (Test-Path $log)
        {
            $time = (Get-Item $log).LastWriteTime
            if ($time -ne $logTime) { Copy-Item $log (Join-Path $game 'Artemis.txt') -Force; $logTime = $time }
        }
        $beat += 1
        if ($beat % 2 -eq 0)
        {
            $loaded = [bool]($process -and $launchedAt -and $logTime -and $logTime -ge $launchedAt)
            Write-Json 'watcher.json' @{ heartbeat = (Get-Date).ToString('o'); mccRunning = [bool]$process; pid = $(if ($process) { $process.Id } else { 0 });
                launchedAt = "$launchedAt"; artemisLoaded = $loaded; crashes = $crashes; lastCrash = $lastCrash; mccDir = $mccDir }
        }

        if (-not (Test-Path $request)) { continue }
        $req = Get-Content $request -Raw | ConvertFrom-Json
        if (-not $req.id -or $req.id -eq $last) { continue }
        $last = $req.id
        $config = Read-Config
        Write-Host "[game_watcher] $(Get-Date -Format HH:mm:ss) $($req.kind) ($($req.id))"
        $message = ''
        switch ($req.kind)
        {
            'deploy'
            {
                $message = 'MCC ' + (Stop-Mcc)
                $out = Join-Path $root 'build\x64\Release'
                Copy-Item (Join-Path $out 'wtsapi32.dll') $mccDir -Force
                if (Test-Path (Join-Path $out 'wtsapi32.pdb')) { Copy-Item (Join-Path $out 'wtsapi32.pdb') $mccDir -Force }
                Set-Content -Path (Join-Path $mccDir 'artemis_bench.txt') -Value $game -NoNewline
                $message += '; dll copied'
                if ($req.launch) { $message += '; ' + (Start-Mcc $config) }
            }
            'launch' { $message = Start-Mcc $config }
            'kill' { $message = 'MCC ' + (Stop-Mcc) }
            'status' { $message = 'ok' }
            'info'
            {
                # Settings of Artemis next to the DLL and the recorded match configs of the storage folder (%LOCALAPPDATA%\Artemis).
                $ini = Join-Path $mccDir 'config.ini'
                $iniText = if (Test-Path $ini) { (Get-Content $ini -Raw).Trim() } else { 'missing' }
                $library = Join-Path $env:LOCALAPPDATA 'Artemis\MatchConfigs'
                $configs = if (Test-Path $library) { (Get-ChildItem $library -File | ForEach-Object { $_.Name }) -join ', ' } else { 'folder missing' }
                $message = "config.ini: $iniText | $library : $configs"
            }
            default { throw "unknown request kind '$($req.kind)'" }
        }
        Write-Json 'result.json' @{ id = $req.id; ok = $true; message = $message }
        Write-Host "[game_watcher] $message"
    }
    catch
    {
        Write-Host "[game_watcher] ERROR: $($_.Exception.Message)"
        if ($req -and $req.id) { Write-Json 'result.json' @{ id = $req.id; ok = $false; message = $_.Exception.Message } }
    }
}