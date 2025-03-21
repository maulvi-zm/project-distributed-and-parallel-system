param (
    [int]$TESTCASE 
)

if (-not $TESTCASE) {
    Write-Host "Usage: .\run.ps1 -TESTCASE <test_num>"
    exit 1
}

Write-Host "Running test case $TESTCASE..."

if (-not (Test-Path -Path "output")) {
    New-Item -ItemType Directory -Path "output"
}

if (-not (Test-Path -Path "output/out-$TESTCASE.txt")) {
    New-Item -ItemType File -Path "output/out-$TESTCASE.txt"
}

Get-Content "test_case/case$TESTCASE.txt" | .\mp > "output/out-$TESTCASE.txt"

if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: Test case $TESTCASE failed."
    exit $LASTEXITCODE
}

Write-Host "Test case $TESTCASE completed. Output saved to output/out-$TESTCASE.txt."