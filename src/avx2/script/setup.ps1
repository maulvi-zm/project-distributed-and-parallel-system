echo "Creating compiled code..."

gcc -mavx2 -O2 avx.c -o avx2 -lm -mfma

if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: Compilation failed."
    exit $LASTEXITCODE
}

echo "Compiled code created successfully!"
