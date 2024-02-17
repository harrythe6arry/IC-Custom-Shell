#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <fstream>
#include <sys/wait.h>
#include <csignal>
#include <cstdlib>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>

using namespace std;

const int MAX_ARGS = 256;
volatile pid_t currentchild = -1;
int exitCode = 0;
const int MAX_BG = 256; 
int bg_count = 0;
pid_t bg_processes[MAX_BG];
bool externalecho = false; // special condition for echo 
string bg_commands[MAX_BG];
volatile bool job_track[MAX_BG] = {false};

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



// implmement the I/O redirection 

void handleIO(char* args[], bool &input_redir, bool&output_redir) {
    int in;
    int out; 

    if (!externalecho) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            input_redir = true;
            args[i] = NULL;
            in = open(args[i + 1], O_RDONLY);
            if (in < 0) {
                cerr << "Error: " << strerror(errno) << endl;
                exit(errno);
            }
            dup2(in, STDIN_FILENO);
            close(in);
            args[i] = NULL;
            i++; 
        } else if (strcmp(args[i], ">") == 0) {
            output_redir = true;
            args[i] = NULL;
            out = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (out < 0) {
                cerr << "Error: " << strerror(errno) << endl;
                exit(errno);
            }
            dup2(out, STDOUT_FILENO);
            close(out);
            args[i] = NULL;
            i++;
        }
    }
    }
}

void completejobs(pid_t pid, int status) {
  for (int i = 0; i < bg_count; i++) {
    if (bg_processes[i] == pid) {
      cout << "\n[" << i + 1 << "]  Done                 " << bg_commands[i] << endl;
      bg_processes[i] = 0;
      bg_commands[i] = "";
      cout << "X SHELL $ ";
      fflush(stdout);
    }
  }
}
void sigchild(int signum) {
  int saved_value = errno; 
  pid_t pid;
  int status;
  while ((pid = waitpid((pid_t)(-1), &status, WNOHANG)) > 0) {
    completejobs(pid, status);
  }

  errno = saved_value; 
}

void listjobs() {
  for (int i = 0; i < bg_count; i++) {
    if (bg_processes[i] == 0) { // no jobs are running
      cout << "No jobs are running" << endl;
    }
    else { // jobs are running
      cout << "[" << i + 1 << "] Running                  " << " " << bg_commands[i] << "&" <<endl;
    }
  }
}

void foreground_job(int job_id) {
  if (job_id < 0 || job_id >= bg_count) {
    cerr << "Invalid job ID" << endl;
    return;
  }
  pid_t pid = bg_processes[job_id];
  if (pid == 0) {
    cerr << "No such job" << endl;
    return;
  }
  int status;
  cout << "Foreground process " << pid << " started" << endl;
  cout << bg_commands[job_id] << endl;
  kill(pid, SIGCONT);
  waitpid(pid, &status, WUNTRACED);
  cout << "Foreground process " << pid << " exited with status " << status << endl;
  if (WIFEXITED(status)) {
    completejobs(pid, status);
  }
}

// void backgroundjobs(int job_id) {
//   if (job_id < 0 || job_id >= bg_count) {
//     cerr << "Invalid job ID" << endl;
//     return;
//   }
//   pid_t pid = bg_processes[job_id];
//   if (pid == 0) {
//     cerr << "No such job" << endl;
//     return;
//   }
//   int status;
//   cout << "Background process " << pid << " started" << endl;
//   cout << bg_commands[job_id] << endl;
//   kill(pid, SIGCONT);
//   job_track[job_id] = true;
// }


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

    
    if (input.rfind("fg ", 0) == 0) {
    string job_id_str = input.substr(input.find('%') + 1);
    int job_id = -1;
    try {
        job_id = stoi(job_id_str) - 1; // Convert job ID from string to int, adjust for 0-based indexing
        foreground_job(job_id);
    } catch (const invalid_argument& e) {
        cerr << "Invalid job ID format" << endl;
    } catch (const out_of_range& e) {
        cerr << "Job ID out of range" << endl;
    }
    continue; // Skip to the next iteration of the loop
}


    bool have_bg = false;
    if (input.back() == '&') {
      have_bg = true;
      input.pop_back();
    }
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
        if (input.substr(0, 2) == "##") continue; // comment line ignore
        if (input.substr(0, 4) == "jobs") {
          listjobs();
          continue;
        }
        if (externalecho) {
          if (input.substr(0, 5) == "echo ") {
              if (input == "echo $?") {
                cout << "The exit code is " << exitCode << endl;
                exitCode = 0;
              }
              else {
                executeEcho(input);
              }
          }
        }

        // if (input.rfind("bg ", 0) == 0) {
        //   string job_id_str = input.substr(input.find('%') + 1);
        //   int job_id = -1;
        //   try {
        //       job_id = stoi(job_id_str) - 1; // Convert job ID from string to int, adjust for 0-based indexing
        //       backgroundjobs(job_id);
        //   } catch (const invalid_argument& e) {
        //       cerr << "Invalid job ID format" << endl;
        //   } catch (const out_of_range& e) {
        //       cerr << "Job ID out of range" << endl;
        //   }
        //   continue; // Skip to the next iteration of the loop
        // }
         else if (input.substr(0, 4) == "exit") {
          exitCode = execExit(input);
          cout << "Exiting with code: " << exitCode << endl;
          cout << "Bye, See you soon :X" << endl;
          exit(exitCode);
        } else {
            char* args[MAX_ARGS]; // Array of pointers to the arguments
            char* new_str = new char[input.length() + 1]; // Create a new string to store the input
            // cout << "the new str is " << new_str << endl;
            strcpy(new_str, input.c_str()); // Copy the input to the new string
            char* p = strtok(new_str, " "); // Tokenize the input string
            // cout << "the p is " << p << endl;
            int i = 0; // Index for the args array
            while (p != NULL) { // Loop through the tokens and add them to the args array
                args[i++] = p;
                p = strtok(NULL, " "); // Get the next token
            }
          
            args[i] = NULL; // Set the last element of the args array to NULL
            bool input_redir = false; // Set to true if input redirection is used
            bool output_redir = false; // Set to true if output redirection is used


            currentchild = fork();// Create a new process
            if (currentchild == -1) {// Check if the fork failed
                cerr << "Fork failed." << endl;
            } else if (currentchild == 0) {
                default_signals(); // Restore default signal behavior for the child process
                // cout << "Getting pid when current child = 0 " << getpid() << endl;
                // cout << "current child is when current == 0 " << currentchild << endl;
                handleIO(args, input_redir, output_redir);

              if (execvp(args[0], args) == -1) {
                  cerr << "bad command" << endl;
                  exit(EXIT_FAILURE);
              }

            } else {
                if (!have_bg) {
               int status;
                // cout << "Getting pid " << getpid() << endl;
                // cout << "current child is " << currentchild << endl;
                waitpid(currentchild, &status, WUNTRACED);
                // cout << "Getting pid" << getpid() << endl;
                if (WIFEXITED(status) && WTERMSIG(status) == SIGINT) {
                    exitCode = 0; 
                    // cout << "The process was terminated" << exitCode << endl; 
                } else if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTSTP) { // Check if the process was stopped by SIGTSTP
                    exitCode = 146;                    
                }
                }
                else {
                  if (have_bg) {
                  // signal(SIGCHLD, sigchild);
                  bg_processes[bg_count] = currentchild;
                  bg_commands[bg_count] = input;
                  cout << "[" << bg_count + 1 << "] " << currentchild << endl;
                  bg_count++;
                }
                }
                // cout << " after the process the current child is " << currentchild << endl;
                currentchild = -1;
            }
            delete[] new_str;
        }
    }
  }

  
int main(int argc, char* argv[]) {

    signal(SIGCHLD, sigchild);
    for (int i = 0; i < MAX_BG; i++) {
        bg_commands[i] = "";
    }
    cout << "Starting the XSHELL.." << endl;

    if (argc > 1) {
        ifstream fileStream(argv[1]);
        if (!fileStream) {
            cerr << "Cannot open file: " << argv[1] << endl;
            return 1;
        }
        executeShell(fileStream, false);
        fileStream.close();
    }
    else {
        executeShell(cin, true);
    }
    cout << "Bye, See you soon :X" << endl;
    return 0;
}


