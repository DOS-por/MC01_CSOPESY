// functions.h -- ALL structs, classes and functions live here (blank, with instructions).
// Header-only: free functions are marked `inline` so you only need two files.

#pragma once

#include <atomic>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
using namespace std;

// ============================================================
// 1. CONFIG
// ============================================================
struct Config {
    // fields: numCpu, scheduler, quantumCycles, batchProcessFreq,
    //         minIns, maxIns, delayPerExec
};

inline bool loadConfig(const string& path, Config& cfg) {
    // 1. Open config.txt. Return false if it can't be opened.
    // 2. Read "key value" pairs until EOF.
    // 3. Strip the quotes around the scheduler value ("rr" -> rr).
    // 4. Store each value in cfg based on the key.
    // 5. Validate ranges (num-cpu 1..128, min-ins <= max-ins, scheduler is fcfs/rr).
    // 6. Return true if everything is valid.
}

// ============================================================
// 2. INSTRUCTIONS
// ============================================================
enum class Op { PRINT, DECLARE, ADD, SUBTRACT, SLEEP, FOR };

struct Instruction {
    // fields: Op type, variable name(s), operand values, message,
    //         sleep ticks, repeat count, child instructions (for FOR)
};

inline void flattenInstruction(const Instruction& ins, vector<Instruction>& out) {
    // 1. If ins is not a FOR, push it to `out` and return.
    // 2. If it is a FOR, repeat `repeats` times:
    //      for each child in its body, call flattenInstruction(child, out).
    // (Nested FORs work automatically through recursion.)
}

// ============================================================
// 3. PROCESS
// ============================================================
enum class State { READY, RUNNING, SLEEPING, FINISHED };

struct Process {
    // fields: id, name, state, assigned core, creation timestamp,
    //         vector<Instruction> code, program counter (pc),
    //         map<string, uint16_t> variables, vector<string> logs,
    //         sleepUntil, quantumUsed, delayLeft
};

inline string getTimestamp() {
    // Return the current time formatted like "08/06/2024 09:15:22AM".
}

inline uint16_t clampUint16(long long value) {
    // Return value limited to the range [0, 65535].
}

inline void executeInstruction(Process& p, unsigned long long currentCycle) {
    // 1. Get the instruction at p.pc.
    // 2. switch on its type:
    //    PRINT    -> build the message (append a variable's value if used);
    //                push "(timestamp) Core:N "msg"" to p.logs.
    //    DECLARE  -> p.variables[var] = value.
    //    ADD      -> read both operands (an undeclared variable counts as 0),
    //                store clampUint16(a + b) in the target.
    //    SUBTRACT -> same as ADD but with a - b.
    //    SLEEP    -> set p.sleepUntil = currentCycle + ticks; state = SLEEPING.
    // 3. Increment p.pc.
    // 4. If p.pc reached the end of p.code: state = FINISHED, clear variables.
}

inline Process* generateProcess(int id, const string& name, const Config& cfg) {
    // 1. Pick a random target length between cfg.minIns and cfg.maxIns.
    // 2. Build random instructions until you have that many:
    //      - PRINT with "Hello world from <name>!"
    //      - DECLARE / ADD / SUBTRACT with random variables and values
    //      - SLEEP with a random uint8 tick count
    //      - FOR with a random body and repeat count (max nesting depth: 3)
    // 3. Flatten the FOR loops with flattenInstruction().
    // 4. Set the creation timestamp, state = READY, and return the process.
}

// ============================================================
// 4. SCHEDULER
// ============================================================
class Scheduler {
public:
    // members: Config, atomic cpuCycles, atomic running/generating flags,
    //          mutex, ready queue, vector of cores (one process pointer each,
    //          null = idle), list of all processes, next process id, OS thread

    void start(const Config& cfg) {
        // 1. Save cfg and size the cores vector to numCpu (all idle).
        // 2. Set running = true.
        // 3. Launch the OS thread running loop().
    }

    void stop() {
        // Set running = false and join the OS thread.
    }

    void setGenerating(bool on) {
        // Set the `generating` flag (used by scheduler-start / scheduler-stop).
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
    }

    void generateStep() {
        // If `generating` is true and cpuCycles % batchProcessFreq == 0:
        //   create a process named p01, p02, ... with generateProcess(),
        //   store it in the process list, push it to the ready queue.
    }

    void wakeSleepers() {
        // For each SLEEPING process: if cpuCycles >= sleepUntil,
        //   set it READY and push it to the ready queue.
    }

    void dispatchStep() {
        // For each idle core: if the ready queue isn't empty,
        //   pop the front process, set RUNNING, set its core id,
        //   reset quantumUsed and delayLeft, put it on that core.
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
    }

    Process* findProcess(const string& name) {
        // Search the process list by name; return nullptr if not found.
    }

    Process* createProcess(const string& name) {
        // Used by "screen -s". Under the lock:
        //   reject duplicate names (return nullptr),
        //   build with generateProcess(), store it, push to the ready queue,
        //   return the pointer.
    }

    string buildReport() {
        // Shared by "screen -ls" and "report-util". Under the lock, build a string with:
        //   - CPU utilization % (busy cores / total cores)
        //   - cores used and cores available
        //   - "Running processes:" each with name, timestamp, core, pc / total lines
        //   - "Finished processes:" each with name, timestamp, "Finished", total / total
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
    // Split the line on whitespace and return the tokens.
}

inline void printProcessInfo(Scheduler& s, Process* p) {
    // Used by "process-smi". Under the lock print:
    //   Process name, ID, Logs (every entry in p->logs).
    //   If FINISHED: print "Finished!".
    //   Otherwise: print "Current instruction line" and "Lines of code".
}

inline void processScreen(Scheduler& s, const string& name) {
    // 1. clearScreen().
    // 2. Loop: print the prompt, read a line.
    //      "process-smi" -> printProcessInfo()
    //      "exit"        -> clearScreen(), printBanner(), return
    //      anything else -> "Unknown command."
}

inline void handleScreenCommand(Scheduler& s, const vector<string>& tokens) {
    // "screen -s <name>": createProcess(); if nullptr print a duplicate-name
    //                     message, else processScreen().
    // "screen -r <name>": if findProcess() is null or FINISHED, print
    //                     "Process <name> not found."; else processScreen().
    // "screen -ls":       print s.buildReport().
    // Anything else:      print a usage message.
}

inline void handleReportUtil(Scheduler& s) {
    // Write s.buildReport() to csopesy-log.txt and print
    // "Report generated at csopesy-log.txt!".
}

inline bool handleInitialize(Scheduler& s) {
    // loadConfig("config.txt"), then s.start(cfg).
    // Print an error and return false if loading fails; otherwise return true.
}
