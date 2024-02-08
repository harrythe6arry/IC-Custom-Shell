#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
using namespace std;


const int MAX_ARGS = 1024;
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



void executeShell(istream& user_input, bool user_mode, int& exitCode) {
    string input, last_input = "";
    exitCode = 0; 
    while (true) {
        if (user_mode) {
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
            if (exitCode == 0 || !user_mode) {
                cout << "Bye, Have a nice day!" << "\n"; 
                break;
            } else {
                cout << "Exit code: " << exitCode << endl;
            }
            
        } else {
            char *new_str = new char[input.length() + 1]; // create a new char array
            strncpy(new_str, input.c_str(), input.length() + 1); // copy the string to the char array
            char *p = strtok(new_str, " "); // split the string by space
            char *args[MAX_ARGS];
            int i = 0;
            while (p != 0) { // loop through the string
                args[i++] = p;
                p = strtok(NULL, " "); // split the string by space
            }
            args[i] = NULL;
            pid_t pid = fork();
            if (pid == -1) {
                cerr << "Fork is not working.." << endl;
                execExit("exit 1");
            } else if (pid == 0) {
                execvp(args[0], args);
                cout << "bad command" << "\n"; 
                execExit("exit 1");
            } else {
                int status;
                waitpid(pid, &status, 0);   
            }
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
    }  
    else {
    executeShell(cin, true, exitCode);
    cout << "bye" << endl;
    return exitCode;
    }

    return 0;
}

