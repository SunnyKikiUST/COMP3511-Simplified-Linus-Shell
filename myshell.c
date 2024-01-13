/*
    COMP3511 Fall 2023
    PA1: Simplified Linux Shell (MyShell)

    Your name:Lam Yeung Kong Sunny
    Your ITSC email:ykslam@connect.ust.hk

    Declaration:

    I declare that I am not involved in plagiarism
    I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks.

*/

/*
    Header files for MyShell
    Necessary header files are included.
    Do not include extra header files
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> // For constants that are required in open/read/write/close syscalls
#include <sys/wait.h> // For wait() - suppress warning messages
#include <fcntl.h>    // For open/read/write/close syscalls
#include <signal.h>   // For signal handling

// Define template strings so that they can be easily used in printf
//
// Usage: assume pid is the process ID
//
//  printf(TEMPLATE_MYSHELL_START, pid);
//
#define TEMPLATE_MYSHELL_START "Myshell (pid=%d) starts\n"
#define TEMPLATE_MYSHELL_END "Myshell (pid=%d) ends\n"
#define TEMPLATE_MYSHELL_TERMINATE "Myshell (pid=%d) terminates by Ctrl-C\n"

// Assume that each command line has at most 256 characters (including NULL)
#define MAX_CMDLINE_LENGTH 256

// Assume that we have at most 8 arguments
#define MAX_ARGUMENTS 8

// Assume that we only need to support 2 types of space characters:
// " " (space) and "\t" (tab)
#define SPACE_CHARS " \t" 

// The pipe character
#define PIPE_CHAR "|"

// Assume that we only have at most 8 pipe segements,
// and each segment has at most 256 characters
#define MAX_PIPE_SEGMENTS 8

// Assume that we have at most 8 arguments for each segment
// We also need to add an extra NULL item to be used in execvp
// Thus: 8 + 1 = 9
//
// Example:
//   echo a1 a2 a3 a4 a5 a6 a7
//
// execvp system call needs to store an extra NULL to represent the end of the parameter list
//
//   char *arguments[MAX_ARGUMENTS_PER_SEGMENT];
//
//   strings stored in the array: echo a1 a2 a3 a4 a5 a6 a7 NULL
//
#define MAX_ARGUMENTS_PER_SEGMENT 9

// Define the standard file descriptor IDs here
#define STDIN_FILENO 0  // Standard input
#define STDOUT_FILENO 1 // Standard output

// This function will be invoked by main()
// This function is given
int get_cmd_line(char *command_line)
{
    int i, n;
    if (!fgets(command_line, MAX_CMDLINE_LENGTH, stdin))
        return -1;
    // Ignore the newline character
    n = strlen(command_line);
    command_line[--n] = '\0';
    i = 0;
    while (i < n && command_line[i] == ' ')
    {
        ++i;
    }
    if (i == n)
    {
        // Empty command
        return -1;
    }
    return 0;
}

// read_tokens function is given
// This function helps you parse the command line
//
// Suppose the following variables are defined:
//
// char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
// int num_pipe_segments; // an output integer to store the number of pipe segment parsed by this function
// char command_line[MAX_CMDLINE_LENGTH]; // The input command line
//
// Sample usage:
//
//  read_tokens(pipe_segments, command_line, &num_pipe_segments, "|");
//
void read_tokens(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}

void process_cmd(char *command_line)
{
    // Uncomment this line to check the cmdline content
    printf("Debug: The command line is [%s]\n", command_line);
}

void signal_custom(int sig)
{
    int pid = getpid();
    printf(TEMPLATE_MYSHELL_TERMINATE, pid);
    exit(1);
}


/* The main function implementation */
int main()
{
    // TODO: replace the shell prompt with your ITSC account name
    // For example, if you ITSC account is cspeter@connect.ust.hk
    // You should replace ITSC with cspeter
    char *prompt = "ykslam";
    char command_line[MAX_CMDLINE_LENGTH];
    const char* exit_keyword = "exit";

    // TODO:
    // The main function needs to be modified
    // For example, you need to handle the exit command inside the main function
    printf(TEMPLATE_MYSHELL_START, getpid());
    signal(SIGINT, signal_custom);

    // The main event loop


    while (1)
    {

        printf("%s> ", prompt);
        if (get_cmd_line(command_line) == -1)
            continue; /* empty line handling */

        if(strcmp(command_line, exit_keyword) == 0){
            printf(TEMPLATE_MYSHELL_END,getpid());
            exit(0);
        }

        int pfds[2];
        //TASK 4
        if(strchr(command_line, '<') == NULL && strchr(command_line, '>') == NULL){
            
            int previous = STDIN_FILENO;

            char *pipe_segment[MAX_PIPE_SEGMENTS]; 
            int num_pipe_segments = 0;
            read_tokens(pipe_segment, command_line, &num_pipe_segments,PIPE_CHAR);

            for(int i = 0; i < num_pipe_segments; i++){
                
                pipe(pfds);
                char *pipe_segment_per_segment[MAX_ARGUMENTS_PER_SEGMENT]; //char pipe_segment_per_segment[MAX_ARGUMENTS][MAX_ARGUMENTS_PER_SEGMENT]
                int num_pipe_per_segment = 0;
                read_tokens(pipe_segment_per_segment, pipe_segment[i], &num_pipe_per_segment,SPACE_CHARS);
                pipe_segment_per_segment[num_pipe_per_segment] = NULL;

                pid_t pid = fork();

                if(pid == 0){
                    // printf("child");
                    // fflush(stdout);
                    if(num_pipe_segments == 1){
                        execvp(pipe_segment_per_segment[0],pipe_segment_per_segment);
                    }
                    else if(i == num_pipe_segments - 1){
                        close(STDIN_FILENO);
                        //dup2(pfds[0],STDIN_FILENO); // now pfds[0] be file descriptor at 0
                        dup2(previous,STDIN_FILENO); //previous output become current input 
                        close(pfds[1]); // close write-end pipe

                        // printf(" child1 ");
                        // fflush(stdout);
                        execvp(pipe_segment_per_segment[0],pipe_segment_per_segment);
                        
    
                    }
                    if(i > 0 && i != num_pipe_segments - 1){
                        close(STDIN_FILENO);
                        //dup2(pfds[0],0); //0 is stdin
                        dup2(previous,STDIN_FILENO); 
                    }
                    // printf("child2");
                    // fflush(stdout);
                    close(STDOUT_FILENO); //close stdout
                    dup2(pfds[1],STDOUT_FILENO); // now pfds[1] be file descriptor at 1
                    close(pfds[0]);
                    execvp(pipe_segment_per_segment[0],pipe_segment_per_segment);
                }
                else{
                        wait(0);
                        close(pfds[1]);
                        previous = pfds[0];

                    }
            
                }
                
            continue;
        }

        

        // FOR TASK 3

        pid_t pid = fork();


        if (pid == 0)
        {
            if(strchr(command_line, '<') != NULL && strchr(command_line, '>') == NULL){
                char *pipe_segment[MAX_PIPE_SEGMENTS];
                int num_pipe_segments = 0;
                read_tokens(pipe_segment, command_line, &num_pipe_segments,"<");

                char *filename[MAX_ARGUMENTS_PER_SEGMENT]; //get file name
                int num_pipe_per_segment = 0;
                read_tokens(filename, pipe_segment[1], &num_pipe_per_segment,SPACE_CHARS);

                char *command[MAX_ARGUMENTS_PER_SEGMENT]; //get file name
                num_pipe_per_segment = 0;
                read_tokens(command, pipe_segment[0], &num_pipe_per_segment,SPACE_CHARS);
                command[num_pipe_per_segment] = NULL;

                int file;
                // the child process handles the command
                //process_cmd(command_line);
                file = open(*filename ,O_CREAT | O_RDONLY , S_IRUSR | S_IWUSR );
                close(STDIN_FILENO);
                dup2(file, STDIN_FILENO);
                execvp(command[0],command);
            }
            else if(strchr(command_line, '<') == NULL && strchr(command_line, '>') != NULL){
                char *pipe_segment[MAX_PIPE_SEGMENTS];
                int num_pipe_segments = 0;
                read_tokens(pipe_segment, command_line, &num_pipe_segments,">");

                char *filename[MAX_ARGUMENTS_PER_SEGMENT]; //get file name
                int num_pipe_per_segment = 0;
                read_tokens(filename, pipe_segment[1], &num_pipe_per_segment,SPACE_CHARS);

                char *command[MAX_ARGUMENTS_PER_SEGMENT]; //get file name
                num_pipe_per_segment = 0;
                read_tokens(command, pipe_segment[0], &num_pipe_per_segment,SPACE_CHARS);
                command[num_pipe_per_segment] = NULL;

                int file;
                // the child process handles the command
                //process_cmd(command_line);
                file = open(*filename ,O_CREAT | O_WRONLY , S_IRUSR | S_IWUSR );
                // >
                close(STDOUT_FILENO);
                dup2(file, STDOUT_FILENO);
                execvp(command[0],command); 
            }
            else{
                char *pipe_segment[MAX_PIPE_SEGMENTS];
                int num_pipe_segments = 0;
                read_tokens(pipe_segment, command_line, &num_pipe_segments,">");

                if(strchr(pipe_segment[1], '<') != NULL){ // < is in the right side of >
                    char *pipe_segment_left[MAX_ARGUMENTS_PER_SEGMENT]; //command
                    int num_pipe_left = 0;
                    read_tokens(pipe_segment_left, pipe_segment[0], &num_pipe_left,SPACE_CHARS);
                    pipe_segment_left[num_pipe_left] = NULL;

                    char *pipe_segment_right_1[MAX_ARGUMENTS_PER_SEGMENT]; // seperate two file/docment
                    int num_pipe_right_1 = 0;
                    read_tokens(pipe_segment_right_1, pipe_segment[1], &num_pipe_right_1,"<");

                    // seperate two file/docment
                    char *pipe_segment_right_2[MAX_ARGUMENTS_PER_SEGMENT]; // first file(output)
                    int num_pipe_right_2 = 0;
                    read_tokens(pipe_segment_right_2, pipe_segment_right_1[0], &num_pipe_right_2,SPACE_CHARS);
                    //printf("%s", *pipe_segment_right_2);
                    fflush(stdout);
                    char *pipe_segment_right_3[MAX_ARGUMENTS_PER_SEGMENT]; // second file(input)
                    int num_pipe_right_3 = 0;
                    read_tokens(pipe_segment_right_3, pipe_segment_right_1[1], &num_pipe_right_3,SPACE_CHARS);

                    int fileForInput;
                    int fileForOutput;
            
                    fileForInput = open(*pipe_segment_right_3 ,O_CREAT | O_RDONLY , S_IRUSR | S_IWUSR );
                    fileForOutput = open(*pipe_segment_right_2 ,O_CREAT | O_WRONLY , S_IRUSR | S_IWUSR );
     
                    close(STDIN_FILENO);
                    dup2(fileForInput, STDIN_FILENO);
                    close(STDOUT_FILENO);
                    dup2(fileForOutput, STDOUT_FILENO);
                    execvp(pipe_segment_left[0],pipe_segment_left); 
                }
                else{
                    char *pipe_segment_right[MAX_ARGUMENTS_PER_SEGMENT]; //right side file (output)
                    int num_pipe_right = 0;
                    read_tokens(pipe_segment_right, pipe_segment[1], &num_pipe_right,SPACE_CHARS);

                    char *pipe_segment_left_1[MAX_ARGUMENTS_PER_SEGMENT]; // seperate command and left side file 
                    int num_pipe_left_1 = 0;
                    read_tokens(pipe_segment_left_1, pipe_segment[0], &num_pipe_left_1,"<");

                    // seperate command and a file
                    char *pipe_segment_left_2[MAX_ARGUMENTS_PER_SEGMENT]; // left 2 is command
                    int num_pipe_left_2 = 0;
                    read_tokens(pipe_segment_left_2, pipe_segment_left_1[0], &num_pipe_left_2,SPACE_CHARS);
                    pipe_segment_left_2[num_pipe_left_2] = NULL;

                    char *pipe_segment_left_3[MAX_ARGUMENTS_PER_SEGMENT]; // left 3 is file (input)
                    int num_pipe_left_3 = 0;
                    read_tokens(pipe_segment_left_3, pipe_segment_left_1[1], &num_pipe_left_3,SPACE_CHARS);

                    int fileForInput;
                    int fileForOutput;
            
                    fileForInput = open(*pipe_segment_left_3 ,O_CREAT | O_RDONLY , S_IRUSR | S_IWUSR );
                    fileForOutput = open(*pipe_segment_right ,O_CREAT | O_WRONLY , S_IRUSR | S_IWUSR );
     
                    close(STDIN_FILENO);
                    dup2(fileForInput, STDIN_FILENO);
                    close(STDOUT_FILENO);
                    dup2(fileForOutput, STDOUT_FILENO);
                    execvp(pipe_segment_left_2[0],pipe_segment_left_2); 
                }



            }


        }
        else
        {
            wait(0);
        }
    
    }
      
    
    return 0;
}