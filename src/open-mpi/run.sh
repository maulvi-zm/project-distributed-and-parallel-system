if [ -z "$1" ]; then
    echo "Usage: $0 <number_of_nodes> <test_num>"
    exit 1
fi

if [ -z "$2" ]; then
    echo "Usage: $0 <number_of_nodes> <test_num>"
    exit 1
fi

N=$1 # Number of nodes
TESTCASE=$2

docker exec -it node1 sh -c "mpicc mpi.c -o mpi -lm"
docker exec -it node1 sh -c "mpirun --allow-run-as-root -np $N --hostfile /mpi/hostfile mpi < test_case/case$TESTCASE.txt > out-$TESTCASE.txt"
