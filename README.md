# CSOPESY---Custom-Operating-System

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
└── commands/                       # Folder containing command & animation logic
    └── marquee.cpp                 # Implementation of marquee thread loop (start, set_text, set_speed, start_marquee, stop_marquee)
```

## Performance & Optimal Settings

Text Speed value suggestions and observations on value changes:

| Range | Delay ($ms$) | Performance & Visual Behavior |
| :--- | :--- | :--- |
| **Optimal** | `100 ms - 160 ms` | Smooth scrolling motion with crisp text legibility. |
| **Too Fast** | `< 15 ms` | Motion blur makes text unreadable; causes terminal screen tearing. |
| **Too Slow** | `> 300 ms` | Choppy, stuttered text movement. |

## Prerequisites
* **C++ Compiler:** `g++` with C++11 (or higher) support.
* **Operating System:** Windows (Command Prompt / PowerShell).

## Instructions
### How to Build and Run

### Compilation Instructions

1. Open your terminal in the root directory where `main.cpp` is located.
2. Run the following command to compile `main.cpp` along with all implementation files inside the `commands/` folder:

```cmd
g++ main.cpp -o ConsoleApp.exe
```

### Commands in the Custom Marquee
| Command | Arguments / Input | Description |
| :--- | :--- | :--- |
| **`help`** | *None* | Displays all available commands and their descriptions. |
| **`start_marquee`** | *None* | Starts the marquee animation. |
| **`stop_marquee`** | *None* | Stops the marquee animation. |
| **`set_text`** | `<string>` | Accepts a text input and displays it as a marquee. |
| **`set_speed`** | `<int>` | Sets the marquee animation refresh delay in milliseconds. |
| **`exit`** | *None* | Terminates the console application. |
