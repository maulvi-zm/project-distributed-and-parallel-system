# Floyd-Warshall Algorithm Parallelization with Open MPI

## Description and Parallelization Explanation
This program parallelize the Floyd-Warshall algorithm in a path-finding problem using Open MPI. The program first calls MPI_Init to initialize MPI and so that each process can know it's rank and the total process. The program is designed so that only process with rank 0 may accept the input and initialize the graph. After that, the program will broadcast the graph to all processes using MPI_Bcast. The parallelization of the Floyd-Warshall algorithm is done by dividing the graph's row into almost equal parts (almost because the modulo of row and number of process might be unequal) for all processes. This is done within the floyd_warshall procedure with this step:
1. Get the number of rows to be processed for a process (including remainder distribution)
2. Get the start and final index of the graph to be calculated by the current process
3. Each process: calculate the distance between nodes in the graph from start to final index
4. If current process's rank is 0, receive the result from all processes using MPI_Recv
5. If current process's rank is not 0, send the process's result to process with rank 0

After process with rank 0 finished calculating it's own part and other processes are done sending their results, process with rank 0 will show the final result.

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

### Side Note
Test cases are available in the test_case folder

## Results

### Test Case 1 (case1.txt)

#### Parameter r = 1

- Serial
![Case 1, Serial, r = 1](img/temp.png)
- Open MPI with [n] processes
![Case 1, Open MPI, r = 1](img/temp.png)
- Speed-Up = [speed-up]

#### Parameter r = 2

- Serial
![Case 1, Serial, r = 2](img/temp.png)
- Open MPI with [n] processes
![Case 1, Open MPI, r = 2](img/temp.png)
- Speed-Up = [speed-up]

#### Parameter r = inf

- Serial
![Case 1, Serial, r = inf](img/temp.png)
- Open MPI with [n] processes
![Case 1, Open MPI, r = inf](img/temp.png)
- Speed-Up = [speed-up]

### Test Case 2 (case2.txt)

#### Parameter r = 1

- Serial
![Case 2, Serial, r = 1](img/temp.png)
- Open MPI with [n] processes
![Case 2, Open MPI, r = 1](img/temp.png)
- Speed-Up = [speed-up]

#### Parameter r = 2

- Serial
![Case 2, Serial, r = 2](img/temp.png)
- Open MPI with [n] processes
![Case 2, Open MPI, r = 2](img/temp.png)
- Speed-Up = [speed-up]

#### Parameter r = inf

- Serial
![Case 2, Serial, r = inf](img/temp.png)
- Open MPI with [n] processes
![Case 2, Open MPI, r = inf](img/temp.png)
- Speed-Up = [speed-up]

### Test Case 3 (case3.txt)

#### Parameter r = 1

- Serial
![Case 3, Serial, r = 1](img/temp.png)
- Open MPI with [n] processes
![Case 3, Open MPI, r = 1](img/temp.png)
- Speed-Up = [speed-up]

#### Parameter r = 2

- Serial
![Case 3, Serial, r = 2](img/temp.png)
- Open MPI with [n] processes
![Case 3, Open MPI, r = 2](img/temp.png)
- Speed-Up = [speed-up]

#### Parameter r = inf

- Serial
![Case 3, Serial, r = inf](img/temp.png)
- Open MPI with [n] processes
![Case 3, Open MPI, r = inf](img/temp.png)
- Speed-Up = [speed-up]

### Test Case 4 (case4.txt)

#### Parameter r = 1

- Serial
![Case 4, Serial, r = 1](img/temp.png)
- Open MPI with [n] processes
![Case 4, Open MPI, r = 1](img/temp.png)
- Speed-Up = [speed-up]

#### Parameter r = 2

- Serial
![Case 4, Serial, r = 2](img/temp.png)
- Open MPI with [n] processes
![Case 4, Open MPI, r = 2](img/temp.png)
- Speed-Up = [speed-up]

#### Parameter r = inf

- Serial
![Case 4, Serial, r = inf](img/temp.png)
- Open MPI with [n] processes
![Case 4, Open MPI, r = inf](img/temp.png)
- Speed-Up = [speed-up]
