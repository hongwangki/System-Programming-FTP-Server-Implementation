/////////////////////////////////////////////////////////////////////
// File Name : cli.c                                               //
// Date : 2024/05/05                                               //
// OS : Ubuntu 20.04.6 LTS 64bits                                  //
//                                                                 //
// Author : Hong Wang ki                                           //
// Student ID : 2020202060                                         //
// ---------------------------------------------------------------- //
// Title : System Programming Assignment #2-2 ftp server           //
// Description:                                                    //
//      This program acts as a client for a simple FTP server.      //
//      It connects to the server specified by IP address and port  //
//      number, sends user input to the server, and displays the    //
//      response received from the server.                          //
/////////////////////////////////////////////////////////////////////
// Function: main                                                  //
// ================================================================= //
// Input:                                                           //
//      argv[1]: Server IP address                                  //
//      argv[2]: Server port number                                 //
//                                                                 //
// (Input parameter Description)                                    //
//      argv[1]: IP address of the server to connect to             //
//      argv[2]: Port number of the server to connect to            //
//                                                                 //
// Output:                                                          //
//                                                                 //
// (Return value)                                                   //
//      Returns 0 upon successful execution, non-zero otherwise      //
//                                                                 //
// Purpose:                                                         //
//      This function is the entry point of the program. It sets    //
//      up a connection to the server, sends user input to the      //
//      server, and displays the response received from the server. //
// ================================================================= //

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#define BUF_SIZE 256 //Max BUF SIZE

//////////////////////////////main function//////////////////////////
int main(int arg, char **argv){
    char buff[BUF_SIZE];

    int n; 
    int sockfd;//Server socket discriptor
    struct sockaddr_in serv_addr; 
    sockfd = socket(AF_INET, SOCK_STREAM,0) ;  // Create a socket for the client
    memset(&serv_addr, 0, sizeof(serv_addr));  // Initialize memory for server address
    serv_addr.sin_family=AF_INET;  // Set the address family to IPv4
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);  // Set the server IP address
    serv_addr.sin_port=htons(atoi(argv[2]));  // Set the server port

    
    // Connect to the server
    connect (sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

     // Start communication loop
    while (1) {
        write(STDOUT_FILENO, "> ", 2); // Display prompt
        n= read (STDIN_FILENO, buff, BUF_SIZE);  // Read input from user
        buff[n] = '\0'; // Null-terminate the string

        // Send the input to the server
        if(write(sockfd, buff, BUF_SIZE) > 0){
            // Receive response from server
            if((n=read(sockfd, buff, BUF_SIZE))>0){ 
                buff[n] = '\0';  // Null-terminate the string
                printf("from server:%s" ,buff); // Print server's response
            }
            else
                // Break the loop if there's an error or connection is closed
                break;
        }
        else
            // Break the loop if there's an error or connection is closed
            break;
    }
    close (sockfd) ;  // Close the socket
    return 0; // Exit the program
    
}
/////////////////////////////main fun END//////////////////////////////////