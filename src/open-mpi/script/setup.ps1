param (
    [int]$N  # Number of nodes
)

if (-not $N) {
    Write-Host "Usage: .\mpi-setup.ps1 -N <number_of_nodes>"
    exit 1
}

# Create Docker network
Write-Host "Creating Docker network..."
docker network create mpi-net

# Create containers dynamically
Write-Host "Creating $N MPI containers..."
for ($i = 1; $i -le $N; $i++) {
    docker run -dit --name "node${i}" --network mpi-net openmpi-cluster
}

# Generate SSH key on node1
Write-Host "Generating SSH key on node1..."
docker exec -it node1 sh -c "ssh-keygen -t rsa -b 4096 -f /root/.ssh/id_rsa -q"
docker exec -it node1 touch /root/.ssh/authorized_keys
docker exec -it node1 sh -c "cat /root/.ssh/id_rsa.pub >> /root/.ssh/authorized_keys"

# Copy SSH key to host machine
Write-Host "Copying public key from node1 to host..."
docker cp node1:/root/.ssh/id_rsa.pub .\id_rsa.pub

# Distribute SSH key to all nodes
Write-Host "Distributing SSH key to all nodes..."
for ($i = 2; $i -le $N; $i++) {
    docker cp .\id_rsa.pub "node${i}:/root/.ssh/temp_key"
    docker exec -it "node$i" sh -c "cat /root/.ssh/temp_key >> /root/.ssh/authorized_keys && rm /root/.ssh/temp_key"
}

# Delete public key from host
Remove-Item .\id_rsa.pub -Force

# Update known_hosts in node1
Write-Host "Updating known_hosts on node1..."
for ($i = 2; $i -le $N; $i++) {
    docker exec -it node1 sh -c "ssh-keyscan node${i} >> /root/.ssh/known_hosts"
}

# Create hostfile inside /mpi
Write-Host "Creating hostfile in /mpi..."
docker exec -it node1 sh -c "mkdir -p /mpi && touch /mpi/hostfile"
for ($i = 1; $i -le $N; $i++) {
    docker exec -it node1 sh -c "echo node${i} >> /mpi/hostfile"
}

# Create output directory
Write-Host "Creating output directory..."
docker exec -it node1 sh -c "mkdir -p /mpi/output"

Write-Host "All $N nodes are set up and ready!"
