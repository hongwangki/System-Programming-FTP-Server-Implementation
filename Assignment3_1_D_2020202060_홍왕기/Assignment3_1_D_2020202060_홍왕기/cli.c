/////////////////////////////////////////////////////////////////////
// File Name : cli.c                                               //
// Date : 2024/05/16                                               //
// OS : Ubuntu 20.04.6 LTS 64bits                                  //
//                                                                 //
// Author : Hong Wang ki                                           //
// Student ID : 2020202060                                         //
// ---------------------------------------------------------------- //
// Title : System Programming Assignment #3-1 ftp server            //
// Description: Project on ID and password by                       //
//              communicating with server through socket            //
/////////////////////////////////////////////////////////////////////
// Function: main log_in                                            //
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
//    success or fail                                                 //
//                                                                    //
// Purpose:                                                           //
//          The client sends the ID and password entered              //
//             by the user to the server,                             //
//          The server receives this information and                  //
//       finds the matching ID and password                          //
//           in the stored password fileI'll certify it.             //
// ================================================================= //

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define MAX_BUF 100 //max buf
#define CONT_PORT 20001

//////////////////// Function to handle user login///////////////////////////
void log_in(int sockfd) {
    int n;
    char user[MAX_BUF], *passwd, buf[103];
    int attempts = 0;

    while (1) {
        // Function to handle user login
        printf("Input ID : ");
        fgets(user, MAX_BUF, stdin);
        user[strcspn(user, "\n")] = '\0'; // Remove trailing newline
        // Prompt for password (getpass hides the input)
        passwd = getpass("Input Password : ");

        // Combine username and password into one string
        snprintf(buf, 103, "%s:%s\n", user, passwd);

        // Send username and password to the server
        write(sockfd, buf, strlen(buf));
       // printf("buf user passwd: %s\n",buf);

        // Read response from server
        n = read(sockfd, buf, MAX_BUF);
        buf[n] = '\0';
        if (!strcmp(buf, "OK")) {
             // Send an empty string to the server (confirming login)
            write(sockfd,"", 1);
            n=read(sockfd, buf, MAX_BUF);
            buf[n]='\0';
             // If server confirms login
            if (strcmp(buf, "OK") == 0) {
                printf("** User ['%s'] logged in **\n", user);
                return;
            // If login failed
            } else if (strcmp(buf, "FAIL") == 0) {
                printf("** Log-in failed **\n");
              // If server closed the connection
            } else if (strcmp(buf, "DISCONNECTION") == 0) {
                printf("** Connection closed **\n");
                close(sockfd);
                exit(0);
                // If the server response is unknow
            } else {
                printf("** Unknown response from server **\n");
            }
        }
    }

    close(sockfd);
    exit(0);
}
//////////////////////login fun end///////////////////////

///////////////////////main start/////////////////////////
int main(int argc, char *argv[]) {
    int sockfd, n;
    struct sockaddr_in servaddr;
    char buf[MAX_BUF];
    // Check for correct number of arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP address> <Port number>\n", argv[0]);
        exit(1);
    }
     // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }
     // Initialize server address structure
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(atoi(argv[2]));
     // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd);
        exit(1);
    }

    // Read initial response from server
    n = read(sockfd, buf, MAX_BUF);
    buf[n] = '\0';
    // If server rejects the connection
    if (!strcmp(buf, "REJECTION")) {
        printf("** Connection refused **\n");
        close(sockfd);
        exit(0);
    // If server accepts the connection
    } else if (!strcmp(buf, "ACCEPTED")) {
         printf("** It is connected to Server **\n");
        log_in(sockfd);
    }
     // Close the socket
    close(sockfd);
    return 0;
}
///////////////////////main end/////////////////////////