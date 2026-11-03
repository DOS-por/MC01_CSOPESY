// main.cpp -- the main program only. It calls the functions in functions.h.
// Build: g++ -std=c++17 -pthread main_skeleton.cpp -o csopesy

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <windows.h>
#include "functions.h"
using namespace std;

int main() {
    // Windows command to change utf encoding and change console color
    SetConsoleOutputCP(65001);
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    Scheduler scheduler;
    bool initialized = false;

    printBanner();

    while (true) {
        cout << "\nroot:\\> ";
        string line;
        if (!getline(cin, line)) break;

        vector<string> tokens = tokenize(line);
        if (tokens.empty()) continue;
        const string& cmd = tokens[0];

        // "exit" is always allowed
        if (cmd == "exit") break;

        // Before initialize, only "initialize" and "exit" are recognized
        if (!initialized) {
            if (cmd == "initialize") initialized = handleInitialize(scheduler);
            else cout << "Please run \"initialize\" first.\n";
            continue;
        }

        if      (cmd == "screen")          handleScreenCommand(scheduler, tokens);
        else if (cmd == "scheduler-start") scheduler.setGenerating(true);
        else if (cmd == "scheduler-stop")  scheduler.setGenerating(false);
        else if (cmd == "report-util")     handleReportUtil(scheduler);
        else if (cmd == "initialize")      cout << "Already initialized.\n";
        else                               cout << "Unknown command.\n";
    }

    scheduler.stop();
    return 0;
}
