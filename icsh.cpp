#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <fstream>
#include <sys/wait.h>
#include <csignal>
using namespace std;

const int MAX_ARGS = 256;
volatile pid_t currentchild = -1;
int exitCode = 0;

void executeEcho(const string& input) {
    cout << input.substr(5) << endl;
}

int execExit(const string& input) {
    int numexit = stoi(input.substr(5)) & 0xFF;
    return numexit;
}

void ignore_signals() {
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
}
void default_signals() {
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
}

void executeShell(istream& user_input, bool user_mode) {
    string input, last_input = "";
    ignore_signals(); 

    while (true) {
        if (user_mode) {
            cout << "X SHELL $ ";
            fflush(stdout);
        }
        if (!getline(user_input, input)) {
            if (user_input.eof()) {
                cout << "End of file reached or bye" << endl; 
            } else {
                cout << "Input error or bye" << endl; // For unexpected errors
            }
            break;
        }

        if (input.empty()) continue;

        if (input == "!!") {
            if (!last_input.empty()) {
                input = last_input;
                if (user_mode) {
                    cout << input << endl;
                }
                exitCode = 0;
            } else {
                cout << "No commands in history." << endl;
                exitCode = 0;
                continue;
            }
        } else {
            last_input = input;
        }

        if (input.substr(0, 2) == "##") continue; // I'm assuming this is a comment line, ignore it.

        if (input.substr(0, 5) == "echo ") {
            if (input == "echo $?") {
                cout << "The exit code is " << exitCode << endl;
                exitCode = 0;
            } else {
                executeEcho(input);
            }
        } else if (input.substr(0, 4) == "exit") {
            exitCode = execExit(input);
            cout << "Exiting with code: " << exitCode << endl;
            exit(exitCode);
        } else {
            char* args[MAX_ARGS];
            char* new_str = new char[input.length() + 1];
            strcpy(new_str, input.c_str());
            char* p = strtok(new_str, " ");
            int i = 0;
            while (p != NULL) {
                args[i++] = p;
                p = strtok(NULL, " ");
            }
            args[i] = NULL;

            currentchild = fork();
            if (currentchild == -1) {
                cerr << "Fork failed." << endl;
            } else if (currentchild == 0) {
                default_signals(); // Restore default signal behavior for the child process
                if (execvp(args[0], args) == -1) {
                    cerr << "bad command" << endl;
                    exit(EXIT_FAILURE);
                }
            } else {
               int status;
                waitpid(currentchild, &status, WUNTRACED);
                if (WIFEXITED(status) && WTERMSIG(status) == SIGINT) {
                    exitCode = 0; 
                    // cout << "The process was terminated" << exitCode << endl; 
                } else if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTSTP) { // Check if the process was stopped by SIGTSTP
                    exitCode = 146;
                    // cout << "The process has entered the background." << exitCode << endl; 
                }
                currentchild = -1;
            }
            delete[] new_str;
        }
    }
}

int main(int argc, char* argv[]) {
    cout << "Starting the XSHELL.." << endl;
    if (argc > 1) {
        ifstream fileStream(argv[1]);
        if (!fileStream) {
            cerr << "Cannot open file: " << argv[1] << endl;
            return 1;
        }
        executeShell(fileStream, false);
        fileStream.close();
    } else {
        executeShell(cin, true);
    }
    cout << "Bye, See you soon :X" << endl;
    return 0;
}
