$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Mp = Join-Path $Root "minipascal.exe"
if (-not (Test-Path $Mp)) { $Mp = Join-Path $Root "minipascal" }
if (-not (Test-Path $Mp)) {
    Write-Host "Build first:  .\build.ps1   or   sh ./build.sh"
    exit 1
}

$fail = 0
$out = Join-Path $env:TEMP "mp-out.txt"
$err = Join-Path $env:TEMP "mp-err.txt"

function Invoke-Mp {
    param([string[]]$Args, [string]$InputText = $null)
    if ($null -ne $InputText) {
        $InputText | & $Mp @Args > $out 2> $err
        return $LASTEXITCODE
    }
    $p = Start-Process -FilePath $Mp -ArgumentList $Args -NoNewWindow -Wait -PassThru `
        -RedirectStandardOutput $out -RedirectStandardError $err
    return $p.ExitCode
}

function Test-OkTree([string]$file) {
    $code = Invoke-Mp @("--tree", (Join-Path $Root $file))
    $stdout = Get-Content $out -Raw -ErrorAction SilentlyContinue
    $stderr = Get-Content $err -Raw -ErrorAction SilentlyContinue
    if ($code -ne 0) {
        Write-Host "FAIL $file (expected success)"
        if ($stderr) { Write-Host $stderr }
        $script:fail = 1
    } elseif ($stdout -notmatch "Program") {
        Write-Host "FAIL $file (no Program node)"
        Write-Host $stdout
        $script:fail = 1
    } else {
        Write-Host "OK   $file (tree)"
    }
}

function Test-Bad([string]$file) {
    $code = Invoke-Mp @("--tree", (Join-Path $Root $file))
    $stderr = Get-Content $err -Raw -ErrorAction SilentlyContinue
    if ($code -eq 0) {
        Write-Host "FAIL $file (expected syntax error)"
        $script:fail = 1
    } elseif ($stderr -notmatch "parse error|lex error") {
        Write-Host "FAIL $file (no error message)"
        if ($stderr) { Write-Host $stderr }
        $script:fail = 1
    } else {
        Write-Host "OK   $file (rejected)"
    }
}

Test-OkTree "samples/gcd.pas"
Test-OkTree "samples/factorial.pas"
Test-OkTree "samples/bubble.pas"
Test-Bad "samples/broken.pas"

$code = Invoke-Mp @("--tokens", (Join-Path $Root "samples/bubble.pas"))
$tok = Get-Content $out -Raw -ErrorAction SilentlyContinue
if ($tok -notmatch "DOTDOT") {
    Write-Host "FAIL array range is not tokenized as .."
    $fail = 1
} else {
    Write-Host "OK   array range tokens"
}

Invoke-Mp @("--run", (Join-Path $Root "samples/gcd.pas")) "48 18`n" | Out-Null
$run = Get-Content $out -Raw -ErrorAction SilentlyContinue
if ($run -notmatch "gcd = 6") {
    Write-Host "FAIL gcd run"
    $fail = 1
} else {
    Write-Host "OK   gcd run"
}

if ($fail -ne 0) { exit 1 }
Write-Host "All tests passed."
