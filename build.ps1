# Build MiniPascal on Windows PowerShell. No WSL required.
# Tools: Flex, Bison, and a C compiler (same lab tools as the course).

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

function Find-Cmd {
    param([string[]]$Names)
    foreach ($name in $Names) {
        $cmd = Get-Command $name -ErrorAction SilentlyContinue
        if ($cmd) { return $cmd.Source }
    }
    return $null
}

$bison = Find-Cmd @("bison", "win_bison")
$flex  = Find-Cmd @("flex", "win_flex")
$cc    = Find-Cmd @("gcc", "clang")

if (-not $bison -or -not $flex -or -not $cc) {
    Write-Host ""
    Write-Host "This project needs Flex, Bison, and gcc on PATH."
    Write-Host "Those are Windows programs. You do not need Ubuntu."
    Write-Host ""
    if (-not $bison) { Write-Host "  missing: bison  (or win_bison)" }
    if (-not $flex)  { Write-Host "  missing: flex   (or win_flex)" }
    if (-not $cc)    { Write-Host "  missing: gcc    (or clang)" }
    Write-Host ""
    Write-Host "Install once in PowerShell, then close and reopen the terminal:"
    Write-Host "  winget install -e --id WinFlexBison.win_flex_bison"
    Write-Host "  winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT"
    Write-Host ""
    Write-Host "If gcc still is not found, add the MinGW bin folder to PATH"
    Write-Host "(Code::Blocks and Dev-C++ already ship a gcc)."
    exit 1
}

Write-Host "bison : $bison"
Write-Host "flex  : $flex"
Write-Host "cc    : $cc"

New-Item -ItemType Directory -Force -Path compiler | Out-Null

& $bison -d -o compiler/parser.tab.c compiler/parser.y
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$flexName = [System.IO.Path]::GetFileNameWithoutExtension($flex)
if ($flexName -eq "win_flex") {
    & $flex --wincompat -o compiler/lex.yy.c compiler/lexer.l
} else {
    & $flex -o compiler/lex.yy.c compiler/lexer.l
}
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$cflags = @("-std=gnu11", "-Wall", "-Wextra", "-Icompiler")
$sources = @(
    "compiler/parser.tab.c",
    "compiler/lex.yy.c",
    "compiler/ast.c",
    "compiler/dump.c",
    "compiler/main.c"
)

& $cc @cflags -o minipascal.exe @sources
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ""
Write-Host "Built minipascal.exe"
Write-Host "  .\minipascal.exe samples\gcd.pas"
Write-Host "  .\minipascal.exe --tokens samples\gcd.pas"
Write-Host "  .\minipascal.exe samples\broken.pas"
Write-Host "  .\tests\run.ps1"
