$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Mp = Join-Path $Root "minipascal.exe"
if (-not (Test-Path $Mp)) { $Mp = Join-Path $Root "minipascal" }
if (-not (Test-Path $Mp)) {
    Write-Host "Build first:  sh ./build.sh"
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

Invoke-Mp @() "program Demo;`nvar x: integer;`nbegin`n  x := 1`nend.`n" | Out-Null
$stdout = Get-Content $out -Raw -ErrorAction SilentlyContinue
if ($stdout -notmatch "Program") {
    Write-Host "FAIL typed MiniPascal"
    $fail = 1
} else {
    Write-Host "OK   typed MiniPascal"
}

$code = Invoke-Mp @((Join-Path $Root "samples/broken.pas"))
$stderr = Get-Content $err -Raw -ErrorAction SilentlyContinue
if ($code -eq 0) {
    Write-Host "FAIL broken.pas (expected syntax error)"
    $fail = 1
} elseif ($stderr -notmatch "parse error|lex error") {
    Write-Host "FAIL broken.pas (no error message)"
    $fail = 1
} else {
    Write-Host "OK   broken.pas (rejected)"
}

Invoke-Mp @("--tokens") "program A;`nvar a: array [1..5] of integer;`nbegin`nend.`n" | Out-Null
$tok = Get-Content $out -Raw -ErrorAction SilentlyContinue
if ($tok -notmatch "DOTDOT") {
    Write-Host "FAIL array range is not tokenized as .."
    $fail = 1
} else {
    Write-Host "OK   array range tokens"
}

if ($fail -ne 0) { exit 1 }
Write-Host "All tests passed."
