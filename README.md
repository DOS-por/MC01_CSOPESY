# CSOPESY-MCO1-Process Scheduler and CLI

## Group Developers
* Campos, Don Oswin
* Lim, Ethan Yuric
* Gutierrez, Hanz
* Co, Stephen

### Project Structure

```text
my_console_project/
├── main.cpp                        # Primary entry point (contains the main() function)
├── README.md                       # Project documentation & setup instructions
└── functions.h                     # file that contains all of the functions for each of the sections of the Program
└── config.txt                      # file that contains the configurations of the program
└── csopesy-log.txt                 # file that contains the report of the utilization
```

## Prerequisites
* **C++ Compiler:** `g++` with C++17 support and `pthread` library.
* **Operating System:** Windows (Command Prompt / PowerShell).

## Configuration (`config.txt`)
Before running the program, ensure a `config.txt` file is placed in the root directory with the following parameters:

| Parameter | Description |
| :--- | :--- |
| **`num-cpu`** | Number of CPUs available. The range is $[1, 128]$. |
| **`scheduler`** | The scheduler algorithm: `"fcfs"` or `"rr"`. |
| **`quantum-cycles`** | The time slice given for each processor if a round-robin scheduler is used. Has no effect on other schedulers. The range is $[1, 2^{32}]$. |
| **`batch-process-freq`** | The frequency of generating processes in the `scheduler-start` command in CPU cycles. The range is $[1, 2^{32}]$. If one, a new process is generated at the end of each CPU cycle. |
| **`min-ins`** | The minimum instructions/command per process. The range is $[1, 2^{32}]$. |
| **`max-ins`** | The maximum instructions/command per process. The range is $[1, 2^{32}]$. |
| **`delays-per-exec`** | Delay before executing the next instruction in CPU cycles. The delay is a “busy-waiting” scheme wherein the process remains in the CPU. The range is $[0, 2^{32}]$. If zero, each instruction is executed per CPU cycle. |


## Build and Run Instructions

### Compilation Instructions
1. Open your terminal in the root directory where `main.cpp` and `functions.h` are located.
2. Run the compilation command:
   ```bash
   g++ -std=c++17 -pthread main.cpp -o main.exe
   ```

### Available Commands in the Program

| Command | Arguments / Input | Description |
| :--- | :--- | :--- |
| **`initialize`** (or `init`) | *None* | Loads parameters from `config.txt` and starts the scheduler background threads. |
| **`screen -s`** | `<process name>` | Creates a new process with the specified name, pushes it to the ready queue, and enters its virtual screen. |
| **`screen -r`** | `<process name>` | Resumes/enters the virtual screen of an existing process. |
| **`screen -ls`** | *None* | Displays overall CPU utilization, core usage statistics, and a full list of running and finished processes. |
| **`scheduler-start`** | *None* | Activates the background process generator loop based on the batch frequency. |
| **`scheduler-stop`** | *None* | Pauses the automatic process generation loop. |
| **`report-util`** | *None* | Exports the current utilization report to a file named `csopesy-log.txt`. |
| **`help`** | *None* | Displays the list of available commands. |
| **`exit`** | *None* | Safely stops background threads and terminates the program. |

*Note: While inside a process screen via `screen -s` or `screen -r`, the available sub-commands are `process-smi` to view internal details/logs and `exit` to return to the main menu.*
