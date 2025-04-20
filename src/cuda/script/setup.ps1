echo "Creating compiled code..."

nvcc -arch=sm_86 -o cuda cuda.cu

if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: Compilation failed."
    exit $LASTEXITCODE
}

echo "Compiled code created successfully!"