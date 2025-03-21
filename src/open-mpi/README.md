# Floyd-Warshall Algorithm Parallelization with Open MPI

## Description
This program parallelize the Floyd-Warshall algorithm in a path-finding problem using Open MPI.

## Prerequisites
- Docker
- Terminal (MacOS/Linux) or PowerShell (Windows)

## Usage
### 1. Build the Docker Image
```
docker build -t openmpi-cluster .
```

### 2. Set-Up & Run<br>
For MacOS/Linux
```
chmod +x /script/setup.sh
chmod +x /script/run.sh

./script/setup.sh <numofnodes>
./script/run.sh <numofnodes> <test_case>
```
For Windows (using PowerShell)
```
./script/setup.ps1 <numofnodes>
./script/run.ps1 <numofnodes> <test_case>
```

### Side Notes
Test cases are available in the test_case folder