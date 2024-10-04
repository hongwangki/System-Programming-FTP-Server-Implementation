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
//     This program implements a simple FTP client that connects    //
//     to a server specified by IP address and port number. It      //
//     sends user input to the server and displays the response    //
//     received from the server.                                    //
/////////////////////////////////////////////////////////////////////
// Function: main,sh_chld,sh_alrm,   client_info                    //
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
//      also Entering QUIT shuts down the child                     //
//       process and fails to respond to the client                 //
// ================================================================= //


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>

#define BUF_SIZE 256

////////////////// Client Information Output Function//////////////////
int client_info(struct sockaddr_in *client_addr) {
    //ip inet_nota use
    char *ip = inet_ntoa(client_addr->sin_addr);
    //port ntohs use
    uint16_t port = ntohs(client_addr->sin_port);

   // Make client information a string
    char output[256]; // Maximum size of the output string
    int len = snprintf(output, sizeof(output), "==========Client info===========\nclient IP: %s\nclient port: %hu\n================================\n", ip, port);

    // Use string as standard output
    write(STDOUT_FILENO, output, len);

    return 0; // success return 
}
////////////////// Client_info fun END////////////////////////////////////

void sh_chld(int); // signal handler for SIGCHLD
void sh_alrm(int); // signal handler for SIGALRM

int main(int argc, char ** argv){

char buff[BUF_SIZE];
int n;
struct sockaddr_in server_addr, client_addr;
int server_fd, client_fd;
int len;
int port;
// Set signal handlers
signal(SIGALRM, sh_alrm);
signal(SIGCHLD, sh_chld);

// Create a socket
server_fd = socket(PF_INET, SOCK_STREAM, 0);

// Initialize server address structure
memset(&server_addr, 0, sizeof(server_addr));
server_addr.sin_family=AF_INET;
server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
server_addr.sin_port=htons(atoi(argv[1]));

// Bind the socket to the server address
bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
// Listen for incoming connections
listen(server_fd, 5);

// Accept Client Connections
    // Accept incoming client connections in a loop
    while(1){
        
        pid_t pid;
        len=sizeof(client_addr);
        // Accept client connection and obtain the file descriptor for communication
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
        if (client_fd < 0) {
            // Print error message if accepting client connection fails
            perror("server accept failed");
            exit(EXIT_FAILURE); // Exit the program
        }
        /// Create a child process to handle communication with the client
        pid = fork();
        if (pid < 0) {
            perror("fork failed"); // Print error message if fork fails
            exit(EXIT_FAILURE);  // Exit the program
        }

       if (pid == 0) {  // child process
               // Print Client Information
            // Print information about the connected client
            if (client_info(&client_addr) < 0) {
                write(STDERR_FILENO, "client_info() err!!\n", sizeof("client_info() err!!\n"));
            }

             char C_buff[BUF_SIZE]; //Child buff
             sprintf(C_buff, "Child Process ID : %d\n", getpid());
             write(STDOUT_FILENO, C_buff, strlen(C_buff));

            while (1) {
                //Save the results received from the server to a buffer
                n = read(client_fd, buff, BUF_SIZE);
                //if read fail
                if (n <= 0) {
                    perror("read error or client closed connection");
                    close(client_fd);
                    exit(EXIT_SUCCESS);  // child exits
                }
                buff[n] = '\0';
                //if buff!="QUIT"
                if(strcmp(buff, "QUIT\n") != 0){
                    //Forwarding Results to Clients
                n = write(client_fd, buff, BUF_SIZE);
                    //Forwarding Results to Clients ->fail
                    if (n < 0) {
                    perror("write error");
                    close(client_fd);
                    exit(EXIT_FAILURE);  // child exits
                    }
                }


                //buff=QUIT
                if(strcmp(buff, "QUIT\n") == 0){
                    alarm(1);
                }
            }
        } 
        else {  // parent process
            
            
        }
        close(client_fd);
    } 
    return 0;
}
//////////////////////main fun END/////////////////////////////

/////////////////////signal fun////////////////////////////////
void sh_chld(int signum){
    printf("Status of Child process was changed.\n");
    wait(NULL);
}
void sh_alrm(int signum){
    printf("Child Process(PID: %d) will be terminated.\n",getpid());
    exit(1);
}
////////////////////signal fun end/////////////////////////////