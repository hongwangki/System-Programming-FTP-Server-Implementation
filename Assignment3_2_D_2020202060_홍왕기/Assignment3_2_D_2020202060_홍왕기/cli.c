//////////////////////////////////////////////////////////////////////
// File Name : cli.c                                               //
// Date : 2024/05/25                                               //
// OS : Ubuntu 20.04.6 LTS 64bits                                  //
//                                                                 //
// Author : Hong Wang ki                                           //
// Student ID : 2020202060                                         //
// ---------------------------------------------------------------- //
// Title : System Programming Assignment #3-2 ftp client           //
// Description: This program implements a simple FTP client that    //
//              connects to an FTP server, sends commands, and      //
//              receives data. It handles both control and data     //
//              connections using sockets.                          //
//////////////////////////////////////////////////////////////////////
// Function: main, conv_cmd ,convert_str_to_addr                    //
// ================================================================= //
// Input:                                                           //
//     - Command line arguments:                                    //
//         argv[1]: Server IP address                               //
//         argv[2]: Server port number                              //
//                                                                    //
// Output:                                                          //
//     - Sends FTP commands to the server and prints server         //
//       responses to the standard output.                          //
//     - Establishes a data connection for file listing (NLST).     //
//                                                                    //
// Purpose:                                                         //
//     - To demonstrate the use of socket programming in C for      //
//       implementing an FTP client that can interact with an FTP   //
//       server, send commands, and handle file listings.           //
//////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#define MAX_BUFF 1500

/////// Function to convert user input commands to FTP commands////////
void conv_cmd(char *input, char *output) {
    char *token;
    char *tokens[MAX_BUFF];
    int num_tokens = 0;
    char input_copy[MAX_BUFF];

    // Copy input to avoid modifying the original string
    strcpy(input_copy, input);
     // Tokenize input string by spaces
    token = strtok(input_copy, " ");
    while (token != NULL && num_tokens < MAX_BUFF - 1) {
        tokens[num_tokens++] = token;
        token = strtok(NULL, " ");
    }
    tokens[num_tokens] = NULL;

     // Convert 'ls' command to 'NLST' for FTP
    if (num_tokens > 0 && strcmp(tokens[0], "ls") == 0) {
        strcpy(output, "NLST");
        for (int i = 1; i < num_tokens; i++) {
            strcat(output, " ");
            strcat(output, tokens[i]);
        }
    }
    else if (strcmp(tokens[0], "quit") == 0) {
            strcpy(output, "QUIT");
            for (int i = 1; i < num_tokens; i++) {
            strcat(output, " ");
            strcat(output, tokens[i]);
        }
        }
     else {
        strcpy(output, tokens[0]);
        for (int i = 1; i < num_tokens; i++) {
            strcat(output, " ");
            strcat(output, tokens[i]);
        }
    }
}
/////////////////conv cmd fun end///////////////////////

//// Function to convert string IP:PORT format to FTP PORT command format////
char* convert_str_to_addr(char *str, unsigned int *port) {
    static char addr[32]; // Increased size to accommodate the entire address including "PORT "
    char copystr[MAX_BUFF];
    char ip_part[4][4];
    int ip[4], p1, p2;

    // Make a copy of the input string
    snprintf(copystr, sizeof(copystr), "%s", str);
   // printf("Original input: %s\n", copystr);

    // Parse the input string to extract the IP and port parts
    sscanf(copystr, "%[^,],%[^,],%[^,],%[^,],%d,%d", ip_part[0], ip_part[1], ip_part[2], ip_part[3], &p1, &p2);
    for (int i = 0; i < 4; i++) {
        ip[i] = atoi(ip_part[i]);
    }
    //printf("Parsed parts: %d.%d.%d.%d, portnum: %d,%d\n", ip[0], ip[1], ip[2], ip[3], p1, p2);

    // Convert IP parts to a comma-separated format and add the "PORT " prefix
    snprintf(addr, sizeof(addr), "PORT %d,%d,%d,%d,%d,%d", ip[0], ip[1], ip[2], ip[3], p1, p2);
    //printf("Formatted address: %s\n", addr);

    // Combine port parts into a single port number
    *port = (p1 << 8) | p2;

    return addr;
}
////////////////// convert_str_to_addr fun end/////////////////////


////////////////////////////main///////////////////////////////////
int main(int argc, char **argv) {
    int sockfd, data_sockfd, data_conn, n, port;
    struct sockaddr_in servaddr, cliaddr, data_servaddr;
    char buff[MAX_BUFF], ftp_cmd[MAX_BUFF], temp[MAX_BUFF], *host_ip;
    socklen_t len;
    unsigned int port_num;

    //if less or many argument
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Socket creation for control connection
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Server address configuration
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(atoi(argv[2]));

    // Connecting to the server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        perror("connection with server failed");
        exit(EXIT_FAILURE);
    }

    // User input and command processing
    while (1) {
        printf("> ");
        fgets(buff, MAX_BUFF, stdin);
        buff[strcspn(buff, "\n")] = '\0';
        // Convert user input command to FTP command
        conv_cmd(buff, ftp_cmd);

        // Handle NLST command which requires data connection
        if (strncmp(ftp_cmd, "NLST", 4) == 0) {
            // Create a socket for data connection
            data_sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (data_sockfd == -1) {
                perror("data socket creation failed");
                exit(EXIT_FAILURE);
            }

            // Generate a random port number for data connection
            srand(time(NULL));
            port = rand() % 20000 + 10001;

            // Client address configuration for data connection
            memset(&cliaddr, 0, sizeof(cliaddr));
            cliaddr.sin_family = AF_INET;
            cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);
            cliaddr.sin_port = htons(port);

            // Bind the data socket to the generated port
            if (bind(data_sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) != 0) {
                perror("bind failed");
                close(data_sockfd);
                exit(EXIT_FAILURE);
            }

            // Listen for incoming connections on the data socket
            listen(data_sockfd, 1);

            // Send PORT command to the server
            len = sizeof(cliaddr);
            getsockname(data_sockfd, (struct sockaddr *)&cliaddr, &len);

            // Convert the IP address from "x.x.x.x" to "x,x,x,x" format
            memset(temp, 0, sizeof(temp)); // Clear buffer
            strcat(temp,argv[1]);
            //printf("temp: %s\n",temp);
            for (int i = 0; temp[i] != '\0'; i++) {
                if (temp[i] == '.') {
                    temp[i] = ',';
                }
            }   
            snprintf(temp + strlen(temp), sizeof(temp) - strlen(temp), ",%d,%d", (port >> 8) & 0xFF, port & 0xFF);

            // Convert string to FTP PORT command format and send to server
            host_ip = convert_str_to_addr(temp, (unsigned int *) &port_num);
            write(sockfd, host_ip, strlen(host_ip));
             memset(buff, 0, sizeof(buff)); // Clear buffer
            read(sockfd, buff, MAX_BUFF);
            write(1,buff,strlen(buff));
           // write(1,"\n",1);

            // Send NLST command to the server
            write(sockfd, ftp_cmd, strlen(ftp_cmd));
            memset(buff, 0, sizeof(buff)); // Clear buffer
            read(sockfd, buff, MAX_BUFF);
            write(1,buff,strlen(buff));

            // Accept the data connection
            len = sizeof(data_servaddr);
            data_conn = accept(data_sockfd, (struct sockaddr *)&data_servaddr, &len);
            if (data_conn < 0) {
                perror("server accept failed");
                close(data_sockfd);
                exit(EXIT_FAILURE);
            }

            size_t data_size;
            // Receive data from server
           memset(buff, 0, sizeof(buff)); // Clear buffer
            n = read(data_conn, buff, MAX_BUFF);
            if (n < 0) {
                perror("data read error");
                exit(1);
            } else {
                 data_size = (size_t)n;
                //buff[n] = '\0';
                write(1,buff,strlen(buff));
                //write(1,"\n",1);
            }
            write(data_conn,"",1);

            // Clear buffer and close data connection
            memset(buff, 0, sizeof(buff)); 
            n= read(sockfd,buff,MAX_BUFF);
            
            write(1,buff,strlen(buff));
            char output_buff[MAX_BUFF];
            int bytes_written = snprintf(output_buff, MAX_BUFF, "OK. %zu bytes is received.\n", data_size);
            write(1, output_buff, bytes_written);
            close(data_conn);
            close(data_sockfd);
        } else {
              // Handle other FTP commands
            n = write(sockfd, ftp_cmd, strlen(ftp_cmd));
            if (n <= 0) {
                perror("write error");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
             memset(buff, 0, sizeof(buff)); // Clear buffer
            n = read(sockfd, buff, MAX_BUFF);
            if (n <= 0) {
                perror("read error or server closed connection");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            buff[n] = '\0';
            write(1,buff,strlen(buff));
            write(1,"\n",1);
            // If the command was QUIT, break the loop and exit
            if (strcmp(ftp_cmd, "QUIT") == 0) {
                 memset(buff, 0, sizeof(buff)); 
                 n = read(sockfd, buff, MAX_BUFF);
                break;
            }
        }
    }
     // Close control connection socket
    close(sockfd);
    return 0;
}