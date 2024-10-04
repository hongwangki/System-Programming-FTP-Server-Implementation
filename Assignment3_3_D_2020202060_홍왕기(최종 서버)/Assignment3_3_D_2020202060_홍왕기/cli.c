/////////////////////////////////////////////////////////////////////
// File Name : cli.c                                               //
// Date : 2024/06/01                                               //
// OS : Ubuntu 20.04.6 LTS 64bits                                  //
//                                                                 //
// Author : Hong Wang ki                                           //
// Student ID : 2020202060                                         //
// ---------------------------------------------------------------- //
// Title : System Programming Assignment #3-3 ftp server           //
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
#include <time.h>

#define BUF_SIZE 5012
#define MAX_BUF 50

//Global Variables
int sockfd;
char getfilename[50];
int CheakList=0;
char TypeChoose[1]={'I'};
char PutBuff[BUF_SIZE];
size_t bytesRead2;

void SendPUT(char *fileName, char type) {
    FILE *file;

    if (type == 'A') {
        // ASCII mode file open
        file = fopen(fileName, "r");
    } else if (type == 'I') {
        // BINARY mode file open
        file = fopen(fileName, "rb");
    }
    else {
        //err case 
        sprintf(PutBuff, "550: Unknown type '%c'. Use 'A' for ASCII or 'I' for binary.\n", type);
        return;
    }

    // fule not exist case
    if (file == NULL) {
        sprintf(PutBuff, "550: File '%s' not found.\n", fileName);
        return;
    }

    // file information -> putbuff
    bytesRead2 = fread(PutBuff, 1, BUF_SIZE, file);
    // fread err case
    if (ferror(file)) {
        sprintf(PutBuff, "550 reading file '%s'.\n", fileName);
        bytesRead2 = 0;
    }

    // close file
    fclose(file);
}


//// Function to convert string IP:PORT format to FTP PORT command format////
char* convert_str_to_addr(char *str, unsigned int *port) {
    static char addr[32]; // Increased size to accommodate the entire address including "PORT "
    char copystr[MAX_BUF];
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
    else if (num_tokens > 0 && strcmp(tokens[0], "get") == 0) {
        strcpy(output, "RETR");
        // Add options and paths
         for (int i = 1; i < num_tokens; i++) {
            strcat(output, " ");
            strcat(output, tokens[i]);
        }
        // getfilename <- token[1] copy
        if(tokens[1]!=NULL){
        strcpy(getfilename, tokens[1]);
        if(tokens[2]!=NULL){
            CheakList=0;
        }
        }
        else{
            CheakList=1;
        }
    } 
    else if (num_tokens > 0 && strcmp(tokens[0], "put") == 0) {
        strcpy(output, "STOR");
        // Add options and paths
         for (int i = 1; i < num_tokens; i++) {
            strcat(output, " ");
            strcat(output, tokens[i]);
        }
        if(num_tokens==2){
            SendPUT(tokens[1],TypeChoose[0]);
        }
        else{
            strcpy(PutBuff,"550 Error PUT command");
        }
    }
    else if (num_tokens > 0 && strcmp(tokens[0], "bin") == 0 ) {
        strcpy(output, "TYPE I");
        // Add options and paths
         for (int i = 1; i < num_tokens; i++) {
            strcat(output, " ");
            strcat(output, tokens[i]);
        }
        if(num_tokens==1) TypeChoose[0]='I';
    }  
    else if (num_tokens > 0 && strcmp(tokens[0], "type") == 0 && num_tokens > 1 && strcmp(tokens[1], "binary") == 0) {
            strcpy(output, "TYPE I");
            // Add options and paths
            for (int i = 2; i < num_tokens; i++) {
                strcat(output, " ");
                strcat(output, tokens[i]);
            }
             if(num_tokens==2) TypeChoose[0]='I';

    } 

    else if (num_tokens > 0 && strcmp(tokens[0], "ascii") == 0 ) {
        strcpy(output, "TYPE A");
        // Add options and paths
         for (int i = 1; i < num_tokens; i++) {
            strcat(output, " ");
            strcat(output, tokens[i]);
        }
        if(num_tokens==1) TypeChoose[0]='A';
    }  
    else if (num_tokens > 0 && strcmp(tokens[0], "type") == 0 && num_tokens > 1 && strcmp(tokens[1], "ascii") == 0) {
            strcpy(output, "TYPE A");
            // Add options and paths
            for (int i = 2; i < num_tokens; i++) {
                strcat(output, " ");
                strcat(output, tokens[i]);
            }
            if(num_tokens==2) TypeChoose[0]='A';
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
    int data_sockfd, data_conn, port;
    int  n;
    char  temp[MAX_BUF], *host_ip;
    struct sockaddr_in serv_addr, cliaddr,data_servaddr;
    socklen_t len;
    unsigned int port_num;

    size_t  size1;
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
    write(STDOUT_FILENO, "ftp> ", strlen("ftp> "));
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


      if (strncmp(ftp_cmd, "NLST", 4) == 0 || strncmp(ftp_cmd,"RETR",4)==0 || strncmp(ftp_cmd,"STOR",4)==0 || strncmp(ftp_cmd,"LIST",4)==0) {
            // Create a socket for data connection
            data_sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (data_sockfd == -1) {
                perror("data socket creation failed");
                exit(EXIT_FAILURE);
            }

            // Generate a random port number for data connection
            srand(time(NULL));
            port = rand() % 50000 + 10001;

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
            strcat(temp, argv[1]);
            for (int i = 0; temp[i] != '\0'; i++) {
                if (temp[i] == '.') {
                    temp[i] = ',';
                }
            }
            snprintf(temp + strlen(temp), sizeof(temp) - strlen(temp), ",%d,%d", (port >> 8) & 0xFF, port & 0xFF);

            // Convert string to FTP PORT command format and send to server
            host_ip = convert_str_to_addr(temp, &port_num);
            write(sockfd, host_ip, strlen(host_ip));

            memset(buff, 0, sizeof(buff)); // Clear buffer
            //200 port sucess message read
            read(sockfd, buff, BUF_SIZE);
            write(1, buff, strlen(buff));
            

            // Send NLST command to the server
            write(sockfd, ftp_cmd, strlen(ftp_cmd));
            memset(buff, 0, sizeof(buff)); // Clear buffer
            
            //150 message read        
            read(sockfd, buff, BUF_SIZE);
            write(1,buff,strlen(buff));


            // Accept the data connection
            len = sizeof(data_servaddr);
            data_conn = accept(data_sockfd, (struct sockaddr *)&data_servaddr, &len);
            if (data_conn < 0) {
                perror("server accept failed");
                close(data_sockfd);
                exit(EXIT_FAILURE);
            }           // Receive data from server
            memset(buff, 0, sizeof(buff)); // Clear buffer



        if(strncmp(ftp_cmd,"STOR",4)==0){
            //put file informatin send
            write(data_conn, PutBuff, strlen(PutBuff));

            //226 message send
            n=read(sockfd, buff, BUF_SIZE);     
            write(1,buff,strlen(buff));


            char message[100]; // save buff
            sprintf(message,"OK. %ld bytes is sent\n", bytesRead2);
          //  write(sockfd,message,strlen(message));
            write(1,message,strlen(message));
        
            

        }
        ///////////////////////ls get////////////////////////
        else{    
            // Read data from the socket
            n = read(data_conn, buff, BUF_SIZE);
             if (n <= 0) {
                    perror("read error or client closed connection");
                    close(sockfd);
                    exit(EXIT_SUCCESS);  // child exits
                }
            else {
                // Check if the command is RETR
                if(strncmp(ftp_cmd,"RETR",4)!=0 ){
                write(1,buff,strlen(buff));
                size1 = strlen(buff);
                }
                else {
                    if(CheakList==0){
                        if(strncmp(buff,"550",3)==0){
                            printf("550 ERR:file not make beacuse no such file\n");
                        }
                        
                        else{// file write mode open
                        FILE *file = fopen(getfilename, "w");
                        if (file == NULL) {
                            perror("file make error");
                            return 0;
                        }

                        // result_buff information-> buff
                        size_t bytes_written = fwrite(buff, 1, strlen(buff), file);
                        if (bytes_written != strlen(buff)) {
                            perror("file write error");
                        }
                        CheakList=0;
                        // fil close
                        fclose(file);
                        }
                    }
                }

            }    
            write(data_conn,"",1);  
              
            
            memset(buff, 0, sizeof(buff));
            // Read data from the socket
            n=read(sockfd, buff, BUF_SIZE);
           
            write(1,buff,strlen(buff));


            if (strncmp(ftp_cmd, "NLST", 4) == 0 || strncmp(ftp_cmd,"LIST",4)==0){
            if (size1 >= 0) {
                //size printf
            printf("OK. %zu bytes is received.\n", size1);
            }
            else {
                //err printf
            printf("550: Received bytes is negative.\n");
                }     
            }

            if (strncmp(ftp_cmd, "RETR", 4) == 0 ){

                memset(buff,0,sizeof(buff));
                // Read data from the socket
                read(sockfd,buff,strlen(buff));
                write(1,buff,strlen(buff));
            }
        }
            //close data soket
            close(data_conn);
            close(data_sockfd);
        }
    
    
    
    // Send converted command to the server
    else{
        n = write(sockfd, ftp_cmd, BUF_SIZE);
        if (n <= 0) {
            perror("write error");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
    // Receive response from the server and store it in the buffer
            n = read(sockfd, buff, BUF_SIZE);
            if (n <= 0) {
                perror("read error or server closed connection");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            buff[n] = '\0';
            // Print the received response from the server
            write(STDOUT_FILENO, buff, strlen(buff));
            if (strncmp(buff, "221 Goodbye", 6) == 0) {
                write(1,"\n",1);
                break;
                }
            }

}

// Close socket
close(sockfd);
}
/////////////////////main fun END/////////////////////////////


