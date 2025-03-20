param (
    [int]$N,        # Number of nodes
    [int]$TESTCASE  # Test case number
)

if (-not $N -or -not $TESTCASE) {
    Write-Host "Usage: .\mpi-run.ps1 -N <number_of_nodes> -TESTCASE <test_num>"
    exit 1
}

Write-Host "Compiling MPI program on node1..."
docker exec -it node1 sh -c "mpicc mpi.c -o mpi -lm"

Write-Host "Running MPI program with $N nodes (Test Case: $TESTCASE)..."
docker exec -it node1 sh -c "mpirun --allow-run-as-root -np $N --hostfile /mpi/hostfile mpi < test_case/case$TESTCASE.txt > out-$TESTCASE.txt"

Write-Host "Execution complete! Output saved in out-$TESTCASE.txt"
