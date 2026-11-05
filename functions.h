// functions.h -- ALL structs, classes and functions locations

#pragma once

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>
using namespace std;


#undef max
// ============================================================
// 1. CONFIG
// ============================================================
struct Config {
    unsigned long long numCpu = 0;
    string scheduler;
    unsigned long long quantumCycles = 0;
    unsigned long long batchProcessFreq = 0;
    unsigned long long minIns = 0;
    unsigned long long maxIns = 0;
    unsigned long long delayPerExec = 0;
};

inline bool loadConfig(const string& path, Config& cfg) {
    ifstream in(path);
    if (!in) {
        cout << "Error: cannot open " << path << ".\n";
        return false;
    }

    const unsigned long long MAX32 = 4294967296ULL; // value for 2^32
    Config c;
    bool hasCpu = false, hasSched = false, hasQuantum = false, hasFreq = false;
    bool hasMin = false, hasMax = false, hasDelay = false;

    string key, value;
    while (in >> key >> value) {
        if (key == "scheduler") {
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
                value = value.substr(1, value.size() - 2);
            c.scheduler = value;
            hasSched = true;
            continue;
        }

        // numeric values must be plain non-negative integers.
        if (value.empty() || value.find_first_not_of("0123456789") != string::npos || value.size() > 12) {
            cout << "Error: invalid value \"" << value << "\" for " << key << ".\n";
            return false;
        }
        unsigned long long n = stoull(value);

        if (key == "num-cpu") { c.numCpu = n; hasCpu = true; }
        else if (key == "quantum-cycles") { c.quantumCycles = n; hasQuantum = true; }
        else if (key == "batch-process-freq") { c.batchProcessFreq = n; hasFreq = true; }
        else if (key == "min-ins") { c.minIns = n; hasMin = true; }
        else if (key == "max-ins") { c.maxIns = n; hasMax = true; }
        else if (key == "delays-per-exec") { c.delayPerExec = n; hasDelay = true; }
        else {
            cout << "Error: unknown config key \"" << key << "\".\n";
            return false;
        }
    }

    if (!(hasCpu && hasSched && hasQuantum && hasFreq && hasMin && hasMax && hasDelay)) {
        cout << "Error: config file is missing one or more parameters.\n";
        return false;
    }
    if (c.numCpu < 1 || c.numCpu > 128) {
        cout << "Error: num-cpu must be in [1, 128].\n";
        return false;
    }
    if (c.scheduler != "fcfs" && c.scheduler != "rr") {
        cout << "Error: scheduler must be \"fcfs\" or \"rr\".\n";
        return false;
    }
    if (c.quantumCycles < 1 || c.quantumCycles > MAX32 ||
        c.batchProcessFreq < 1 || c.batchProcessFreq > MAX32 ||
        c.minIns < 1 || c.minIns > MAX32 ||
        c.maxIns < 1 || c.maxIns > MAX32 ||
        c.delayPerExec > MAX32) {
        cout << "Error: a config value is out of range.\n";
        return false;
    }
    if (c.minIns > c.maxIns) {
        cout << "Error: min-ins must not exceed max-ins.\n";
        return false;
    }

    cfg = c;
    return true;
}

// ============================================================
// 2. INSTRUCTIONS
// ============================================================
enum class Op { PRINT, DECLARE, ADD, SUBTRACT, SLEEP, FOR };

struct Instruction {
    Op type;

    //Fields that are needed for each of the operation type
    string message; // used to store the message for PRINT operation
    string printVariable; // optional variable appended to a PRINT message
    string op1; // First operand for ADD or SUBTRACT
    string op2; // Second operand for ADD or SUBTRACT
    string result_var; //variable used to store the result for DECLARE, SUBTRACT, and ADD
    uint16_t var = 0; //used for DECLARE operation with default value of 0
    uint8_t sleepTicks = 0; // used for SLEEP operation with default value of 0


    //Needed variables for the FOR operations
    unsigned long long repeats = 0; // value that decides how may repetitions will be done
    vector<Instruction> body; // The instruction that will be repeated
};

inline void flattenInstruction(const Instruction& ins, vector<Instruction>& out) {

    // if the operation/instruction does not need repetion push it to out and return
    if(ins.type != Op::FOR){
        out.push_back(ins);
        return;
    }

    // This is specifically used for the FOR operation
    for(unsigned long long i = 0; i < ins.repeats; i++){
        // for loop used to get the body of the FOR operation command
        for(const auto& child : ins.body) {
            flattenInstruction(child, out);
        }
    }
}

// ============================================================
// 3. PROCESS
// ============================================================
enum class State { READY, RUNNING, SLEEPING, FINISHED };

struct Process {

    int id;  // Process ID
    int assignedCore = -1;  // Core assigned to the process
    State state;  // Current state of the process
    string name;  // Name of the process
    string creationTimestamp;  // Timestamp when the process was created
    unsigned long long pc = 0;  // Program counter
    unsigned long long sleepUntil = 0;  // Cycle until which the process sleeps
    unsigned long long quantumUsed = 0;  // Quantum used by the process
    unsigned long long delayLeft = 0;  // Remaining delay for the process
    map<string, uint16_t> variables;  // Variables of the process
    vector<string> logs;  // Logs generated by the process
    vector<Instruction> code;  // Instructions/code of the process
};

inline string getTimestamp() {
    // Returns the current time formatted like "08/06/2024 09:15:22AM".
    SYSTEMTIME st;
    GetLocalTime(&st);
    const unsigned short hour = st.wHour % 12 == 0 ? 12 : st.wHour % 12;
    const char* meridiem = st.wHour < 12 ? "AM" : "PM";
    char buffer[32];
    sprintf(buffer, "%02d/%02d/%04d %02d:%02d:%02d%s", st.wMonth, st.wDay,
        st.wYear, hour, st.wMinute, st.wSecond, meridiem);
    return string(buffer);
}

inline uint16_t clampUint16(long long value) {
    // Returns value limited to the range [0, 65535].
    if (value < 0) return 0;
    if (value > 65535) return 65535;
    return static_cast<uint16_t>(value);
}

inline void executeInstruction(Process& p, unsigned long long currentCycle) {
    
    // Check if program counter has exceeded code size, indicating process has finished.
    if (p.pc >= p.code.size()) {
        p.state = State::FINISHED;
        p.variables.clear();
        return;
    }

    // Fetches the current instruction to execute.
    const Instruction& instruction = p.code[static_cast<size_t>(p.pc)];
    auto readOperand = [&p](const string& operand) -> uint16_t {
        if (!operand.empty() &&
            operand.find_first_not_of("0123456789") == string::npos) {
            unsigned int value = 0;
            for (char digit : operand) {
                value = value * 10 + static_cast<unsigned int>(digit - '0');
                if (value > 65535) return 65535;
            }
            return static_cast<uint16_t>(value);
        }   
        return p.variables[operand];
    };

    // Executes the instruction based on its type.
    switch (instruction.type) {
    case Op::PRINT: { // Handles PRINT operation
        string message = instruction.message;
        const string& variableName = instruction.printVariable.empty()
            ? instruction.op1
            : instruction.printVariable;
        if (!variableName.empty()) {
            auto variable = p.variables.find(variableName);
            message += variable == p.variables.end()
                ? "0"
                : to_string(variable->second);
        }
        p.logs.push_back("(" + getTimestamp() + ") Core:" +
            to_string(p.assignedCore + 1) + " \"" + message + "\"");
        break;
    }
    case Op::DECLARE: // Handles DECLARE operation
        p.variables[instruction.result_var] = instruction.var;
        break;
    case Op::ADD: { // Handles ADD operation
        const long long lhs = readOperand(instruction.op1);
        const long long rhs = readOperand(instruction.op2);
        p.variables[instruction.result_var] = clampUint16(lhs + rhs);
        break;
    }
    case Op::SUBTRACT: { // Handles SUBTRACT operation
        const long long lhs = readOperand(instruction.op1);
        const long long rhs = readOperand(instruction.op2);
        p.variables[instruction.result_var] = clampUint16(lhs - rhs);
        break;
    }
    case Op::SLEEP: // Handles SLEEP operation
        p.sleepUntil = currentCycle >
            numeric_limits<unsigned long long>::max() - instruction.sleepTicks
            ? numeric_limits<unsigned long long>::max()
            : currentCycle + instruction.sleepTicks;
        p.state = State::SLEEPING;
        break;
    case Op::FOR: // Handles FOR operation - this is already handled during instruction generation
        break; // No action needed for FOR during execution (supposedly)
    }

    ++p.pc;
    if (p.pc >= p.code.size()) {
        p.state = State::FINISHED;
        p.variables.clear();
    }
}

inline Process* generateProcess(int id, const string& name, const Config& cfg) {
    static thread_local mt19937_64 randomEngine(random_device{}());
    uniform_int_distribution<unsigned long long> lengthDistribution(
        cfg.minIns, cfg.maxIns);
    uniform_int_distribution<unsigned int> smallValueDistribution(0, 65535);
    uniform_int_distribution<unsigned int> variableDistribution(0, 2);
    uniform_int_distribution<unsigned int> sleepDistribution(1, 255);
    uniform_int_distribution<unsigned int> choiceDistribution(0, 5);
    uniform_int_distribution<unsigned int> bodySizeDistribution(1, 2);
    uniform_int_distribution<unsigned int> repeatDistribution(1, 3);
    const vector<string> variableNames = {"x", "y", "z"};
    const unsigned long long targetLength = lengthDistribution(randomEngine);

    auto randomVariable = [&]() -> const string& {
        return variableNames[variableDistribution(randomEngine)];
    };
    auto randomOperand = [&]() -> string {
        if (smallValueDistribution(randomEngine) % 2 == 0)
            return randomVariable();
        return to_string(smallValueDistribution(randomEngine));
    };

    function<Instruction(unsigned int)> makeInstruction;
    makeInstruction = [&](unsigned int depth) -> Instruction {
        Instruction instruction{};
        unsigned int choice = choiceDistribution(randomEngine);
        if (choice == 5 && depth < 3) {
            instruction.type = Op::FOR;
            instruction.repeats = repeatDistribution(randomEngine);
            const unsigned int bodySize = bodySizeDistribution(randomEngine);
            for (unsigned int i = 0; i < bodySize; ++i)
                instruction.body.push_back(makeInstruction(depth + 1));
            return instruction;
        }

        switch (choice % 5) {
        case 0:
            instruction.type = Op::PRINT;
            instruction.message = "Hello world from " + name + "!";
            break;
        case 1:
            instruction.type = Op::DECLARE;
            instruction.result_var = randomVariable();
            instruction.var = static_cast<uint16_t>(
                smallValueDistribution(randomEngine));
            break;
        case 2:
            instruction.type = Op::ADD;
            instruction.result_var = randomVariable();
            instruction.op1 = randomOperand();
            instruction.op2 = randomOperand();
            break;
        case 3:
            instruction.type = Op::SUBTRACT;
            instruction.result_var = randomVariable();
            instruction.op1 = randomOperand();
            instruction.op2 = randomOperand();
            break;
        case 4:
            instruction.type = Op::SLEEP;
            instruction.sleepTicks = static_cast<uint8_t>(
                sleepDistribution(randomEngine));
            break;
        }
        return instruction;
    };

    auto process = new Process{};
    process->id = id;
    process->name = name;
    process->state = State::READY;
    process->creationTimestamp = getTimestamp();

    while (process->code.size() < targetLength) {
        Instruction instruction = makeInstruction(0);
        vector<Instruction> flattened;
        flattenInstruction(instruction, flattened);
        const size_t remaining = static_cast<size_t>(
            targetLength - process->code.size());
        if (flattened.size() <= remaining) {
            process->code.insert(process->code.end(),
                flattened.begin(), flattened.end());
        } else {
            Instruction fallback = makeInstruction(3);
            vector<Instruction> singleInstruction;
            flattenInstruction(fallback, singleInstruction);
            process->code.push_back(singleInstruction.front());
        }
    }

    return process;
}

// ============================================================
// 4. SCHEDULER
// ============================================================
class Scheduler {
public:
    // members: Config, atomic cpuCycles, atomic running/generating flags,
    //          mutex, ready queue, vector of cores (one process pointer each,
    //          null = idle), list of all processes, next process id, OS thread
    Config config;
    atomic<unsigned long long> cpuCycles{0};
    atomic<bool> running{false};
    atomic<bool> generating{false};
    thread osThread;
    
    mutex mtx;
    deque<Process*> readyQueue;
    vector<Process*> cores;
    vector<Process*> allProcesses;
    unsigned long long nextProcessId = 1;

    ~Scheduler() { 
        stop(); 
        for (Process* p : allProcesses) {
            delete p;
        }
    }

    void start(const Config& cfg) {
        stop();
        config = cfg;
        cpuCycles = 0;
        cores.assign(config.numCpu, nullptr);
        running = true;
        osThread = thread(&Scheduler::loop, this);
    }

    void stop() {
        running = false;
        if (osThread.joinable()) osThread.join();
    }

    void setGenerating(bool on) {
        generating = on;
    }

    void loop() {
        // While running:
        //   lock the mutex, then each CPU cycle:
        //     1. generateStep()
        //     2. wakeSleepers()
        //     3. dispatchStep()
        //     4. executeStep()
        //     5. cpuCycles++
        //   unlock, then sleep briefly so the loop doesn't spin at 100% CPU.
        while (running) {
            {
                lock_guard<mutex> lock(mtx);
                generateStep();
                wakeSleepers();
                dispatchStep();
                executeStep();
                cpuCycles++;
            }
            this_thread::sleep_for(chrono::milliseconds(1));
        }
    }

    void generateStep() {
        // If `generating` is true and cpuCycles % batchProcessFreq == 0:
        //   create a process named p01, p02, ... with generateProcess(),
        //   store it in the process list, push it to the ready queue.
        if (generating && (cpuCycles % config.batchProcessFreq == 0)) {
            string name = "p" + to_string(nextProcessId);
            Process* p = generateProcess(nextProcessId, name, config);
            nextProcessId++;
            
            allProcesses.push_back(p);
            readyQueue.push_back(p);
        }
    }

    void wakeSleepers() {
        // For each SLEEPING process: if cpuCycles >= sleepUntil,
        //   set it READY and push it to the ready queue.
        for (Process* p : allProcesses) {
            if (p->state == State::SLEEPING && cpuCycles >= p->sleepUntil) {
                p->state = State::READY;
                readyQueue.push_back(p);
            }
        }
    }

    void dispatchStep() {
        // For each idle core: if the ready queue isn't empty,
        //   pop the front process, set RUNNING, set its core id,
        //   reset quantumUsed and delayLeft, put it on that core.
        for (size_t i = 0; i < cores.size(); ++i) {
            if (cores[i] == nullptr && !readyQueue.empty()) {
                Process* p = readyQueue.front();
                readyQueue.pop_front();
                
                p->state = State::RUNNING;
                p->assignedCore = i;
                p->quantumUsed = 0;
                p->delayLeft = config.delayPerExec;
                cores[i] = p;
            }
        }
    }

    void executeStep() {
        // For each busy core:
        //   1. If delayLeft > 0: decrement it (busy-wait), count a quantum tick, move on.
        //   2. Otherwise executeInstruction(), reset delayLeft = delayPerExec,
        //      count a quantum tick.
        //   3. If the process is FINISHED or SLEEPING: free the core.
        //   4. Else if scheduler is "rr" and quantumUsed >= quantumCycles:
        //      set READY, push to the back of the ready queue, free the core.
        //   5. "fcfs" never preempts.
        for (size_t i = 0; i < cores.size(); ++i) {
            Process* p = cores[i];
            if (p == nullptr) continue;

            if (p->delayLeft > 0) {
                p->delayLeft--;
                p->quantumUsed++;
            } else {
                executeInstruction(*p, cpuCycles);
                p->delayLeft = config.delayPerExec;
                p->quantumUsed++;
            }

            if (p->state == State::FINISHED || p->state == State::SLEEPING) {
                cores[i] = nullptr;
            } 
            else if (config.scheduler == "rr" && p->quantumUsed >= config.quantumCycles) {
                p->state = State::READY;
                readyQueue.push_back(p);
                cores[i] = nullptr;
            }
        }
    }

    Process* findProcess(const string& name) {
        // Search the process list by name; return nullptr if not found.
        lock_guard<mutex> lock(mtx);
        for (Process* p : allProcesses) {
            if (p->name == name) return p;
        }
        return nullptr;
    }

    Process* createProcess(const string& name) {
        // Used by "screen -s". Under the lock:
        //   reject duplicate names (return nullptr),
        //   build with generateProcess(), store it, push to the ready queue,
        //   return the pointer.
        lock_guard<mutex> lock(mtx);
        for (Process* p : allProcesses) {
            if (p->name == name) return nullptr;
        }
        
        Process* p = generateProcess(nextProcessId, name, config);
        nextProcessId++;
        
        allProcesses.push_back(p);
        readyQueue.push_back(p);
        return p;
    }

    string buildReport() {
        // Shared by "screen -ls" and "report-util". Under the lock, build a string with:
        //   - CPU utilization % (busy cores / total cores)
        //   - cores used and cores available
        //   - "Running processes:" each with name, timestamp, core, pc / total lines
        //   - "Finished processes:" each with name, timestamp, "Finished", total / total
        lock_guard<mutex> lock(mtx);
        int coresUsed = 0;
        for (Process* core : cores) {
            if (core != nullptr) coresUsed++;
        }
        
        int util = (config.numCpu == 0) ? 0 : (coresUsed * 100) / config.numCpu;
        
        stringstream ss;
        ss << "CPU utilization: " << util << "%\n";
        ss << "Cores used: " << coresUsed << "\n";
        ss << "Cores available: " << (config.numCpu - coresUsed) << "\n\n";
        
        ss << "----------------------------------------------\n";
        ss << "Running processes:\n";
        for (Process* p : allProcesses) {
            if (p->state != State::FINISHED) {
                string coreStr = (p->assignedCore != -1) ? to_string(p->assignedCore) : "N/A";
                ss << p->name << "\t(" << p->creationTimestamp << ")\tCore: " << coreStr 
                   << "\t" << p->pc << " / " << p->code.size() << "\n";
            }
        }
        
        ss << "\nFinished processes:\n";
        for (Process* p : allProcesses) {
            if (p->state == State::FINISHED) {
                ss << p->name << "\t(" << p->creationTimestamp << ")\tFinished\t" 
                   << p->code.size() << " / " << p->code.size() << "\n";
            }
        }
        ss << "----------------------------------------------\n";
        return ss.str();
    }
};

// ============================================================
// 5. CLI HELPERS
// ============================================================
inline void printBanner() {
    cout << "    ▄▄▄▄▄▄▄▄▄▄   ▄▄▄▄▄▄▄▄▄▄   ▄▄▄▄▄▄▄▄▄   ▄▄▄▄▄▄▄▄▄▄    ▄▄▄▄▄▄▄▄▄▄   ▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄   ▄▄▄▄ \n";
    cout << "    ███▓┌─ ███▓│ ▄██▓┌─ ███▓│ ▄▓█▓┌─ ███▓┐ ███▓┌─ ███▓┐ ███▓┌─ ███▓│ ▄██▓┌─ ███▓│ ███▓│  ███▓│\n";
    cout << "    ███▓│░  ───┘ ▀███▄▄▄▄▄ ─┘ ███▓│  ████│ ████▄▄▄██▀┌┘ ████▄▄  ───┘ ▀███▄▄▄▄▄ ─┘ ▀███▄▄▄████│\n";
    cout << "    ███▓│▒ ▄▄▄▄  ▄▄▄▄┌─ ███▓│ ███▓│  ███▓│ ███▓┌────┘ ▒ ███▓┌─┘▄▄▄▄  ▄▄▄▄┌─ ███▓│ ▄▄▄▄┌─ ███▓│\n";
    cout << "    ▀███▄▄▄███▓│ ▓███▄▄▄██▓┌┘ └▓██▄▄▄█▓┌─┘ ▓███│▀▀▀▀▀▀▀ ▀███▄▄▄███▓│ ▓███▄▄▄██▓┌┘ ▓███▄▄▄██▓┌┘\n";
    cout << "    ────────┘  ─────────┘    ───────┘    ───┘         ────────┘  ─────────┘   ─────────┘ \n";
    cout << "====================================\n";
    cout << "Welcome to CSOPESY Emulator!\n";
    cout << "Developers: \n";
    cout << "Campos, Don Oswin\n";
    cout << "Lim, Ethan Yuric\n";
    cout << "Co, Stephen\n";
    cout << "Gutierrez, Hanz\n";
    cout << "Last updated: 2026-11-04\n";
    cout << "====================================\n";
}

inline void clearScreen() {
    cout << "\033[2J\033[H"; // ANSI escape code to clear the terminal and move the cursor to the top-left corner
}

inline vector<string> tokenize(const string& line) {
    vector<string> tokens;
    istringstream iss(line);
    string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

inline void printProcessInfo(Scheduler& s, Process* p) {
    if(!p) return;

    // printing of processes
    cout << "----------------------------------------------\n";
    cout << "Process Name: " << p->name << "\n";
    cout << "ID: " << p->id << "\n";
    cout << "Logs: " << "\n";
    for(const auto& log: p->logs) {
        cout << log << "\n";
    }
    cout << "\n";
    cout << "\n";

    // for printing the current instruction line and lines of code and if process is finished
    if(p->state == State::FINISHED) {
        cout << "Finished!!\n";
    }
    else {
        cout << "Current Instruction line: " << p->pc << "\n";
        cout << "Lines of code: " << p->code.size() << "\n";
    }
    cout << "----------------------------------------------\n";
}

inline void processScreen(Scheduler& s, const string& name) {
    string line;
    while(true) {
        cout << "\nroot:\\> ";
        if (!getline(cin, line)) break;

        auto tokens = tokenize(line);
        
        if(tokens.empty()) {
            continue;
        }

        if(tokens[0] == "process-smi") {
            Process* p = s.findProcess(name);
            if(p) {
                printProcessInfo(s, p);
            }
            else
            {
                cout << "Error: Process not found\n";
            }
        }
        else if(tokens[0] == "exit") {
            clearScreen();
            printBanner();
            return;
        }
        else {
            cout << "Unknown command. Please type valid commands\n";
        }
    }
}

inline void handleScreenCommand(Scheduler& s, const vector<string>& tokens) {
    if (tokens.size() < 2) {
        cout << "Usage: screen -s <name> | screen -r <name> | screen -ls\n";
        return;
    }

    if (tokens[1] == "-s" && tokens.size() >= 3) {
        string name = tokens[2];
        Process* p = s.createProcess(name);
        if (p == nullptr) {
            cout << "Error: Process with name \"" << name << "\" already exists.\n";
        } else {
            processScreen(s, name);
        }
    } 
    else if (tokens[1] == "-r" && tokens.size() >= 3) {
        string name = tokens[2];
        Process* p = s.findProcess(name);
        if (p == nullptr || p->state == State::FINISHED) {
            cout << "Process \"" << name << "\" not found or already finished.\n";
        } else {
            processScreen(s, name);
        }
    } 
    else if (tokens[1] == "-ls") {
        cout << s.buildReport();
    } 
    else {
        cout << "Invalid screen command usage.\n";
    }
}

inline void handleReportUtil(Scheduler& s) {
    ofstream logFile("csopesy-log.txt");
    if (logFile.is_open()) {
        logFile << s.buildReport();
        logFile.close();
        cout << "Report generated at csopesy-log.txt!\n";
    } else {
        cout << "Error: File not Found\n";
    }
}

inline bool handleInitialize(Scheduler& s) {
    Config cfg;
    if (!loadConfig("config.txt", cfg)) return false;
    s.start(cfg);
    cout << "Configuration loaded from config.txt.\n";
    cout << "Initialized with " << cfg.numCpu << " CPU(s)\n";
    cout << "Scheduler: " << cfg.scheduler << "\n";
    cout << "Quantum Cycles: " << cfg.quantumCycles << "\n";
    cout << "Batch Process Frequency: " << cfg.batchProcessFreq << "\n";
    cout << "Min Instructions: " << cfg.minIns << "\n";
    cout << "Max Instructions: " << cfg.maxIns << "\n";
    cout << "Delays per Execution: " << cfg.delayPerExec << "\n";
    return true;
}

inline void handleHelp() {
    cout << "Available commands:\n";
    cout << "  initialize       Initialize the scheduler with config.txt\n";
    cout << "  screen -s <name> Create and enter a new process screen\n";
    cout << "  screen -r <name> Resume an existing process screen\n";
    cout << "  screen -ls       List all processes\n";
    cout << "  scheduler-start  Start the scheduler\n";
    cout << "  scheduler-stop   Stop the scheduler\n";
    cout << "  report-util      Generate a utilization report\n";
    cout << "  help             Show this help message\n";
    cout << "  exit             Exit the program\n";
}
