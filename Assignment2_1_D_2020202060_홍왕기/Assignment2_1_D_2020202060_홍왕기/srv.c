///////////////////////////////////////////////////////////////////////
// File Name : srv.c                                               //
// Date : 2024/04/29                                                //
// OS : Ubuntu 20.04.6 LTS 64bits                                   //
//                                                                  //
// Author : Hong Wang ki                                            //
// Student ID : 2020202060                                          //
// ----------------------------------------------------------------- //
// Title : System Programming Assignment #2-1 (ftp server)           //
// Description: This program implements a simple FTP client that    //
//              communicates with an FTP server.                    //
///////////////////////////////////////////////////////////////////////
// Function: client_info, execute_NLST normal -l -al -a,cmd_process, main//
// ================================================================= //
// Input:                                                             //
//     argc: number of command-line arguments                         //
//     argv: array of command-line arguments                          //
//                                                                    //
// (Input parameter Description)                                      //
//     - argc: number of command-line arguments                       //
//     - argv: array of command-line arguments, where argv[0] is the  //
//             program name and argv[1] (if provided) is the         ////////////////////
//             directory path.                                                          //
//                                                                                       //
// Output:                                                                               //
//     Wipe the buffer and forward it to the server                                     //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose:                                                                                             //
//     This program implements a basic FTP client that communicates with an FTP server using sockets.  //
//     It establishes a connection to the server, sends commands, and receives responses over           //
//     the network. The client facilitates file transfers and directory navigation between the          //
//     client and server. It supports uploading, downloading, listing directories, changing           //
//     directories, renaming files, and quitting the FTP session. It handles FTP commands             //
//     and responses according to the FTP protocol, ensuring proper communication between             //
//     the client and server.                                                                         //
// =====================================================================================================//



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_BUFF 5012
#define SEND_BUFF 5012
#define SA struct sockaddr


////////////////// Client Information Output Function//////////////////
int client_info(struct sockaddr_in *cliaddr) {
    //ip inet_nota use
    char *ip = inet_ntoa(cliaddr->sin_addr);
    //port ntohs use
    uint16_t port = ntohs(cliaddr->sin_port);

   // Make client information a string
    char output[256]; // Maximum size of the output string
    int len = snprintf(output, sizeof(output), "==========Client info===========\nclient IP: %s\nclient port: %hu\n================================\n", ip, port);

    // Use string as standard output
    write(STDOUT_FILENO, output, len);

    return 0; // success return 
}
////////////////// Client_info fun END////////////////////////////////////


////////////////Normal ls function////////////////////////////////////////
void execute_NLST(char *result_buff, char **argv) {
    char *path = NULL;

    //Explore the second argument first
    for (int i = 1; argv[i] != NULL; ++i) {
       // Consider it a path if it is a non-optional argument
        if (argv[i][0] != '-') {
            path = argv[i];
            break;
        }
    }

    // use dirent
    DIR *dir;
    struct dirent *entry;

    if (path != NULL) {
        // directory check exist
        if (access(path, F_OK) == -1) {
            perror("Error accessing directory");
            //send direcotry does not exist-> cli.c
            strcpy(result_buff, "Directory does not exist\n");
            return; // if not exist return
        }

        // access cheak
        if (access(path, R_OK) == -1) {
            perror("Error accessing directory");
            //send Permission denied ->cli.c
            strcpy(result_buff, "Permission denied\n");
            return; // if not access return.
        }
        //open dir
        dir = opendir(path);
    } 
    else {
        //dircetory is current
        dir = opendir(".");
    }
    //if not open directory return errno
    if (dir == NULL) {
        perror("Error opening directory");
        //send Error open directory ->cli.c
        strcpy(result_buff, "Error opening directory\n");
        return;
    }

    // Get a list of all files in the directory and add them to result_buff
    while ((entry = readdir(dir)) != NULL) {
        // Make sure it's a hidden file and add it only if it's not a hidden file
        if (entry->d_name[0] != '.') {
            strcat(result_buff, entry->d_name);
            strcat(result_buff, "\n");
        }
    }
    //close directory
    closedir(dir);
}
////////////////////////Normal ls fun END/////////////////////////////


/////////////////////// ls -l function//////////////////////////////////
void execute_NLST_l(char *result_buff, char **argv) {
    // Open directory
    DIR *dir;
    struct dirent *entry;
    struct stat fileStat;
    char buf[256];
    // If path is not specified, use current path
    char *path = (argv[2] != NULL) ? argv[2] : "."; 

   // Structure for storing file information
    struct file_info {
        char type; // File type
        char permissions[10]; // File permissions
        int links; // Number of links
        char owner[32]; // File owner
        char group[32];  // File group
        long size; // File size
        char date[20];   // Last modified date
        char name[256]; // File name
    } file_info; 

     // Check if the path is a file or directory
    int is_file = 0;
    if (stat(path, &fileStat) == -1) {
        // Print error message if the directory does not exist
        perror("Error: No such file or directory");
        strcpy(result_buff, "Error: No such file or directory\n");
        return;
    }
    if (S_ISREG(fileStat.st_mode)) {
        // If the path points to a file
        is_file = 1;
    }

    // Check read permission for the file or directory
    if (!is_file && access(path, R_OK) == -1) {
        // Print error message if there is no read permission
        perror("Error: No read permission");
        strcpy(result_buff, "Error: No read permission\n");
        return;
    }

    // Open directory if the path is not a file
    if (!is_file) {
        dir = opendir(path);
        // Print error message if failed to open directory
        if (dir == NULL) {
            perror("Error opening directory");
            strcpy(result_buff, "Error opening directory\n");
            return;
        }
    }

   // Get information of all files in the directory
    if (!is_file) {
       // Check read permission
        if (!(fileStat.st_mode & S_IRUSR)) {
            // Print error message if there is no read permission
            perror("Error: No read permission");
            strcpy(result_buff, "Error: No read permission\n");
            closedir(dir);
            return;
        }

        while ((entry = readdir(dir)) != NULL) {
            // Skip hidden files
            if (entry->d_name[0] == '.')
                continue;

             // Get detailed information of the file
            // Array with sufficient size to store the file name
            char filename[512]; 
            // Create full path
            sprintf(filename, "%s/%s", path, entry->d_name); 

            // Get file information
            if (stat(filename, &fileStat) == -1) {
                perror("Error reading file information");
                continue;
            }
            // Change time format
            strftime(buf, sizeof(buf), "%m월 %d일 %H:%M", localtime(&fileStat.st_mtime)); // time change

            // Store file information in the structure
            file_info.type = (S_ISDIR(fileStat.st_mode)) ? 'd' : '-';
            sprintf(file_info.permissions, "%s%s%s%s%s%s%s%s%s",
                    (fileStat.st_mode & S_IRUSR) ? "r" : "-",
                    (fileStat.st_mode & S_IWUSR) ? "w" : "-",
                    (fileStat.st_mode & S_IXUSR) ? "x" : "-",
                    (fileStat.st_mode & S_IRGRP) ? "r" : "-",
                    (fileStat.st_mode & S_IWGRP) ? "w" : "-",
                    (fileStat.st_mode & S_IXGRP) ? "x" : "-",
                    (fileStat.st_mode & S_IROTH) ? "r" : "-",
                    (fileStat.st_mode & S_IWOTH) ? "w" : "-",
                    (fileStat.st_mode & S_IXOTH) ? "x" : "-");
            file_info.links = fileStat.st_nlink;
            strcpy(file_info.owner, getpwuid(fileStat.st_uid)->pw_name);
            strcpy(file_info.group, getgrgid(fileStat.st_gid)->gr_name);
            file_info.size = (file_info.type == 'd') ? 0 : fileStat.st_size;
            strcpy(file_info.date, buf);
            strcpy(file_info.name, entry->d_name);

            // Add result to result_buff
            sprintf(result_buff + strlen(result_buff), "%c%s %d %s %s %ld %s %s\n",
                    file_info.type, file_info.permissions, file_info.links,
                    file_info.owner, file_info.group, file_info.size,
                    file_info.date, file_info.name);
        }
        //close dir
        closedir(dir);
    } else {
        // if file case
        //import  file information 
        stat(path, &fileStat);
        strftime(buf, sizeof(buf), "%m월 %d일 %H:%M", localtime(&fileStat.st_mtime)); // time chage format

         // Store file information in the structure
        file_info.type = (S_ISDIR(fileStat.st_mode)) ? 'd' : '-';
        sprintf(file_info.permissions, "%s%s%s%s%s%s%s%s%s",
                (fileStat.st_mode & S_IRUSR) ? "r" : "-",
                (fileStat.st_mode & S_IWUSR) ? "w" : "-",
                (fileStat.st_mode & S_IXUSR) ? "x" : "-",
                (fileStat.st_mode & S_IRGRP) ? "r" : "-",
                (fileStat.st_mode & S_IWGRP) ? "w" : "-",
                (fileStat.st_mode & S_IXGRP) ? "x" : "-",
                (fileStat.st_mode & S_IROTH) ? "r" : "-",
                (fileStat.st_mode & S_IWOTH) ? "w" : "-",
                (fileStat.st_mode & S_IXOTH) ? "x" : "-");
        file_info.links = fileStat.st_nlink;
        strcpy(file_info.owner, getpwuid(fileStat.st_uid)->pw_name);
        strcpy(file_info.group, getgrgid(fileStat.st_gid)->gr_name);
        file_info.size = (file_info.type == 'd') ? 0 : fileStat.st_size;
        strcpy(file_info.date, buf);
        strcpy(file_info.name, path);

          // Add result to result_buff
        sprintf(result_buff + strlen(result_buff), "%c%s %d %s %s %ld %s %s\n",
                file_info.type, file_info.permissions, file_info.links,
                file_info.owner, file_info.group, file_info.size,
                file_info.date, file_info.name);
    }
}
//////////////////////ls -l fun END////////////////////////////////////


/////////////////////ls -a function//////////////////////////////////
void execute_NLST_a(char *result_buff, char **argv) {
    char *path = NULL;

    // Ignore the first argument of argv,
    // which is the name of the executable,
    // and start from the second argument
    for (int i = 1; argv[i] != NULL; ++i) {
        // If the argument is not an option, consider it as a path
        if (argv[i][0] != '-') {
            path = argv[i];
            break;
        }
    }

     // Open directory
    DIR *dir;
    struct dirent *entry;

    if (path != NULL) {
         // Check if the directory exists
        if (access(path, F_OK) == -1) {
            perror("Error accessing directory");
            strcpy(result_buff, "Directory does not exist\n");
            // If the directory does not exist, exit the function immediately.
            return;
        }
        // Check permission
        if (access(path, R_OK) == -1) {
            perror("Error accessing directory");
            strcpy(result_buff, "Permission denied\n");
            // If there is no access permission, exit the function immediately.
            return;
        }
        dir = opendir(path);
    } else {
        dir = opendir(".");
    }
    //dir dont open case errno
    if (dir == NULL) {
        perror("Error opening directory");
        strcpy(result_buff, "Error opening directory\n");
        return;
    }

     // Get the list of all files in the directory and add them to result_buff
    while ((entry = readdir(dir)) != NULL) {
        strcat(result_buff, entry->d_name);
        strcat(result_buff, "\n");
    }
    //close dir
    closedir(dir);
}
////////////////////////ls -a fun END////////////////////////////////


///////////////////////ls -al function///////////////////////////////
void execute_NLST_al(char *result_buff, char **argv) {
     // Open directory
    DIR *dir;
    struct dirent *entry;
    struct stat fileStat;
    char buf[256];
     // If path is not specified, use current directory
    char *path = (argv[2] != NULL) ? argv[2] : "."; 

     // Structure for storing file information
    struct file_info {
        char type;  // File type
        char permissions[10];  // File permissions
        int links; // Number of links
        char owner[32];  // File owner
        char group[32];  // File group
        long size;  // File size
        char date[20];   // Last modified date
        char name[256];  // File name
    } file_info; // Structure to store individual file information

    // Check if the path is a file or directory
    int is_file = 0;
    if (stat(path, &fileStat) == -1) {
        perror("Error: No such file or directory");
        strcpy(result_buff, "Error: No such file or directory\n");
        return;
    }
    if (S_ISREG(fileStat.st_mode)) {
         // If the path points to a file
        is_file = 1;
    }

     // Check read permission for the file or directory
    if (!is_file && access(path, R_OK) == -1) {
        perror("Error: No read permission");
        strcpy(result_buff, "Error: No read permission\n");
        return;
    }

    // Open directory if the path is not a file
    if (!is_file) {
        dir = opendir(path);
        if (dir == NULL) {
            perror("Error opening directory");
            strcpy(result_buff, "Error opening directory\n");
            return;
        }
    }

     // Get information of all files in the directory
    if (!is_file) {
         // Check read permission
        if (!(fileStat.st_mode & S_IRUSR)) {
            perror("Error: No read permission");
            strcpy(result_buff, "Error: No read permission\n");
            closedir(dir);
            return;
        }

        while ((entry = readdir(dir)) != NULL) {
            // Get detailed information of the file
            // Array with sufficient size to store the file name
            char filename[512]; 
            sprintf(filename, "%s/%s", path, entry->d_name); // Create full path

            // Get file information
            if (stat(filename, &fileStat) == -1) {
                perror("Error reading file information");
                continue;
            }

            strftime(buf, sizeof(buf), "%m월 %d일 %H:%M", localtime(&fileStat.st_mtime)); // Change time forma

           // Store file information in the structure
            file_info.type = (S_ISDIR(fileStat.st_mode)) ? 'd' : '-';
            sprintf(file_info.permissions, "%s%s%s%s%s%s%s%s%s",
                    (fileStat.st_mode & S_IRUSR) ? "r" : "-",
                    (fileStat.st_mode & S_IWUSR) ? "w" : "-",
                    (fileStat.st_mode & S_IXUSR) ? "x" : "-",
                    (fileStat.st_mode & S_IRGRP) ? "r" : "-",
                    (fileStat.st_mode & S_IWGRP) ? "w" : "-",
                    (fileStat.st_mode & S_IXGRP) ? "x" : "-",
                    (fileStat.st_mode & S_IROTH) ? "r" : "-",
                    (fileStat.st_mode & S_IWOTH) ? "w" : "-",
                    (fileStat.st_mode & S_IXOTH) ? "x" : "-");
            file_info.links = fileStat.st_nlink;
            strcpy(file_info.owner, getpwuid(fileStat.st_uid)->pw_name);
            strcpy(file_info.group, getgrgid(fileStat.st_gid)->gr_name);
            file_info.size = (file_info.type == 'd') ? 0 : fileStat.st_size;
            strcpy(file_info.date, buf);
            strcpy(file_info.name, entry->d_name);

             // Add result to result_buff
            sprintf(result_buff + strlen(result_buff), "%c%s %d %s %s %ld %s %s\n",
                    file_info.type, file_info.permissions, file_info.links,
                    file_info.owner, file_info.group, file_info.size,
                    file_info.date, file_info.name);
        }
        closedir(dir);
    } else {
       // If the path is a file
        // Get file information
        stat(path, &fileStat);
        strftime(buf, sizeof(buf), "%m월 %d일 %H:%M", localtime(&fileStat.st_mtime)); //time format change

       // Store file information in the structure
        file_info.type = (S_ISDIR(fileStat.st_mode)) ? 'd' : '-';
        sprintf(file_info.permissions, "%s%s%s%s%s%s%s%s%s",
                (fileStat.st_mode & S_IRUSR) ? "r" : "-",
                (fileStat.st_mode & S_IWUSR) ? "w" : "-",
                (fileStat.st_mode & S_IXUSR) ? "x" : "-",
                (fileStat.st_mode & S_IRGRP) ? "r" : "-",
                (fileStat.st_mode & S_IWGRP) ? "w" : "-",
                (fileStat.st_mode & S_IXGRP) ? "x" : "-",
                (fileStat.st_mode & S_IROTH) ? "r" : "-",
                (fileStat.st_mode & S_IWOTH) ? "w" : "-",
                (fileStat.st_mode & S_IXOTH) ? "x" : "-");
        file_info.links = fileStat.st_nlink;
        strcpy(file_info.owner, getpwuid(fileStat.st_uid)->pw_name);
        strcpy(file_info.group, getgrgid(fileStat.st_gid)->gr_name);
        file_info.size = (file_info.type == 'd') ? 0 : fileStat.st_size;
        strcpy(file_info.date, buf);
        strcpy(file_info.name, path);

        // Add result to result_buff
        sprintf(result_buff + strlen(result_buff), "%c%s %d %s %s %ld %s %s\n",
                file_info.type, file_info.permissions, file_info.links,
                file_info.owner, file_info.group, file_info.size,
                file_info.date, file_info.name);
    }
}
/////////////////////////ls -al fun END////////////////////////////////


////////////////////////cmd process function///////////////////////////
int cmd_process(char *buff, char *result_buff) {
    // Initialize result_buff
    memset(result_buff, 0, SEND_BUFF);

    char *command;
    char *args[MAX_BUFF];
    int argc = 0;

   // Parse the buffer into command and arguments separated by " "
    command = strtok(buff, " ");
    while (command != NULL && argc < MAX_BUFF - 1) {
        args[argc++] = command;
        command = strtok(NULL, " ");
    }
    args[argc] = NULL; // Add NULL termination

    // Process the NLST command
    if (strcmp(args[0], "NLST") == 0) {
         // Handle NLST command
        if (argc > 1 &&argc <4) {
            // NLST command with options
            if (strcmp(args[1], "-a") == 0) {
                for (int i = 0; i < argc; i++) {
                write(1, args[i], strlen(args[i]));
                write(1, " ", 1);
            }
                write(1, "\n", 1);
                execute_NLST_a(result_buff, args);
            }
            // NLST command with "-l" option
            else if (strcmp(args[1], "-l") == 0) {
                for (int i = 0; i < argc; i++) {
                write(1, args[i], strlen(args[i]));
                write(1, " ", 1);
                }
                write(1, "\n", 1);
                execute_NLST_l(result_buff,args);
            }   
            // NLST command with "-al" option
            else if (strcmp(args[1], "-al") == 0) {
                for (int i = 0; i < argc; i++) {
                write(1, args[i], strlen(args[i]));
                write(1, " ", 1);
                }
                write(1, "\n", 1);
                execute_NLST_al(result_buff,args);
            }

            // NLST command with path argument
            else if(argc==2 && args[1][0]!='-'){
                for (int i = 0; i < argc; i++) {
                write(1, args[i], strlen(args[i]));
                write(1, " ", 1);
                }
                write(1, "\n", 1);
                execute_NLST(result_buff,args);
            }
        
        
            else {
                 // Invalid options
                if(args[1][0]=='-'){
                    for (int i = 0; i < argc; i++) {
                    write(1, args[i], strlen(args[i]));
                    write(1, " ", 1);
                    strcpy(result_buff, "Invalid option\n");
                    }
                     write(1,"\n",1);
                }
                else{
                     // Too many path arguments
                    for (int i = 0; i < argc; i++) {
                    write(1, args[i], strlen(args[i]));
                    write(1, " ", 1);
                    strcpy(result_buff, "too many path arguments\n");
                    }
                     write(1,"\n",1);
                }
            }

        }
        // NLST command without options or arguments
         else if(argc==1) {
            write(1,"NLST",strlen("NLST"));
            write(1,"\n",1);
            execute_NLST(result_buff,args);
        }

         // Too many arguments case
        else{
            for (int i = 0; i < argc; i++) {
                write(1, args[i], strlen(args[i]));
                write(1, " ", 1);
            strcpy(result_buff, "too many arguments\n");
        }
         write(1,"\n",1);
        }
    }
    

     // Process the QUIT command
    else if (strcmp(args[0], "QUIT") == 0) {
        if(argc>1){
            // QUIT command with additional arguments
            if(args[1][0]=='-'){
                 for (int i = 0; i < argc; i++) {
                    write(1, args[i], strlen(args[i]));
                    write(1, " ", 1);
                strcpy(result_buff, "QUIT is Not option\n");
                }
                  write(1,"\n",1);
            }
            else{
                // QUIT command with arguments
                    for (int i = 0; i < argc; i++) {
                    write(1, args[i], strlen(args[i]));
                    write(1, " ", 1);
                    }
                    strcpy(result_buff, "QUIT is No argument\n");
                    write(1,"\n",1);
            }
        }
        // QUIT command without additional arguments
        else{
        write(1,"QUIT\n",strlen("QUIT\n"));
        strcpy(result_buff, "Program quit!!\n");
        }
    }

     // Unknown command
    else {
        for (int i = 0; i < argc; i++) {
            write(1, args[i], strlen(args[i]));
            write(1, " ", 1);
            }
            write(1,"\n",1);
        strcpy(result_buff, "Invalid command\n");
    }

    return 0;
}

///////////////////////////main function//////////////////////////////
int main(int argc, char **argv) {
    int listenfd, connfd, n;
    struct sockaddr_in servaddr, cliaddr;
    char buff[MAX_BUFF], result_buff[SEND_BUFF];
    // less arument error
    if (argc == 1) {
        write(1,"less argument\n",strlen("less argument\n"));
        exit(1); 
    }

    if(argc >2){
        write(1,"too many argument\n",strlen("too many argument\n"));
        exit(1);
    }
    // Check if the port number is numeric
    // If not numeric, exit the program
    for (int i = 0; argv[1][i] != '\0'; i++) {
        if (!isdigit(argv[1][i])) {
            write(1, "Port number must be numeric\n", strlen("Port number must be numeric\n"));
            exit(1);
        }
    }

    // Socket Creation
    // Create a socket for the server
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Server Address Configuration
    // Set up the server address structure
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1])); // server fort.

    // Bind Socket to Address
    // Bind the socket to the server address
    if ((bind(listenfd, (SA *)&servaddr, sizeof(servaddr))) != 0) {
        perror("socket bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for Connection Requests
    // Start listening for incoming connections
    if ((listen(listenfd, 5)) != 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    // Accept Client Connections
    // Accept incoming client connections in a loop
    for (;;) {
        socklen_t clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (SA *)&cliaddr, &clilen);
        if (connfd < 0) {
            perror("server accept failed");
            exit(EXIT_FAILURE);
        }

        // Print Client Information
        // Print information about the connected client
        if (client_info(&cliaddr) < 0) {
            write(STDERR_FILENO, "client_info() err!!\n", sizeof("client_info() err!!\n"));
        }

        /// Communicate with Client
        // Communicate with the client
        while (1) {
            // Receive Command from Client
            // Receive a command from the client
            n = read(connfd, buff, MAX_BUFF);
            if (n <= 0) {
                perror("read error or client closed connection");
                close(connfd);
                break;
            }
            buff[n] = '\0';

            // Process Command
            // Process the received command
            cmd_process(buff, result_buff);

            // Send Result to Client
            // Send the result back to the client
            n = write(connfd, result_buff, strlen(result_buff) + 1); 
            
            // Handle QUIT Command
            // If QUIT command is received, close the connection
           if (strcmp(result_buff, "Program quit!!\n") == 0) {
                close(connfd);
                close(listenfd);
                exit(0);
            
            }
            //error case
            if (n < 0) {
                perror("write error");
                close(connfd);
                exit(EXIT_FAILURE);
            }

        }
    }
    //close
    close(connfd);
    close(listenfd);
    return 0;
}
//////////////////////main fun END/////////////////////////////