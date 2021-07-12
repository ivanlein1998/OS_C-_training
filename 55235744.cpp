// Assignment 1 Name: Lein Chun Hang eid: chlein2 SID: 55235744
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <cstring>
using namespace std;

pid_t pid; //store pid to determine whether the process is parent/child
pid_t bgPid; //store bg process PID
pid_t fgPid;  // store fg process PID
int status; //store process status: running, stopped, terminated
bool fgRunning; // to tell if there is fg process running

bool fileExist(const char *filename) { // check if the input file path is exist
    std::ifstream ifile(filename);
    return (bool) ifile;
}

void fg(char **input){ //create foreground process
    if (fgRunning == false){
        fgRunning = true;
        pid = fork();
        if (pid < 0) exit(EXIT_FAILURE);
        fgPid = getpid();
        char *fgFilename = input[0];
        if (execvp(fgFilename, input) == -1) {
            // If execute failed
            if (fileExist(fgFilename) == false) {
                cout << "sh >File not found, please input a correct file path"<< endl;
            }
            else{
                cout << "sh >interval or times valuse incorrect, plaese enter correct base-10 mathimatical value for interval and times" << endl;
            }
        }
    }
    else{
        cout << "sh >There is already an foreground process running, please wait for it to be stopped/terminated. " << endl;
    }
}

void bg(char **input){ //create background process
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    else{
        char *bgFilename = input[0];
        if (execvp(bgFilename, input) == -1) {
            // If execute failed
            if (fileExist(bgFilename) == false) {
                cout << "sh >File not found, please input a correct file path"<< endl;
            }
            else{
                cout << "sh >interval or times valuse incorrect, plaese enter correct base-10 mathimatical value for interval and times"<< endl;
            }
        }
    }
}
void fgSigint(int signo) { //signal handle of ctrl-c
    // Check status
    if (fgRunning == false) {
        cout << "sh >No foreground process is running." << endl;
    }
    // Terminate the process
    kill(fgPid, SIGINT);
    cout << "sh >Process " << fgPid << " terminated." << endl;
    // Reset all flag
    fgRunning = false;

}

void fgSigtstp(int signo) {//signal handle of ctrl-z
    // Check status
    if (fgRunning == false) {
        cout << "sh >No foreground process is running." << endl;

    }
    // Stop the process
    kill(fgPid, SIGTERM);
    cout << "sh >Process " << fgPid << " stopped.\n";

    // Reset all flag
    fgRunning = false;

}

void sig_chld(int signo)
{ //signal handle when receive a child process stopped/terminated
    pid_t pid;
    while ( (pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)){
            cout << "Process " << pid << " completed";
        }
    }
}

void list(){

    //waiting to complete
}
void exit_shell(){

    //waiting to complete
}

int main(){
    while(true){
        signal(SIGINT, fgSigint);
        signal(SIGTSTP, fgSigtstp);
        //signal(SIGCHLD,sig_chld);
        char *input = NULL;
        char *args[3];
        char prompt[] = "sh >"; 
        input = readline("sh >");      
        char *p = strtok(input," \t"); // cut upon <space> or <tab>    
        if (!p) {
            cout << "No input detected!\n";   
            continue;    
        }
        else if (strcmp(p, "fg") == 0) {
            int count = 0;
			while (p=strtok(NULL," \t")){
                args[count] = p;
                count++;
            }
            fg(args);
            int printTime = atoi(args[2]);
            int interval = atoi(args[3]);
            int sleeptime = printTime * interval;
            sleep(sleeptime);
		}
		else if (strcmp(p, "bg") == 0) {
            int count = 0;
			while (p=strtok(NULL," \t")){
                args[count] = p;
                count++;
            }
            bg(args);
		}
		else if (strcmp(p, "list") == 0) {
		//waiting to completed

		}
		else if (strcmp(p, "exit") == 0) {
		//waiting to completed
            
        }
    }
}