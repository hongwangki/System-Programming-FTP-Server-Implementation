/////////////////////////////////////////////////////////////////////
// File Name : cli.c                                               //
// Date : 2024/05/16                                               //
// OS : Ubuntu 20.04.6 LTS 64bits                                  //
//                                                                 //
// Author : Hong Wang ki                                           //
// Student ID : 2020202060                                         //
// ---------------------------------------------------------------- //
// Title : System Programming Assignment #3-2 ftp server            //
// Description:                                                     //
//    This program implements an FTP server that handles basic      //
//    FTP commands and manages data connections to send directory   //
//    listings to the client.                                       //
// ---------------------------------------------------------------- //
// Function: main  list_directory   convert_str_to_addr             //
// ================================================================= //
// Input:                                                           //
//    - argv[1]: Server port number (integer)                       //
// Output:                                                          //
//    - Directory listing to the client through data connection     //
// Purpose:                                                         //
//    - Create a control connection and handle FTP commands from    //
//      the client. Establish data connections as needed for        //
//      transferring directory listings.                            //
// ================================================================= //
///////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_BUFF 1500
// Variable to count directory paths
   int path_count = 0;

////// Function to list the contents of a directory/////////
void list_directory(int data_conn, const char *path) {

    

    // Check if the first character of the path is '-'
    if (path[0] == '-') {
        // Invalid option error
        write(1, "550 Invalid option.\n", strlen("550 Invalid option.\n"));
        write(data_conn, "550 Invalid option.\n", strlen("550 Invalid option.\n"));
        return;
    }
    // Check if multiple paths are provided
    if (path_count > 1) {
        write(1, "501 Multiple directory paths are not supported.\n", strlen("501 Multiple directory paths are not supported.\n"));
        write(data_conn, "501 Multiple directory paths are not supported.\n", strlen("501 Multiple directory paths are not supported.\n"));
        return;
    }


    struct stat path_stat;
    // Check if the path is valid
    if (stat(path, &path_stat) != 0) {
        // Failed to get file status
        write(1, "550 Failed to get directory information.\n", strlen("550 Failed to get directory information.\n"));
        write(data_conn, "550 Failed to get directory information.\n", strlen("550 Failed to get directory information.\n"));
        return;
    }
     if (access(path, R_OK) != 0) {
        // Access to the path is denied
        write(data_conn, "550 Access to the path is denied.\n", strlen("550 Access to the path is denied.\n"));
        write(1, "550 Access to the path is denied.\n", strlen("550 Access to the path is denied.\n"));
        return ;
    }

    // Check if the path is a directory
    if (S_ISDIR(path_stat.st_mode)) {
        // Open the directory
        DIR *d = opendir(path);
        if (d) {
            struct dirent *dir;
            char buffer[MAX_BUFF];
            memset(buffer, 0, sizeof(buffer)); // Clear buffer
            int is_empty = 1; // Flag to check if the directory is empty

            // Read directory contents and save to buffer
            while ((dir = readdir(d)) != NULL) {
                if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                    strcat(buffer, dir->d_name);
                    strcat(buffer, "\n");
                    is_empty = 0; // Directory is not empty
                }
            }
            closedir(d);

            // If directory is empty, send appropriate message
            if (is_empty) {
                strcpy(buffer, "Directory is empty.\n");
            }

            // Write buffer to data connection
            write(data_conn, buffer, strlen(buffer));
        }
        else {
            
            // Failed to open directory
            write(data_conn, "550 Failed to open directory.\n", strlen("550 Failed to open directory.\n"));
            write(1, "550 Failed to open directory.\n", strlen("550 Failed to open directory.\n"));
        }
    } else {

        // Check if access to the path is denied
        if (access(path, R_OK) != 0) {
        // Access to the path is denied
        write(data_conn, "550 Access to the path is denied.\n", strlen("550 Access to the path is denied.\n"));
        write(1, "550 Access to the path is denied.\n", strlen("550 Access to the path is denied.\n"));
        }
        // Path is not a directory
        write(data_conn, "550 Specified path is not a directory.\n", strlen("550 Specified path is not a directory.\n"));
        write(1, "550 Specified path is not a directory.\n", strlen("550 Specified path is not a directory.\n"));
    }

    // Check if access to the path is denied
    if (access(path, R_OK) != 0) {
        // Access to the path is denied
        write(data_conn, "550 Access to the path is denied.\n", strlen("550 Access to the path is denied.\n"));
        write(1, "550 Access to the path is denied.\n", strlen("550 Access to the path is denied.\n"));
    }
}
//////////////////////list_directory fun end//////////////////////////



// Function to convert IP address and port number from FTP PORT command format
char* convert_str_to_addr(const char *str, unsigned int *port) {
    static char addr[32];
    unsigned int ip[4]; // Changed to unsigned int array
     unsigned int p1, p2; // Changed to unsigned int
    //printf("str: %s\n",str);
    // Corrected format specifier to match the type of ip array
    sscanf(str, "PORT %u,%u,%u,%u,%d,%d", &ip[0], &ip[1], &ip[2], &ip[3], &p1, &p2);

    // Using snprintf to prevent buffer overflow
    snprintf(addr, sizeof(addr), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    *port = (p1 << 8) | p2;
    // printf("p1 p2 : %d %d\n", p1,p2); 

    return addr;
}
//////////////////////convert_str_to_addr fun end//////////////////////////


/////////////////////main////////////////////////////
int main(int argc, char **argv) {
    int sockfd, connfd, len, data_sockfd = -1, data_conn;
    struct sockaddr_in servaddr, cliaddr, data_servaddr;
    char buff[MAX_BUFF], cmd[MAX_BUFF];
    char *host_ip;
    unsigned int port_num;

    // Check for correct usage
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
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
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    // Binding the socket
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        perror("socket bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Listening for incoming connections
    if (listen(sockfd, 5) != 0) {
        perror("listen failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Accept an incoming connection
    len = sizeof(cliaddr);
    connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len);
    if (connfd < 0) {
        perror("server accept failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        memset(buff, 0, MAX_BUFF);
        read(connfd, buff, MAX_BUFF);
        write(1,buff,strlen(buff));
        write(1,"\n",1);

        //if buff = port -> port cmd change
        //data connection make and success message send to cli
        if (strncmp(buff, "PORT", 4) == 0) {
            if (data_sockfd != -1) {
                close(data_sockfd);
            }
            host_ip = convert_str_to_addr(buff, &port_num);

            // Create data connection socket
            data_sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (data_sockfd == -1) {
                perror("data socket creation failed");
                close(connfd);
                close(sockfd);
                exit(EXIT_FAILURE);
            }

            // Server address configuration for data connection
            memset(&data_servaddr, 0, sizeof(data_servaddr));
            data_servaddr.sin_family = AF_INET;
            data_servaddr.sin_addr.s_addr = inet_addr(host_ip);
            data_servaddr.sin_port = htons(port_num);

            // Connecting to the client
            if (connect(data_sockfd, (struct sockaddr *)&data_servaddr, sizeof(data_servaddr)) != 0) {
                perror("data connection failed");
                close(data_sockfd);
                close(connfd);
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            // Acknowledge the PORT command
            write(1, "200 Port command successful\n", 28);
            write(connfd, "200 Port command successful\n", 28);
            
        } 
        //if buff =nlst -> ls information send to cli
       else if (strncmp(buff, "NLST", 4) == 0 ) {
            if (data_sockfd == -1) {
                write(connfd, "425 No data connection established\n", 35);
                continue;
            }

              // Extract directory path from buff
            char *directory_path = NULL;
            char *nlst_token = strtok(buff, " ");  // First token after NLST
            nlst_token = strtok(NULL, ""); // Rest of the string (directory path)
            if (nlst_token != NULL) {
                directory_path = strdup(nlst_token);
                // Check for multiple paths (more than one space-separated path)
                char *token = strtok(directory_path, " ");
                path_count=0;
                while (token != NULL) {
                    path_count++;
                    token = strtok(NULL, " ");
                }
                } 
            else {
                // No directory path provided, use current directory
                directory_path = strdup(".");
            }
            // Acknowledge the NLST command
            write(1, "150 Opening data connection for directory list.\n", 48);
            write(connfd, "150 Opening data connection for directory list.\n", 48);

            // Send directory listing to the client
            list_directory(data_sockfd, directory_path);

            read(data_sockfd,"",1);

            // Notify client of successful transfer
            write(1, "226 Complete transmission.\n", strlen("226 Complete transmission.\n"));
            write(connfd, "226 Complete transmission.\n", strlen("226 Complete transmission.\n"));
            // Close the data connection
            close(data_sockfd);
            data_sockfd = -1;
            //break;
        } 
        else if (strncmp(buff, "QUIT", 4) == 0) {
            char *token = strtok(buff, " ");
            token = strtok(NULL, " ");

            if (token != NULL) {
                write(connfd, "500 Syntax error, command unrecognized.\n", 40);
            } else {
                write(1, "221 Goodbye.\n", 13);
                write(connfd, "221 Goodbye.\n", 13);
                break;
            }
        }
        else {
            write(1, "500 Syntax error, command unrecognized.\n", 41);
            write(connfd, "500 Syntax error, command unrecognized.\n", 41);
            //break;
        }
    }
    // Close connections
    close(connfd);
    close(sockfd);
    return 0;
}