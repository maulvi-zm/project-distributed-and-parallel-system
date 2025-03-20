if [ -z "$1" ]; then
    echo "Usage: $0 <number_of_nodes>"
    exit 1
fi

N=$1 # Number of nodes

# Create Docker network
docker network create mpi-net

# Create containers dynamically
for i in $(seq 1 $N); do
    docker run -dit --name node$i --network mpi-net openmpi-cluster
done

# Generate SSH key on node1
docker exec -it node1 ssh-keygen -t rsa -b 4096 -N "" -f /root/.ssh/id_rsa
docker exec -it node1 touch /root/.ssh/authorized_keys
docker exec -it node1 cat /root/.ssh/id_rsa.pub >>/root/.ssh/authorized_keys

# Copy public key from node1
docker cp node1:/root/.ssh/id_rsa.pub .

# Distribute SSH key to all nodes
for i in $(seq 2 $N); do
    docker cp id_rsa.pub node$i:/root/.ssh/temp_key
    docker exec -it node$i sh -c "cat /root/.ssh/temp_key >> /root/.ssh/authorized_keys && rm /root/.ssh/temp_key"
done

# Delete public key from current directory
rm id_rsa.pub

# Update known_hosts in node1
for i in $(seq 2 $N); do
    docker exec -it node1 sh -c "ssh-keyscan node$i >> /root/.ssh/known_hosts"
done

# Create hostfile in /mpi
docker exec -it node1 sh -c "mkdir -p /mpi && touch /mpi/hostfile"
for i in $(seq 1 $N); do
    docker exec -it node1 sh -c "echo node$i >> /mpi/hostfile"
done

echo "All $N nodes are set up and ready!"
