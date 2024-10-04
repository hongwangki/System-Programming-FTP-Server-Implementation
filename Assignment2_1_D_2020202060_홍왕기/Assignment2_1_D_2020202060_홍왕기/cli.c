/////////////////////////////////////////////////////////////////////
// File Name : cli.c                                               //
// Date : 2024/04/16                                               //
// OS : Ubuntu 20.04.6 LTS 64bits                                  //
//                                                                 //
// Author : Hong Wang ki                                           //
// Student ID : 2020202060                                         //
// ---------------------------------------------------------------- //
// Title : System Programming Assignment #2-1 ftp server           //
// Description: This program switches Linux commands to FTP commands//
//              and sends them to the server.                      //
/////////////////////////////////////////////////////////////////////

// Function: main conv_cmd
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
//     This program establishes a connection with an FTP server,      //
//     sends commands entered by the user, and prints the server's    //
//     response for each command. It converts Linux commands to FTP   //
//     commands before sending them to the server.                    //
// ================================================================= //



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#define MAX_BUFF 5012

//////////////////////conv_cmd function////////////////////////////
void conv_cmd(char *input, char *output) {
    char *token;
    char *tokens[MAX_BUFF];// Array to store commands and options
    int num_tokens = 0;

        // Copy the input string and tokenize it
    char input_copy[MAX_BUFF];
    strcpy(input_copy, input);

    // Tokenize the input string based on whitespace
    token = strtok(input_copy, " ");
    while (token != NULL && num_tokens < MAX_BUFF - 1) {
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
    int sockfd, n;
    struct sockaddr_in servaddr, cliaddr;
    char buff[MAX_BUFF], ftp_cmd[MAX_BUFF];

     // Error handling for insufficient arguments
    if (argc == 1 ||argc==2) {
        write(1,"less argument\n",strlen("less argument\n"));
        exit(1); 
    }
    //Error argument  too many case
    if(argc>3){
        write(1,"too many argument\n",strlen("too many argument\n"));
        exit(1);
    }

    // Socket creation
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Server address configuration
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]); // Server address
    servaddr.sin_port = htons(atoi(argv[2])); // Server port

    // Connecting to the server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        perror("connection with server failed");
        exit(EXIT_FAILURE);
    }


    // User input and command processing
    while (1) {
        write(1,"> ",strlen("> "));
        //printf("> ");
        fgets(buff, MAX_BUFF, stdin);
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
        n = read(sockfd, buff, MAX_BUFF);
        if (n <= 0) {
            perror("read error or server closed connection");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        buff[n] = '\0';
         // Print the received response
        write(1,buff,strlen(buff));

        // Handle QUIT command
        if (!strcmp(buff, "Program quit!!\n")) {
           break;
        }
    }
    //close socket
    close(sockfd);
}
/////////////////////main fun END/////////////////////////////