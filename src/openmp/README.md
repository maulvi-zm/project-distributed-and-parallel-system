# Floyd-Warshall Algorithm Parallelization with Open MP

## Description
This program parallelize the Variant of Floyd-Warshall algorithm, Blocked Floyd-Warshall algorithm in a path-finding problem using Open MP. This algorithm will use the Parallel Floyd-Warshall if maxtrix size is not possible for division.

## Prerequisites
- Terminal (MacOS/Linux) or PowerShell (Windows)

## Usage
### 1. Set-Up & Run<br>
For MacOS/Linux
```
ON PROGRESS
```
For Windows using PowerShell
1. Compile the C file (open mp should already included in MSVC)
    ```
    ./script/setup.ps1
    ```
2. Run testcase with x in range of (1 - 4)
    ```
    ./script/run.ps1 X
    ```

### Side Notes
Test cases are available in the test_case folder