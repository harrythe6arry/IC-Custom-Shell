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
#include <ctime>
#include <chrono>
#include <thread>

using namespace std;
void printColored(const string &message, const char *colorCode); 
void executeEcho(const string &input);
int execExit(const string &input);
void ignore_signals();
void default_signals();
void handleIO(char *args[], bool &input_redir, bool &output_redir);
void completejobs(pid_t pid, int status);
void sigchild(int signum);
void listjobs();
void foreground_job(int job_id);
void backgroundjobs(int job_id);
void executeShell(istream &user_input, bool user_mode);\
void greets();
int main(int argc, char *argv[]);




// ANSI color codes
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_COLOR_BRIGHT "\x1b[1m"
#define ANSI_COLOR_DIM "\x1b[2m"


const int MAX_ARGS = 256;
volatile pid_t currentchild = -1;
int exitCode = 0;
const int MAX_BG = 256;
int bg_count = 0; // number of background processes
pid_t bg_processes[MAX_BG];
// bool externalecho = false; // special condition for echo
string bg_commands[MAX_BG];
volatile bool job_track[MAX_BG] = {false};

void printColored(const string &message, const char *colorCode)
{
  cout << colorCode << message << ANSI_COLOR_RESET;
}

void executeEcho(const string &input)
{
  cout << input.substr(5) << endl;
}

int execExit(const string &input)
{
  int numexit = stoi(input.substr(5)) & 0xFF;
  return numexit;
}

void ignore_signals()
{ // ignore signals
  signal(SIGINT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
}
void default_signals()
{ // restore default signals
  signal(SIGINT, SIG_DFL);
  signal(SIGTSTP, SIG_DFL);
}

// hndle input and output redirection
void handleIO(char *args[], bool &input_redir, bool &output_redir)
{
  int in;
  int out;

  // if (!externalecho)
  // { // if echo is not used
    for (int i = 0; args[i] != NULL; i++)
    { // Loop through the arguments
      if (strcmp(args[i], "<") == 0)
      {
        input_redir = true;
        args[i] = NULL;
        in = open(args[i + 1], O_RDONLY);
        if (in < 0)
        {
          printColored("Error: " + string(strerror(errno)), ANSI_COLOR_RED);
          exit(errno);
        }
        dup2(in, STDIN_FILENO);
        close(in);
        args[i] = NULL;
        i++;
      }
      else if (strcmp(args[i], ">") == 0)
      {
        output_redir = true;
        args[i] = NULL;
        out = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (out < 0)
        {
          printColored("Error: " + string(strerror(errno)), ANSI_COLOR_RED);
          exit(errno);
        }
        dup2(out, STDOUT_FILENO);
        close(out);
        args[i] = NULL;
        i++;
      }
    // }
  }
}

// complete the background jobs
void completejobs(pid_t pid, int status)
{
  for (int i = 0; i < bg_count; i++)
  {
    if (bg_processes[i] == pid)
    { // job is done
      printColored("\n[" + to_string(i + 1) + "]  Done                 " + bg_commands[i] + "\n", ANSI_COLOR_GREEN);
      // cout << "\n[" << i + 1 << "]  Done                 " << bg_commands[i] << endl;

      bg_processes[i] = 0;  // reset the process
      bg_commands[i] = "";  // reset the command
      // cout << "X SHELL $ "; // print the prompt and flush the output
      printColored("X SHELL $ ", ANSI_COLOR_CYAN);
      fflush(stdout);
    }
  }
}

// sigchild does the signal handling for SIGCHLD which is sent to the parent process when a child process terminates and the parent process is in the foreground

void sigchild(int signum)
{ // signal handler for SIGCHLD
  int saved_value = errno;
  pid_t pid;
  int status;
  while ((pid = waitpid((pid_t)(-1), &status, WNOHANG)) > 0)
  {
    completejobs(pid, status);
  }

  errno = saved_value;
}

void listjobs()
{ // this is the function to list the background jobs
  for (int i = 0; i < bg_count; i++)
  {
    if (bg_processes[i] == 0)
    { // no jobs are running
      // cout << "No jobs are running" << endl;
      printColored("No jobs are running\n", ANSI_COLOR_RED);
    }
    else
    { // jobs are running
      printColored("[" + to_string(i + 1) + "] Running                  " + " " + bg_commands[i] + "&\n", ANSI_COLOR_YELLOW);
      // cout << "[" << i + 1 << "] Running                  " << " " << bg_commands[i] << "&" <<endl;
    }
  }
}

void foreground_job(int job_id)
{
  if (job_id < 0 || job_id >= bg_count)
  {
    
    printColored("Invalid job ID", ANSI_COLOR_RED);
    return;
  }
  pid_t pid = bg_processes[job_id];
  if (pid == 0)
  {
    printColored("No such job", ANSI_COLOR_RED);
    
    return;
  }
  
  int status;
  
  printColored("Foreground process     " + to_string(pid) + " started " + bg_commands[job_id] + "\n", ANSI_COLOR_BLUE);
  // cout << bg_commands[job_id] << endl;
  kill(pid, SIGCONT);
  waitpid(pid, &status, WUNTRACED);
  printColored("\nForeground process " + to_string(pid) + " exited with status " + to_string(status), ANSI_COLOR_MAGENTA);
  if (WIFEXITED(status))
  {
    completejobs(pid, status);
  }
}

void backgroundjobs(int job_id)
{
  if (job_id < 0 || job_id >= bg_count)
  {
    // cerr << "Invalid job ID" << endl;
    printColored("Invalid job ID", ANSI_COLOR_RED);
    return;
  }
  pid_t pid = bg_processes[job_id];
  if (pid == 0)
  {
    printColored("No such job", ANSI_COLOR_RED);
    // cerr << "No such job" << endl;
    return;
  }
  // cout << "Background process " << pid << " started" << endl;
  printColored("Background process " + to_string(pid) + " started", ANSI_COLOR_BLUE);
  cout << bg_commands[job_id] << endl;
  kill(pid, SIGCONT);
}


void executeShell(istream &user_input, bool user_mode)
{
  string input, last_input = "";
  ignore_signals();
  while (true)
  {
    if (user_mode)
    {
      // cout << "X SHELL $ ";
      printColored("\nX SHELL $ ", ANSI_COLOR_CYAN);
      fflush(stdout);
    }
    if (!getline(user_input, input))
    {
      if (user_input.eof())
      {
        cout << "End of file reached or bye" << endl;
      }
      else
      {
        cout << "Input error or bye" << endl; // For unexpected errors
      }
      break;
    }
    if (input.empty())
      continue;

    if (input.rfind("fg ", 0) == 0)
    {
      string jobto_string = input.substr(input.find('%') + 1);
      int job_id = -1;
      try
      {
        job_id = stoi(jobto_string) - 1; // Convert job ID from string to int, adjust for 0-based indexing
        foreground_job(job_id);
      }
      catch (const invalid_argument &e)
      {
        cerr << "Invalid job ID format" << endl;
      }
      catch (const out_of_range &e)
      {
        cerr << "Job ID out of range" << endl;
      }
      continue; // Skip to the next iteration of the loop
    }

    bool have_bg = false;
    if (input.back() == '&')
    {
      have_bg = true;
      input.pop_back();
    }
    if (input == "!!")
    {

      if (!last_input.empty())
      {
        input = last_input;
        if (user_mode)
        {
          // cout << input << endl;
          printColored(input, ANSI_COLOR_MAGENTA);
        }
        exitCode = 0;
      }
      else
      {
        // cout << "No commands in history." << endl;
        printColored("No commands in history.", ANSI_COLOR_RED);
        exitCode = 0;
        continue;
      }
    }
    else
    {
      last_input = input;
    }
    if (input.substr(0, 2) == "##")
      continue; // comment line ignore
    if (input.substr(0, 4) == "jobs")
    {
      listjobs();
      continue;
    }
    // if (externalecho)
    // {
      if (input.substr(0, 5) == "echo ")
      {
        if (input == "echo $?")
        {
          cout << "The exit code is " << exitCode << endl;
          exitCode = 0;
        }
        else
        {
          executeEcho(input);
          continue;
        }
      }
    // }

      if (input.rfind("bg ", 0) == 0) {
        string jobto_string = input.substr(input.find('%') + 1);
        int job_id = -1;
        try {
          job_id = stoi(jobto_string) - 1; // Convert job ID from string to int, adjust for 0-based indexing
          backgroundjobs(job_id);
        } catch (const invalid_argument& e) {
          cerr << "Invalid job ID format" << endl;
        } catch (const out_of_range& e) {
          cerr << "Job ID out of range" << endl;
        }
        continue; // Skip to the next iteration of the loop
      }

    if (input == "pwd") {
      char curr_direc[1024];
      if (getcwd(curr_direc, sizeof(curr_direc)) != NULL) {
        cout << curr_direc << endl;
      } else {
        cerr << "getcwd() error" << endl;
      }
      continue;
    }

    else if (input.substr(0, 4) == "exit")
    {
      exitCode = execExit(input);
      cout << "Exiting with code: " << exitCode << endl;
      // cout << "Bye, See you soon :X" << endl;
      printColored("Goodbye, See you soon, :X\n", ANSI_COLOR_YELLOW);
      exit(exitCode);
    }


    else
    {
      char *args[MAX_ARGS];                         // Array of pointers to the arguments
      char *new_str = new char[input.length() + 1]; // Create a new string to store the input
      // cout << "the new str is " << new_str << endl;
      strcpy(new_str, input.c_str()); // Copy the input to the new string
      char *in_token_string = strtok(new_str, " "); // Tokenize the input string
      // cout << "the p is " << p << endl;
      int i = 0; // Index for the args array
      while (in_token_string != NULL)
      { // Loop through the tokens and add them to the args array
        args[i++] = in_token_string;
        in_token_string = strtok(NULL, " "); // Get the next token
      }

      args[i] = NULL;            // Set the last element of the args array to NULL
      bool input_redir = false;  // Set to true if input redirection is used
      bool output_redir = false; // Set to true if output redirection is used

      currentchild = fork(); // Create a new process
      if (currentchild == -1)
      { // Check if the fork failed
        cerr << "Fork failed." << endl;
      }
      else if (currentchild == 0)
      {
        default_signals(); // Restore default signal behavior for the child process
        // cout << "Getting pid when current child = 0 " << getpid() << endl;
        // cout << "current child is when current == 0 " << currentchild << endl;
        handleIO(args, input_redir, output_redir);

        if (execvp(args[0], args) == -1)
        {
          // cerr << "bad command" << endl;
          printColored("bad command", ANSI_COLOR_RED);
          exit(EXIT_FAILURE);
        }
      }
      else
      {
        if (!have_bg)
        {
          int status;
          // cout << "Getting pid " << getpid() << endl;
          // cout << "current child is " << currentchild << endl;
          waitpid(currentchild, &status, WUNTRACED);
          // cout << "Getting pid" << getpid() << endl;
          if (WIFEXITED(status) && WTERMSIG(status) == SIGINT)
          {
            exitCode = 0;
            // cout << "The process was terminated" << exitCode << endl;
          }
          else if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTSTP)
          { // Check if the process was stopped by SIGTSTP
            exitCode = 146;
          }
        }
        else
        {
          if (have_bg)
          { // if the process is a background process
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

void greets()
{
  cout << "Welcome to the XSHELL\n"
       << endl;
  // display the time and stuff
  time_t t = time(nullptr);
  cout << ctime(&t);
  cout << "Here are some quotes to inspire you:" << endl;
  cout << "--------------------------------------" << endl;

  const char *quotes[] = {
      "\nWith the new day comes new strength and new thoughts., Eleanor Roosevelt\n",
      "\nThe only way to do great work is to love what you do., Steve Jobs\n",
      "\nThe best way to predict the future is to create it., Peter Drucker\n",
      "\nThe only limit to our realization of tomorrow will be our doubts of today., Franklin D. Roosevelt\n",
      "\nThe future belongs to those who believe in the beauty of their dreams., Eleanor Roosevelt\n"};
  printColored(quotes[rand() % 5], ANSI_COLOR_YELLOW);
  cout << endl
       << "--------------------------------------" << endl;
}

int main(int argc, char *argv[])
{
  signal(SIGCHLD, sigchild); // signal handler for SIGCHLD
  for (int i = 0; i < MAX_BG; i++)
  {                      // initialize the background processes
    bg_commands[i] = ""; // reset the command
  }

  greets();
  // cout << "Starting the XSHELL.." << endl;
  printColored("\nWelcome to the XSHELL \n", ANSI_COLOR_RED);
  if (argc > 1)
  {
    ifstream fileStream(argv[1]);
    if (!fileStream)
    {
      cerr << "Cannot open file: " << argv[1] << endl;
      return 1;
    }
    executeShell(fileStream, false);
    fileStream.close();
  }
  else
  {
    executeShell(cin, true);
  }
  // cout << "Bye, See you soon :X" << endl;
  printColored("Goodbye, See you soon, :X\n", ANSI_COLOR_YELLOW);
  return 0;
}
