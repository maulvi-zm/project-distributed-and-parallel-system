if [ -z "$1" ]; then
    echo "Usage: $0 <number_of_nodes>"
    exit 1
fi

N=$1 # Number of nodes

docker exec -it node1 sh -c "mpicc mpi.c -o mpi"
docker exec -it node1 sh -c "mpirun --allow-run-as-root -np $N --hostfile /mpi/hostfile mpi"
