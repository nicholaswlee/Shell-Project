#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include<sys/wait.h>
#include <fcntl.h>
#include <errno.h>

/* Prints a message */
void myPrint(char* msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));

}

/* Prints an error */
void printError()
{
    char error_message[30] = "An error has occurred\n";
    write(STDOUT_FILENO, error_message, strlen(error_message));
}

void nullArguments(char **args)
{
    int i;
    for(i = 0; i < 256; i++){
        args[i] = NULL;
    }
}

char** tokenizeArgument(char *pinput, char **arguments_command)
{
    char avoidedChars[] = {'\n','\t', ' ', '\0'};
    char *arguments_buffer[256];
    nullArguments(arguments_buffer);
    int i = 0, arguments_size;
    char *token = strtok(pinput, avoidedChars);
    while(token != NULL){
        myPrint(token);
        //token = strdup(token);
        arguments_buffer[i] = token;
        token = strtok(NULL, avoidedChars);
        i++;
    }
    arguments_size = i;
    arguments_command = (char**)malloc(sizeof(char*)*arguments_size);
    for(i=0; i <arguments_size; i++)
    {
        arguments_command[i] = strdup(arguments_buffer[i]);
    }
    arguments_command[i] = NULL;
    return arguments_command;
}
/* Frees the heap allocated arguments */
void freeArguments(char **args)
{
    int i=0;
    while(args[i] != NULL){
        free(args[i]);
        i++;
    }
    free(args);
}

/* Runs a command */

void runCommand(char *pinput, char **arguments_command)
{
    pid_t child_pid;
    char avoidedChars[] = {'\n','\t', ' ', '\0'};
    char *arguments_buffer[256];
    char pwdBuff[256];
    int success;
    nullArguments(arguments_buffer);
    int i = 0, arguments_size;
    char *token = strtok(pinput, avoidedChars);
    while(token != NULL){
        //token = strdup(token);
        arguments_buffer[i] = token;
        token = strtok(NULL, avoidedChars);
        i++;
    }
    arguments_size = i;
    arguments_command = (char**)malloc(sizeof(char*)*arguments_size);
    for(i=0; i <arguments_size; i++)
    {
        arguments_command[i] = strdup(arguments_buffer[i]);
    }
    arguments_command[i] = NULL;
    if(arguments_command[0] == NULL){
        //BLANK LINE
    }else if(!strcmp(arguments_command[0], "cd")){
        if(arguments_command[1] == NULL){
            chdir(getenv("HOME"));
        }else if(arguments_command[2] != NULL){
            printError();
        }else{
            int cdSucc = chdir(arguments_command[1]);
            if(cdSucc == -1){
                printError();
            }  
        }
    }else if(!strcmp(arguments_command[0], "exit")){
        if(arguments_command[1] != NULL){
            printError();
        }else{
            exit(0);
        }
    }else{
        child_pid = fork();
        if(child_pid == 0){
            if(!strcmp(arguments_command[0], "pwd")){
                if(arguments_command[1] != NULL){
                    printError();
                }else{
                    myPrint(getcwd(pwdBuff, 256));
                    myPrint("\n");
                }
                exit(0);
            }else if(!strcmp(arguments_command[0], "exit")){
                if(arguments_command[1] != NULL){
                    printError();
                }
                exit(0);
            }else{
                success = execvp(arguments_command[0], arguments_command);
                if (success == -1){
                    printError();
                }
                exit(0);
            }
        }else{
            wait(NULL);
        }
    }
}
/* This needs to account for all forms of piping and illegal forms */
void pipeCommand(char *pinput, char **arguments_command)
{
    pid_t child_pid;
    char avoidedChars[] = {'\n','\t', ' ', '\0'};
    char *arguments_buffer[256];
    char file_name[256];
    int success;
    int fd = 0;
    char *piping = ">";
    int i = 0, arrow_count = 0, arguments_size;
    char *pipe_buffer[3];
    while(pinput[i] != '\0'){
        if(pinput[i] == '>'){
            arrow_count++;
        }
        i++;
    }
    i = 0;
    if(arrow_count > 1){
        printError();
    }else{
        char *token = strtok(pinput, piping);
        while(token != NULL){
            //token = strdup(token);
            pipe_buffer[i] = token;
            token = strtok(NULL, piping);
            i++;
        }
        nullArguments(arguments_buffer);
        i = 0;
        token = strtok(pipe_buffer[0], avoidedChars);
        while(token != NULL){
            //token = strdup(token);
            arguments_buffer[i] = token;
            token = strtok(NULL, avoidedChars);
            i++;
        }
        arguments_size = i;
        arguments_command = (char**)malloc(sizeof(char*)*arguments_size);
        for(i=0; i <arguments_size; i++)
        {
            arguments_command[i] = strdup(arguments_buffer[i]);
        }
        arguments_command[i] = NULL;
        if(arguments_command[0] == NULL){
            printError();
        }else if((!strcmp(arguments_command[0], "cd")) || 
            !strcmp(arguments_command[0], "pwd") || 
            !strcmp(arguments_command[0], "exit")){
                printError();
        }else{
            sscanf(pipe_buffer[1], "%s", file_name);
            fd = open(file_name, O_CREAT | O_EXCL | O_RDWR);
            if(EEXIST == errno || fd == -1){
                close(fd);
                printError();   
            }else{
                child_pid = fork();
                if(child_pid == 0){
                    dup2(fd, STDOUT_FILENO);
                    success = execvp(arguments_command[0], arguments_command);
                    if (success == -1){
                        printError();
                    }
                    exit(0);
                }else{
                    wait(NULL);
                }
            }
        }   
        //freeArguments(arguments_command); 
    }
}

/* This is extremely glitchy and only works half the time*/
void pipeAdvancedCommand(char *pinput, char **arguments_command)
{
    char c[2];
    pid_t child_pid;
    char avoidedChars[] = {'\n','\t', ' ', '\0'};
    char *arguments_buffer[256];
    char file_name[256];
    int success;
    int fd = 0;
    char *piping = ">";
    int i = 0, arrow_count = 0, arguments_size;
    char *pipe_buffer[3];
    char pinputArray[514];
    while(pinput[i] != '\0'){
        if(pinput[i] == '>'){
            arrow_count++;
        }
        i++;
    }
    i = 0;
    if(arrow_count > 1){
        printError();
    }else{
        //Copys pinput into a char array
        while(pinput[i] != '\0'){
            pinputArray[i] = pinput[i];
            i++;
        }
        pinputArray[i] = '\0';
        i = 0;
        //Creating tokens out of the >
        char *token = strtok(pinputArray, piping);
        while(token != NULL){
            //token = strdup(token);
            pipe_buffer[i] = token;
            token = strtok(NULL, piping);
            i++;
        }
        //Checks to see if the next charcter after the > is a +
        if(pipe_buffer[1][0] != '+' || (strlen(pipe_buffer[1]) < 2)){
            //myPrint(pipe_buffer[1]);
            printError();
        }else{
            //Parses out the +
            char *file = strtok(pipe_buffer[1] , "+");
            nullArguments(arguments_buffer);
            i = 0;
            token = strtok(pipe_buffer[0], avoidedChars);
            while(token != NULL){
                arguments_buffer[i] = token;
                token = strtok(NULL, avoidedChars);
                i++;
            }
            arguments_size = i;
            arguments_command = (char**)malloc(sizeof(char*)*arguments_size);
            //Copying the arguments over to an array that is the correct length
            for(i=0; i <arguments_size; i++)
            {
                arguments_command[i] = strdup(arguments_buffer[i]);
            }
            arguments_command[i] = NULL;
            if(arguments_command[0] == NULL){
                printError();
            }else if((!strcmp(arguments_command[0], "cd")) || 
                !strcmp(arguments_command[0], "pwd") || 
                !strcmp(arguments_command[0], "exit")){
                    printError();
            }else{
                sscanf(file, "%s", file_name);
                if(strlen(file_name) == 0){
                    printError(); 
                }else{
                    fd = open(file_name, O_RDWR | O_CREAT , 0664);
                    if(fd == -1){
                        close(fd);
                        printError();   
                    }else{
                        //Create a child process
                        child_pid = fork();
                        if(child_pid == 0){
                            int fd_open = open("temporary.tmp", O_CREAT | O_EXCL | O_TRUNC | O_RDWR);
                            if(fd_open == -1){
                                printError();
                                exit(0);
                            }
                            while(read(fd, c, 1)){
                                write(fd_open, c, 1);
                            }
                            close(fd);
                            child_pid = fork();
                            if(child_pid == 0){
                                fd = open(file_name, O_TRUNC | O_EXCL | O_RDWR | O_APPEND);
                                dup2(fd, STDOUT_FILENO);
                                success = execvp(arguments_command[0], arguments_command);
                                if (success == -1){
                                    printError();
                                }
                                exit(0);
                            }else{
                                wait(&child_pid);
                                child_pid = fork();
                                if(child_pid == 0){
                                    close(fd);
                                    fd = open(file_name, O_CREAT | O_WRONLY | O_APPEND);
                                    if(fd == -1){
                                        close(fd);
                                        printError();   
                                    }
                                    off_t t = lseek(fd_open, 0, SEEK_SET);
                                    if(t == -1){
                                    }
                                    while(read(fd_open, c, 1)){
                                        write(fd, c, 1);
                                    }
                                    close(fd);
                                    exit(0);
                                }else{
                                    wait(&child_pid);
                                }
                                
                            }
                            close(fd);
                            close(fd_open);
                            remove("temporary.tmp");
                            exit(0);
                        }else{
                            wait(&child_pid);
                            close(fd);
                            
                        }
                        
                        
                    }
                }
            }
            freeArguments(arguments_command); 
        }
    }
    
}

void tokenizeAndRunCommand(char *pinput, char **arguments_command)
{
    if(strchr(pinput, '>')){
        if(strchr(pinput, '+')){
            pipeAdvancedCommand(pinput, arguments_command);
        }else{
            pipeCommand(pinput, arguments_command);
        }
    }else{
        runCommand(pinput, arguments_command);
    }
}

void tokenizeAndRunMultiCommand(char *pinput, char **arguments_command)
{
    char *commands[256];
    char pinputArray[514];
    int i =0, command_count;
    while(pinput[i] != '\0'){
        pinputArray[i] = pinput[i];
        i++;
    }
    pinputArray[i] = '\0';
    nullArguments(commands);
    char *token = strtok(pinputArray, ";");
    i = 0;
    while(token != NULL){
        commands[i] = token;
        token = strtok(NULL, ";");
        i++;
    }
    command_count = i;
    for(i = 0; i < command_count; i++){
        tokenizeAndRunCommand(commands[i] ,arguments_command);
    }
}

void batchCommand(int fd)
{
    char c[2];
    char buffer[514];
    char **arguments_command = NULL;
    int i;
    int blank_line = 1, new_line = 0;
    
    while(read(fd, c, 1)){
        arguments_command = NULL;
        blank_line = 1;
        new_line = 0;
        i = 0;
        while((c[0] != '\n') && (i < 513)){
            buffer[i] = c[0];
            if(c[0] != ' ' && c[0] != '\n' && c[0] != '\t'){
                blank_line = 0;
            }
            i++;
            read(fd, c, 1);
            if(c[0] == '\n'){
                new_line = 1;
            }
        }
        buffer[i] = '\0';
        if(!blank_line){
            myPrint(buffer);
        }
        if(!blank_line && new_line){
            myPrint("\n");
            if (strchr(buffer, ';') != NULL)
            {
                tokenizeAndRunMultiCommand(buffer, arguments_command);
            }else{
                tokenizeAndRunCommand(buffer, arguments_command);
            }
        }else{
            if(!new_line && !blank_line){
                myPrint(c);
                while((c[0] != '\n')){
                    read(fd, c, 1);
                    myPrint(c);
                }
                printError();
            }
        }
    }
}
int main(int argc, char *argv[]) 
{
    char cmd_buff[514];
    //char exit_buff[5];
    char *pinput;
    char overflow_c;
    char **arguments_command = NULL;
    if (argc > 1){
        if(argc > 2){
            printError();
            exit(0);
        }
        int fd = open(argv[1], O_RDONLY);
        if(fd == -1){
            printError();
            exit(0);
        }else{
            batchCommand(fd);
        }
        
    }else{
        while (1) {
            myPrint("myshell> ");
            pinput = fgets(cmd_buff, 514, stdin);
            if(strchr(pinput, '\n') == NULL){
                myPrint(cmd_buff);
                while((overflow_c = getc(stdin)) != '\n'){
                    myPrint(&overflow_c);
                }
                myPrint("\n");
                printError();
            }
            else
            {
                if (strchr(pinput, ';') != NULL)
                {
                    tokenizeAndRunMultiCommand(pinput, arguments_command);
                }else{
                    tokenizeAndRunCommand(pinput, arguments_command);
                }
                
            }

        }
    }
    return 0;
}