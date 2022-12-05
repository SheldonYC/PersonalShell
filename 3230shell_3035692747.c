/*
Name: Ng Yuk Chuen
UID: 3035692747
Development platform: 100% on workbench2 through termius:)))
Remark: able to execute simple command like ls, pwd. Able to do exit. Handled SIGINT and SIGUSR1
Did not do bonus part

*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include<sys/wait.h>
#include <errno.h>
#include<stdlib.h>
#include <signal.h>
#include <sys/resource.h>

// assume only 1025characters long and 30 strings in total
#define MAX_CHAR 1025
#define MAX_STR 30

// if SIGUSR1 is received, set flag = 0
int flag = 1;
int* ptr = &flag;

// print terminal
void printT() {printf("$$ 3230shell ## ");}

// print timeX statisitics
void printTimeX(int num, struct rusage* rusage, int* pid, char** cmd) {
    for (int i = 0; i < num +1; i++) {
        printf("(PID)%d  (CMD)%s    (user)%.3f s  (sys)%.3f s\n", pid[i], cmd[i], rusage[i].ru_utime.tv_sec + rusage[i].ru_utime.tv_usec / 1000000.0, rusage[i].ru_stime.tv_sec + rusage[i].ru_stime.tv_usec / 1000000.0);
    }
}

// trim preceding whitespaces given string
void trim(char* string) {
    int pos = 0;
    int temp = 0;
    char newStr[MAX_CHAR];
    for (int i = 0; i < MAX_CHAR; i++) {
        if (string[i] != 32) {
	// find earliest non-whitespace character
	    pos = i;
	    break;
	}
    }
    // return new string without whitespaces at front
    while (string[pos]!=0) {
        newStr[temp] = string[pos];
	temp++;
	pos++;
    }
    newStr[temp] = '\0';
    strcpy(string, newStr);
}

// parsing is taken example from "http://www.dmulholl.com/lets-build/a-command-line-shell.html"
void parseStr(char* string, char** parsed) {
    // parsing spacebar
    int pos = 0;
    // first token
    char* token = strtok(string, " \n");
    // strtok will give NULL at the end
    while (token != NULL) {
        parsed[pos] = token;
        token = strtok(NULL, " \n");
	pos++;
    }
    parsed[pos] = NULL;
    return;
}

// check for pipe symbol
int isPipe(char* command, int* num) {
    int len = strlen(command);
    int count = 0;
    // prompt again if | is the first or last character
    if (command[0] == 124 || command[len-2] == 124 ) {
        printf("Incorrect usage of |\n");
	strcpy(command,"");
	return 0;
    }
    // count number of "|"
    for (int i = 0; i < len; i++) {
        if (command[i] == 124) {
	    count++;
	}
    }
    // continue to parse with | if there is | symbol
    if (count > 0) {
        *num = count;
	return 1;
    }
    else {return 0;}
}

// return true if the command begins with timeX
int checkTimeX(char* command) {
    char* temp;
    temp = strdup(command);
    char* token = strtok(temp, " \n");
    // return 0 for no user input
    if (token == NULL) {return 0;}
    // if command first word is "timeX", trim it away or prompt again for incorrect usage
    if (strcmp(token, "timeX") == 0) {
        // check for incorrect usage
        token = strtok(NULL, " \n");
	if (token == NULL) {
            strcpy(command, "");
	    printf("3230shell: \"timeX\" cannot be a standalone command\n");
	}
        return 1;
    }
    return 0;
}

// parsing pipe, extracting command between pipes
int parsePipe(char* string, char** pipeBuffer, int pipeCount, int isTimeX) {
    int pos = 0;
    if (isTimeX) {
        // trim "TimeX "
        string+=6;
    }
    // first token
    char* token  = strtok(string, "|");
    // preparing commands to be executed in child process
    while (token != NULL) {
        // remove whitespace at first and last characters
	trim(token);
	// report incorrect usage of pipe
	if (strcmp(token, "") == 0) {
	    printf("3230shell: should not have two consecutive | without in-between command\n");
	    return 0;
	}
	pipeBuffer[pos] = token;
	pos++;
	token = strtok(NULL, "|");
    }
    if (pipeCount >= pos) {
        printf("3230shell: should not have two consecutive | without in-between command\n");
	return 0;
    }
    pipeBuffer[pos] = NULL;
    return 1;
}

// signal handlers
void sigusr1() {
    *ptr = 0;
//    printf("received SIGUSR1\n");
}

// handler for main process to not be interrupted
void parentSIGINT(int signum) {
    printf("\n");
    printT();
    fflush(stdout);
}

// check for built-in function
int isBuiltIn(char** parsed, char** builtIn) {
    for (int i = 0; i < 2; i++) {
        if (strcmp(builtIn[i], parsed[0]) == 0) {
	    return 1;
	}
    }
    // return false if not built-in function
    return 0;
}

// execute built in function like exit
void execBuiltIn(char** parsed, char** builtIn) {
    for (int i = 0; i < 2; i++) {
        if (strcmp(builtIn[i], parsed[0]) == 0) {
	// switch does not take 0 as case
	i++;
	switch (i) {
	    // exit
	    case 1:
	    if (parsed[1] == NULL) {
	        printf("3230shell: Terminated\n");
	        exit(0);
	    }
	    else {printf("3230shell: \"exit\" with other arguments!!!\n");}
	    // timeX, but the checking is moved to checkTimeX
	    case 2: {return;}
	    default: return;
	    }
	}
    }
}

// execute simple function like ls
void execCMD(char** parsed) {
    int status;
    *ptr = 1;
    signal(SIGUSR1, sigusr1);
    pid_t pid = fork();
    // failed fork
    if (pid == -1) {
        printf("Failed fork\n");
	return;
    }
    // child process
    else if (pid == 0) {
        // spin-wait for SIGUSR1 before execvp
        while (flag) {
	    sleep(1);
	}
	execvp(parsed[0], parsed);
	// if function cannot be executed
	printf("3230shell: \'%s\': %s\n", parsed[0], strerror(errno));
	exit(0);
    }
    // parent process
    else {
        // sending SIGUSR1 to child
	kill(pid, SIGUSR1);
	waitpid(pid, &status, 0);
	// if child is terminated by signal
	if (WIFSIGNALED(status)) {
	    switch(WTERMSIG(status)){
	    // interrupt
	    case 2: {printf("Interrupt\n"); return;}
	    // kill
	    case 9: {printf("Killed\n"); return;}
	    // Not mentioned in doc, left unhandled
	    default: return; 
	    }
	}
	return;
    }
}

// multiple fork approach from https://www.youtube.com/watch?v=NkfIUo_Qq4c
void execPipe(char** pipeBuffer, char** parsed, int count, struct rusage* rusage, int* pid, char** commandList){
    int pos = 0;
    int status;
    int pipeCount = count + 1;
    char command[MAX_CHAR];
    char* tempCMD;
    char *output;
    int saved_stdout = dup(1);
    // 6 pipes maximum for 5 child process and 1 parent
    int fd[6][2];
    for (int i = 0; i < 6; i++ ) {
         if (pipe(fd[i]) < 0) {
	     printf("Errors during pipe creation, exiting...\n");
	     exit(1);
	 }
    }
    // preparing first pipe to start the following loop
    if (write(fd[0][1], &pos, sizeof(int)) == -1 ) {
    printf("Errors during pipe write, exiting...\n");
    exit(1);
    }
    close(fd[0][1]);

    // create child process to execute commands
    for (int i = 0; i < pipeCount; i++) {
        pid[i] = fork();
	if (pid[i] == 0) {
	     // close all unused pipes
	     for (int j = 0; j < pipeCount; j++) {
		 if (j != i+1) {close(fd[j][1]);}
		 else if (j != i){close(fd[j][0]);}
	     }
	     strcpy(command, pipeBuffer[i]);
	     // redirect pipe as input
	     dup2(fd[i][0], STDIN_FILENO);
	     // close reading end
	     close(fd[i][0]);
	     // redirect pipe as output
	     if (i != pipeCount - 1) {
	         dup2(fd[i+1][1], STDOUT_FILENO);
	     }
	     else {dup2(saved_stdout, 1);}
	     // close writing end
	     close(fd[i+1][1]);
	     parseStr(command, parsed);
	     execCMD(parsed);
	     // exit child process to prevent unwanted fork
	     exit(0);
	}
	// wait for child process before next iteration
	else {
	    // close the reading pipe, prevent infinite waiting input
	    close(fd[i][0]);
	    close(fd[i+1][1]);
	    // store terminated process status for timeX
	    wait4(pid[i], &status, 0, &rusage[i]);
	    // store only the program name
	    tempCMD = strdup(pipeBuffer[i]);
	    strtok(tempCMD, " \n");
	    commandList[i] = tempCMD;
	}
    }
    close(fd[pipeCount][0]);
}

// updated execCMD to work with timeX. All command execution is still done by execCMD
// but NEW_execCMD will collect information of process.
/*void NEW_execCMD(char** parsed, struct rusage* rusage, int* pid, char** commandList) {
    int status;
    pid[0] = fork();
    if (pid[0] == 0){
        execCMD(parsed);
    }
    else {
        //printf("%s\n", parsed[0]);
	// store terminated process status for timeX
        wait4(pid[0], &status, 0, &rusage[0]);
        strcpy(commandList[0], parsed[0]);
    }
}*/

int main () {
    char command[MAX_CHAR];
    char* pipeBuffer[MAX_CHAR];
    char* parsed[MAX_CHAR];
    int isTimeX;
    int pipeCount = 0;
    char* builtIn[2];
    builtIn[0] = "exit";
    builtIn[1] = "timeX";
    // monitor status of terminated child process
    struct rusage rusage[6];
    int pid[6];
    char* commandList[6];

    signal(SIGINT, parentSIGINT);
    while (1) {
        flag = 1;
	isTimeX = 0;
	pipeCount = 0;
	printT();
	// get user input
	fgets(command, MAX_CHAR, stdin);
	// if pipe symbol exist, parse for pipe
	if (isPipe(command, &pipeCount)) {
	    // check if it is timeX command
	    isTimeX = checkTimeX(command);
	    // execute pipe command if no wrong usage
	    if (parsePipe(command, pipeBuffer, pipeCount, isTimeX)) {
	        // execute commands
	        execPipe(pipeBuffer, parsed, pipeCount, rusage, pid, commandList);
	    }
	    // print terminated process statistics if it is timeX command
	    if (isTimeX){
	        printTimeX(pipeCount, rusage, pid, commandList);
	    }
	}
	else {
	    // check for timeX and restructure command
	    isTimeX = checkTimeX(command);
	    // trim "TimeX "
	    if (isTimeX){
	        char* temp = command;
		temp+=6;
	        strcpy(command, temp);
	    }
	    // parse for string
	    parseStr(command, parsed);
	    // prompt again if no user input
	    if (parsed[0] == NULL) {continue;}
	    commandList[0] = parsed[0];
	    // check for built-in function first, then linux command
	    if (isBuiltIn(parsed, builtIn)) {
	        execBuiltIn(parsed, builtIn);
	    }
	    else {
	        //NEW_execCMD(parsed, rusage, pid, commandList);
		execCMD(parsed);    
	        if (isTimeX) {
	            printTimeX(0,rusage, pid, commandList);
	        }    
	    }
	}
    }
}
