echo "Creating compiled code..."

gcc mp.c -o mp -fopenmp -lm

if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: Compilation failed."
    exit $LASTEXITCODE
}

echo "Compiled code created successfully!"