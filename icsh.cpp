#include <iostream>
#include <string>
#include <fstream>
using namespace std;

void executeEcho(const string& input) {
    cout << input.substr(5) << endl; 
}

int execExit(const string& input) {
    int numexit = 0; 
    if (input.size() > 5) {
        numexit = stoi(input.substr(5)) & 0xFF;
    }

    return numexit;
}

void executeShell(istream& user_input, bool isInteractive, int& exitCode) {
    string input, last_input = "";
    exitCode = 0; 
    while (true) {
        if (isInteractive) {
            cout << "X SHELL $ ";
            if (!getline(cin, input)) {
                cout << "bye" << endl;
                break; 
            }
        } else {
            if (!getline(user_input, input)) {
                break;
            }
        }
        if (input.empty()) continue; 
        if (input == "!!") {
            if (!last_input.empty()) {
                input = last_input; 
                cout << input << endl; 
            } else {
                cout << "No commands in history." << endl;
                continue;
            }
        } else {
            last_input = input; 
        }
        if (input.substr(0, 2) == "##") continue; 
        if (input.substr(0, 4) == "echo") {
            executeEcho(input); 
        } else if (input.substr(0, 4) == "exit") {
            exitCode = execExit(input);
            if (exitCode == 0 || !isInteractive) {
                cout << "Bye, Have a nice day!" << "\n"; 
                break;
            } else {
                cout << "Exit code: " << exitCode << endl;
            }
        } else {
            cout << "bad command" << endl;
        }
    } 
}

int main(int argc, char* argv[]) {
    cout << "Starting the XSHELL.." << endl;
    int exitCode = 0;

    if (argc > 1) {
        ifstream fileStream(argv[1]);
        if (!fileStream) {
            cerr << "Cannot open this file!" << endl;
            return 1;  
        }
        executeShell(fileStream, false, exitCode);
        fileStream.close();
    } else {
            executeShell(cin, true, exitCode);
    cout << "bye" << endl;
    return exitCode;
    }
}

