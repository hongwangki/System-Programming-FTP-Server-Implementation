/////////////////////////////////////////////////////////////////////
// File Name : cli.c                                               //
// Date : 2024/05/11                                               //
// OS : Ubuntu 20.04.6 LTS 64bits                                  //
//                                                                 //
// Author : Hong Wang ki                                           //
// Student ID : 2020202060                                         //
// ---------------------------------------------------------------- //
// Title : System Programming Assignment #2-3 ftp server           //
// Description: This program switches Linux commands to FTP commands//
//              and sends them to the server.                      //
/////////////////////////////////////////////////////////////////////
// Function: main conv_cmd sh_int                                   //
// ================================================================= //
// Input:                                                             //
//     argc: number of command-line arguments                         //
//     argv: array of command-line arguments                          //
//                                                                    //
// (Input parameter Description)                                      //
//     - argc: number of command-line arguments                       //
//     - argv: array of command-line arguments, where argv[0] is the  //
//             program name and argv[1] (if provided) is the         //
//             server IP address, and argv[2] (if provided) is the   //
//             server port number.                                    //
//                                                                    //
// Output:                                                            //
//     The program sends FTP commands to the server and displays     //
//     the server's response for each command.                        //
//                                                                    //
// Purpose:                                                           //
//   The program converts the commands into ftp commands                //
//    and passes them through the socket to the server.                 //
//    After that, they communicate with each other by                   //
//    outputting the results they receive from the server.              //
//    Enter ctrl+c to shut down the client as soon as they enter ctrl+c.//
// ================================================================= //

typedef int bool;
#define true 1
#define false 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>

#define BUF_SIZE 5012

////ctrl+c ->QUIT fun////
void sh_int(int);
//////////end////////

//Global Variables
int sockfd;


//////////////////////conv_cmd function////////////////////////////
void conv_cmd(char *input, char *output) {
    char *token;
    char *tokens[BUF_SIZE];// Array to store commands and options
    int num_tokens = 0;

        // Copy the input string and tokenize it
    char input_copy[BUF_SIZE];
    strcpy(input_copy, input);

    // Tokenize the input string based on whitespace
    token = strtok(input_copy, " ");
    while (token != NULL && num_tokens < BUF_SIZE - 1) {
        tokens[num_tokens++] = token;
        token = strtok(NULL, " ");
    }
    tokens[num_tokens] = NULL; // Add NULL at the end of the array

    // Command conversion
    if (num_tokens > 0 && strcmp(tokens[0], "ls") == 0) {
        strcpy(output, "NLST");
        // Add options and paths
        for (int i = 1; i < num_tokens; i++) {
            strcat(output, " ");
            strcat(output, tokens[i]);
        }
    } 
    //quit command
    else if (num_tokens > 0 && strcmp(tokens[0], "quit") == 0) {
        strcpy(output, "QUIT");
        // Add options and paths
         for (int i = 1; i < num_tokens; i++) {
            strcat(output, " ");
            strcat(output, tokens[i]);
        }
    } 

    //change dir to LIST
    else if (num_tokens > 0 && strcmp(tokens[0], "dir") == 0){
         strcpy(output, "LIST"); //Copy "LIST" string to buff
           // Outputs from index 2 to argc-1 of the argv array and adds to the buff
            for (int i = 1; i < num_tokens; i++) {
            strcat(output, " ");
            strcat(output, tokens[i]);
            }
    }
    
    else if (num_tokens > 0 && strcmp(tokens[0], "pwd") == 0){
        strcpy(output, "PWD"); //Copy "PWD" string to buff
          // Outputs from index 2 to argc-1 of the argv array and adds to the buff
        for (int i = 1; i < num_tokens; i++) {
            strcat(output, " ");
            strcat(output, tokens[i]); // Add string from current index to buff
        }
    }

    // Change "cd" command to "CWD"
    else if (num_tokens > 0 && strcmp(tokens[0], "cd") == 0 && (num_tokens == 1 || strcmp(tokens[1], "..") != 0)) {
        if (num_tokens == 1) {
            // If only "cd" is entered
            strcpy(output, "CWD");
        } 
        else {
            // If there are arguments along with "cd"
            strcpy(output, "CWD"); // Copy "CWD" string to the buffer
              // Add command and options to the buffer
            for (int i = 1; i < num_tokens; i++) {
                strcat(output, " ");
                strcat(output, tokens[i]); // Add the string at the current index to the buffer
            }
        }
    }   

    //change cd .. to CDUP
    else if (num_tokens > 0 && strcmp(tokens[0], "cd") == 0 && strcmp(tokens[1], "..") == 0) {
        strcpy(output, "CDUP"); //Copy "CDUP" string to buff
        // Outputs from index 3 to argc-1 of the argv array and adds to the buff
        //cd... Since this CDUP should be tied up, from 3
        for (int i = 2; i < num_tokens; i++) { 
            strcat(output, " ");
            strcat(output, tokens[i]); // Add string from current index to buff
        }

    }
    
    //change mldir to MKD
    else if (num_tokens > 0 && strcmp(tokens[0], "mkdir") == 0){
        strcpy(output, "MKD");//Copy "MKD" stirng to buff
        // Outputs from index 2 to argc-1 of the argv array and adds to the buff
        for (int i = 1; i < num_tokens; i++) {
            strcat(output, " ");
            strcat(output, tokens[i]); // Add string from current index to buff
        }
    }

    //change delete to DELE
    else if (num_tokens > 0 && strcmp(tokens[0], "delete") == 0){
        strcpy(output, "DELE");//Copy "DELE" string to buff
        // Outputs from index 2 to argc-1 of the argv array and adds to the buff
        for (int i = 1; i < num_tokens; i++) {
            strcat(output, " ");
            strcat(output, tokens[i]); // Add string from current index to buff
        }
    }    

    //change rmdir to RWD
    else if (num_tokens > 0 && strcmp(tokens[0], "rmdir") == 0){
        strcpy(output, "RMD"); //Copy "RMD" string to buff
        // Outputs from index 2 to argc-1 of the argv array and adds to the buff
        for (int i = 1; i < num_tokens; i++) {
            strcat(output, " ");
            strcat(output, tokens[i]); // Add string from current index to buff
        }
    }

    // change rename to RNFR A RNTO B 
    else if (num_tokens > 0 && strcmp(tokens[0], "rename") == 0) {
        // If the factors are provided correctly
        if (num_tokens == 3) {
            // Form the FTP commands RNFR A RNTO B
            snprintf(output, BUF_SIZE, "RNFR %s RNTO %s", tokens[1], tokens[2]);
        } else {
            // Print error message if incorrect number of arguments provided
            snprintf(output, BUF_SIZE, "%s", input);
        }
    }

    //invalid command
    else {
        // Unsupported command
        strcpy(output, tokens[0]);
        for (int i = 1; i < num_tokens; i++) {
            
            strcat(output, " ");
            strcat(output, tokens[i]);
        }
    }
}
////////////////////////conv_cmd fun END////////////////////////////


///////////////////////main function strat////////////////////////
int main(int argc, char **argv) {
     char buff[BUF_SIZE], ftp_cmd[BUF_SIZE];
    
    int  n;
    struct sockaddr_in serv_addr, cliaddr;

     signal(SIGINT, sh_int);

     // Error handling for insufficient arguments
    if (argc <3) {
        write(1,"less argument\n",strlen("less argument\n"));
        exit(1); 
    }

    // Socket creation
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Server address configuration
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]); // Server address
    serv_addr.sin_port = htons(atoi(argv[2])); // Server port

    // Connecting to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
        perror("connection with server failed");
        exit(EXIT_FAILURE);
    }


    // User input and command processing
    while (1) {
    write(STDOUT_FILENO, "> ", 2);
    fgets(buff, BUF_SIZE, stdin);

      // Check if input buffer is empty
        if (strlen(buff) == 1) {
            // Empty input buffer, ignore and continue
            continue;
        }
         // Check if input buffer contains only space characters
        bool is_all_spaces = true;
        for (int i = 0; i < strlen(buff); i++) {
            if (buff[i] != ' ' && buff[i] != '\n' && buff[i] != '\0') {
                is_all_spaces = false;
                break;
            }
        }

        // If input buffer contains only space characters, print error and continue
        if (is_all_spaces) {
            printf("Error: Invalid input. Please enter a valid command.\n");
            continue;
        }
        
    buff[strcspn(buff, "\n")] = '\0'; // Remove newline character

    // Convert command to FTP command
    conv_cmd(buff, ftp_cmd);

    // Send converted command to the server
    n = write(sockfd, ftp_cmd, strlen(ftp_cmd));
    if (n <= 0) {
        perror("write error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Receive response from the server
    n = read(sockfd, buff, BUF_SIZE);
    if (n <= 0) {
        perror("read error or server closed connection");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    buff[n] = '\0';

    // Print the received response
    write(STDOUT_FILENO, buff, strlen(buff));

}

// Close socket
close(sockfd);
}
/////////////////////main fun END/////////////////////////////


///////////ctrl+c fun/////////////
void sh_int(int signum){
    //sockfd =QUIT
    write(sockfd, "QUIT", strlen("QUIT"));
    printf("\n");
    //exit
    exit(0);
}
///////////end/////////////
