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
    param([string[]]$Arguments, [string]$InputText = $null)
    if ($null -ne $InputText) {
        $InputText | & $Mp @Arguments > $out 2> $err
        return $LASTEXITCODE
    }
    $p = Start-Process -FilePath $Mp -ArgumentList $Arguments -NoNewWindow -Wait -PassThru `
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

$trailing = Join-Path $env:TEMP "mp-trailing.pas"
Set-Content -Path $trailing -Value "program A; begin end. garbage"
$code = Invoke-Mp @($trailing)
if ($code -eq 0) {
    Write-Host "FAIL trailing input after final dot was accepted"
    $fail = 1
} else {
    Write-Host "OK   trailing input rejected"
}

$code = Invoke-Mp @("--run") "program A;`nbegin`nwriteln(1 / 0)`nend.`n"
$stderr = Get-Content $err -Raw -ErrorAction SilentlyContinue
if ($code -eq 0 -or $stderr -notmatch "division by zero") {
    Write-Host "FAIL division by zero was not reported"
    $fail = 1
} else {
    Write-Host "OK   division by zero reported"
}

$code = Invoke-Mp @("--run") "program A;`nbegin`nwriteln('a' = 'b')`nend.`n"
$stdout = Get-Content $out -Raw -ErrorAction SilentlyContinue
if ($code -ne 0 -or $stdout -notmatch "0") {
    Write-Host "FAIL unequal string comparison"
    $fail = 1
} else {
    Write-Host "OK   unequal strings compare false"
}

if ($fail -ne 0) { exit 1 }

$native = Join-Path $env:TEMP "mp-native-test.exe"
$code = Invoke-Mp @("--compile", (Join-Path $Root "samples/native-demo.pas"), "-o", $native)
if ($code -ne 0 -or -not (Test-Path $native)) {
    Write-Host "FAIL standalone executable generation"
    exit 1
}
$nativeOutput = & $native
if ($LASTEXITCODE -ne 0 -or $nativeOutput.Trim() -ne "5") {
    Write-Host "FAIL standalone executable output"
    exit 1
}
Write-Host "OK   standalone executable outputs 5"
Write-Host "All tests passed."
