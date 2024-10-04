///////////////////////////////////////////////////////////////////////
// File Name : srv.c                                               //
// Date : 2024/05/11                                                //
// OS : Ubuntu 20.04.6 LTS 64bits                                   //
//                                                                  //
// Author : Hong Wang ki                                            //
// Student ID : 2020202060                                          //
// ----------------------------------------------------------------- //
// Title : System Programming Assignment #2-3 (ftp server)           //
// Description: This program implements a simple FTP client that    //
//              communicates with an FTP server.                    //
///////////////////////////////////////////////////////////////////////
// Function: client_info, execute_NLST normal -l -al -a,cmd_process, main//
//           cwd,pwd,cdup,renmae,list,rmdir,mkdir cmd fun             //
//             compare, sh_alarm.sh_chld                              //
//              print client, add_client, remove client              //
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
//          When the program receives 10 instructions from the client,                                  //
//          the operation is transmitted to the client using a socket.                                  //
//           At this time, a plurality of clients can come in,                                          //
//           so that the plurality of clients can request from the server.                              //
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
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#define MAX_BUFF 5012
#define SEND_BUFF 5012
#define BUF_SIZE 100
#define SA struct sockaddr

pid_t save_pid; // Declare as a global variable
int count_arguments=0;

// Structure to hold client information
struct ClientInfo {
    pid_t pid;
    int port;
    time_t connect_time;
};


// Global vector to hold connected client information
struct ClientInfo *connected_clients = NULL;
size_t connected_clients_count = 0;

void add_client(pid_t pid, int port, time_t connect_time) {
    // Increase the size of the vector
    connected_clients_count++;
    connected_clients = (struct ClientInfo *)realloc(connected_clients, sizeof(struct ClientInfo) * connected_clients_count);
    if (connected_clients == NULL) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }

    // Add the new client to the vector
    connected_clients[connected_clients_count - 1].pid = pid;
    connected_clients[connected_clients_count - 1].port = port;
    connected_clients[connected_clients_count - 1].connect_time = connect_time;
}

void remove_client(pid_t pid) {
    size_t found_index = connected_clients_count;
    for (size_t i = 0; i < connected_clients_count; i++) {
        if (connected_clients[i].pid == pid) {
            found_index = i;
            break;
        }
    }
    
    if (found_index < connected_clients_count) {
        // Move the elements after the found client one position to the left
        for (size_t i = found_index; i < connected_clients_count - 1; i++) {
            connected_clients[i] = connected_clients[i + 1];
        }
        // Clear the last element
        connected_clients[connected_clients_count - 1].pid = 0;
        connected_clients[connected_clients_count - 1].port = 0;
        connected_clients[connected_clients_count - 1].connect_time = 0;
        // Decrement the count of connected clients
        connected_clients_count--;
    } else {
        // if no client
        printf("Client with PID %d not found\n", pid);
    }
}

void print_connected_clients() {
    //print all client pid, port, time
    printf("\nCurrent Number of Client : %zu\n", connected_clients_count);
    printf("PID\tPORT\tTIME\n");
    for (size_t i = 0; i < connected_clients_count; i++) {
        printf("%d\t%d\t%d\n", connected_clients[i].pid, connected_clients[i].port, (int)difftime(time(NULL), connected_clients[i].connect_time));
    }
    printf("\n");
}

// ls -l and ls -al need comepare fun
int compare(const struct dirent **a, const struct dirent **b) {
    return strcmp((*a)->d_name, (*b)->d_name);
}
//normal ls and ls -a compare fun
int compare1(const void *a, const void *b) {
    const char *const *str_a = a;
    const char *const *str_b = b;
    return strcmp(*str_a, *str_b);
}


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


////////////////Normal ls function////////////////////////////////////////
void execute_NLST(char *result_buff, char **argv) {
     memset(result_buff, 0, SEND_BUFF);
    char *path = NULL;
    // Explore the second argument first
    for (int i = 1; argv[i] != NULL; ++i) {
        // Consider it a path if it is a non-optional argument
        if (argv[i][0] != '-') {
            path = argv[i];
            break;
        }
    }

    // Use dirent
    DIR *dir;
    struct dirent *entry;
    struct stat st; // stat struct



    if (path != NULL) {
        // Directory check exist
        if (access(path, F_OK) == -1) {
            
            strcpy(result_buff, "Directory does not exist\n");
            return; // If not exist return
        }

        // Access check
        if (access(path, R_OK) == -1 && (stat(path, &st) == 0 && S_ISDIR(st.st_mode))) {
            // Send Permission denied ->cli.c
            strcpy(result_buff, "Permission denied\n");
            return; // If not access return.
        }
        // Open dir
        dir = opendir(path);
    } else {
        // Directory is current
        dir = opendir(".");
    }

    if (dir == NULL) {
    strcat(result_buff, "Error: is not directory\n");
    return;
    }

 

    // Array to store file names
    char **file_names = NULL;
    int num_files = 0;

    // Get a list of all files in the directory and add them to file_names array
    while ((entry = readdir(dir)) != NULL) {
        // Make sure it's a hidden file and add it only if it's not a hidden file
        if (entry->d_name[0] != '.') {
            file_names = realloc(file_names, (num_files + 1) * sizeof(char *));
            file_names[num_files] = strdup(entry->d_name);
            num_files++;
        }
    }

    // Close directory
    closedir(dir);

    // Sort file names alphabetically
    qsort(file_names, num_files, sizeof(char *), compare1);

    // Concatenate sorted file names to result_buff
    for (int i = 0; i < num_files; ++i) {
        strcat(result_buff, file_names[i]);

        // Check if it's a directory and add "/" if it is
        struct stat st;
        char path_with_name[1024];
        sprintf(path_with_name, "%s/%s", path != NULL ? path : ".", file_names[i]);
        if (stat(path_with_name, &st) == 0 && S_ISDIR(st.st_mode)) {
            strcat(result_buff, "/");
        }

        strcat(result_buff, "\n");
        
        // Free memory allocated for each file name
        free(file_names[i]);
    }

    // Free memory allocated for file_names array
    free(file_names);
}
////////////////////////Normal ls fun END/////////////////////////////


/////////////////////// ls -l function//////////////////////////////////
void execute_NLST_l(char *result_buff, char **args) {
     memset(result_buff, 0, SEND_BUFF);
    // If no directory is specified, work in the current directory
    char *dirname = (args[2] == NULL) ? "." : args[2];

    // Open the directory
    struct dirent **namelist;
    int n;

    n = scandir(dirname, &namelist, NULL, compare);
    if (n < 0) {
        //if file case
        if (errno == ENOTDIR) {
            strcat(result_buff, "Error: is not directory\n");
            return;
        } else if (errno == EACCES) { // If access is restricted
            strcat(result_buff, "Error: Permission denied\n");
            return;
        } else if (errno == ENOENT) { // Non-existent files or directories
            strcat(result_buff, "Error: Does not exist\n");
            return;
        } else {
            perror(result_buff + strlen(result_buff));
            return;
        }
    }

    // Check if the directory is accessible
    if (access(dirname, R_OK) == -1) {
        strcat(result_buff, "Error: Permission denied\n");
        return;
    }

    // Output the sorted list
    for (int i = 0; i < n; i++) {
        // Check if the file name starts with a dot (hidden file)
        if (namelist[i]->d_name[0] == '.')
            continue; // Skip hidden files

        // Get file details
        struct stat fileStat;
        char path[1024];
        sprintf(path, "%s/%s", dirname, namelist[i]->d_name);
        if (lstat(path, &fileStat) < 0) {
            perror(result_buff + strlen(result_buff));
            continue;
        }

        // Output file type and permissions
        char permissions[11];
        sprintf(permissions, "%s%s%s%s%s%s%s%s%s%s",
                (S_ISDIR(fileStat.st_mode)) ? "d" : "-",
                (fileStat.st_mode & S_IRUSR) ? "r" : "-",
                (fileStat.st_mode & S_IWUSR) ? "w" : "-",
                (fileStat.st_mode & S_IXUSR) ? "x" : "-",
                (fileStat.st_mode & S_IRGRP) ? "r" : "-",
                (fileStat.st_mode & S_IWGRP) ? "w" : "-",
                (fileStat.st_mode & S_IXGRP) ? "x" : "-",
                (fileStat.st_mode & S_IROTH) ? "r" : "-",
                (fileStat.st_mode & S_IWOTH) ? "w" : "-",
                (fileStat.st_mode & S_IXOTH) ? "x" : "-");
        strcat(result_buff, permissions);
        strcat(result_buff, " ");

        // Output link count
        char linkCount[32];
        int linkLength = sprintf(linkCount, "%lu", fileStat.st_nlink);
        strcat(result_buff, linkCount);
        strcat(result_buff, " ");

        // Output owner and group
        struct passwd *pwd = getpwuid(fileStat.st_uid);
        struct group *grp = getgrgid(fileStat.st_gid);
        strcat(result_buff, pwd->pw_name);
        strcat(result_buff, " ");
        strcat(result_buff, grp->gr_name);
        strcat(result_buff, " ");

        // Output file size
        char fileSize[32];
        int sizeLength = sprintf(fileSize, "%lu", fileStat.st_size);
        strcat(result_buff, fileSize);
        strcat(result_buff, " ");

        // Output modification time
        char date[100];
        strftime(date, sizeof(date), "%b %e %H:%M", localtime(&fileStat.st_mtime));
        strcat(result_buff, date);
        strcat(result_buff, " ");

        // Output file name
        strcat(result_buff, namelist[i]->d_name);

        // Add '/' if it's a directory
        if (S_ISDIR(fileStat.st_mode)) {
            strcat(result_buff, "/");
        }

        strcat(result_buff, "\n");

        free(namelist[i]);
    }

    // Free memory and close directory
    free(namelist);
}
//////////////////////ls -l fun END////////////////////////////////////


/////////////////////ls -a function//////////////////////////////////
void execute_NLST_a(char *result_buff, char **argv) {
     memset(result_buff, 0, SEND_BUFF);
    char *path = NULL;

    // Explore the second argument first
    for (int i = 1; argv[i] != NULL; ++i) {
        // Consider it a path if it is a non-optional argument
        if (argv[i][0] != '-') {
            path = argv[i];
            break;
        }
    }

    // Use dirent
    DIR *dir;
    struct dirent *entry;
    struct stat st; // stat struct

    if (path != NULL) {
        // Directory check exist
        if (access(path, F_OK) == -1) {
            // Send directory does not exist-> cli.c
            strcpy(result_buff, "Directory does not exist\n");
            return; // If not exist return
        }

        // Access check
           // Access check
        if (access(path, R_OK) == -1 && (stat(path, &st) == 0 && S_ISDIR(st.st_mode))) {
            // Send Permission denied ->cli.c
            strcpy(result_buff, "Permission denied\n");
            return; // If not access return.
        }
        // Open dir
        dir = opendir(path);
    } else {
        // Directory is current
        dir = opendir(".");
    }

       if (dir == NULL) {
        strcat(result_buff, "Error: is not directory\n");
        return;
    }

    // Array to store file names
    char **file_names = NULL;
    int num_files = 0;

    // Get a list of all files in the directory and add them to file_names array
    while ((entry = readdir(dir)) != NULL) {
        file_names = realloc(file_names, (num_files + 1) * sizeof(char *));
        file_names[num_files] = strdup(entry->d_name);
        num_files++;
    }

    // Close directory
    closedir(dir);

    // Sort file names alphabetically
    qsort(file_names, num_files, sizeof(char *), compare1);

    // Concatenate sorted file names to result_buff
    for (int i = 0; i < num_files; ++i) {
        strcat(result_buff, file_names[i]);

        // Check if it's a directory and add "/" if it is
        struct stat st;
        char path_with_name[1024];
        sprintf(path_with_name, "%s/%s", path != NULL ? path : ".", file_names[i]);
        if (stat(path_with_name, &st) == 0 && S_ISDIR(st.st_mode)) {
            strcat(result_buff, "/");
        }

        strcat(result_buff, "\n");
        
        // Free memory allocated for each file name
        free(file_names[i]);
    }

    // Free memory allocated for file_names array
    free(file_names);
}
////////////////////////ls -a fun END////////////////////////////////


///////////////////////ls -al function///////////////////////////////
void execute_NLST_al(char *result_buff, char **argv) {
    // Check if the number of arguments exceeds one
      memset(result_buff, 0, SEND_BUFF);
   if (count_arguments>3){
        strcat(result_buff, "Error: too many arguments\n");
        return;
    }

    // If no directory is specified, work in the current directory
    char *dirname = (argv[2] == NULL) ? "." : argv[2];

    // Open the directory
    struct dirent **namelist;
    int n;

    n = scandir(dirname, &namelist, NULL, compare);
    if (n < 0) {
        if(errno == ENOTDIR){ //if file case
        strcat(result_buff, "Error: is not directory\n");
        return;
    }
    else if (errno == EACCES) {// If access is restricted
        strcat(result_buff, "Error: Permission denied\n");
        return;
        }
        
    
    else if (errno == ENOENT) { // // Non-existent files or directories
            strcat(result_buff, "Error: Does not exist\n");
            return;
        }
    else{
        perror(result_buff + strlen(result_buff));
            return;
    }   
    }

     // Check if the directory is accessible
    if (access(dirname, R_OK) == -1) {
        strcat(result_buff, "Error: Permission denied\n");
        return;
    }

    // Output the sorted list
    for (int i = 0; i < n; i++) {
        // Get file details
        struct stat fileStat;
        char path[1024];
        sprintf(path, "%s/%s", dirname, namelist[i]->d_name);
        if (lstat(path, &fileStat) < 0) {
            perror(result_buff + strlen(result_buff));
            continue;
        }

        // Output file type and permissions
        char permissions[11];
        sprintf(permissions, "%s%s%s%s%s%s%s%s%s%s",
                (S_ISDIR(fileStat.st_mode)) ? "d" : "-",
                (fileStat.st_mode & S_IRUSR) ? "r" : "-",
                (fileStat.st_mode & S_IWUSR) ? "w" : "-",
                (fileStat.st_mode & S_IXUSR) ? "x" : "-",
                (fileStat.st_mode & S_IRGRP) ? "r" : "-",
                (fileStat.st_mode & S_IWGRP) ? "w" : "-",
                (fileStat.st_mode & S_IXGRP) ? "x" : "-",
                (fileStat.st_mode & S_IROTH) ? "r" : "-",
                (fileStat.st_mode & S_IWOTH) ? "w" : "-",
                (fileStat.st_mode & S_IXOTH) ? "x" : "-");
        strcat(result_buff, permissions);
        strcat(result_buff, " ");

        // Output link count
        char linkCount[32];
        int linkLength = sprintf(linkCount, "%lu", fileStat.st_nlink);
        strcat(result_buff, linkCount);
        strcat(result_buff, " ");

        // Output owner and group
        struct passwd *pwd = getpwuid(fileStat.st_uid);
        struct group *grp = getgrgid(fileStat.st_gid);
        strcat(result_buff, pwd->pw_name);
        strcat(result_buff, " ");
        strcat(result_buff, grp->gr_name);
        strcat(result_buff, " ");

        // Output file size
        char fileSize[32];
        int sizeLength = sprintf(fileSize, "%lu", fileStat.st_size);
        strcat(result_buff, fileSize);
        strcat(result_buff, " ");

        // Output modification time
        char date[100];
        strftime(date, sizeof(date), "%b %e %H:%M", localtime(&fileStat.st_mtime));
        strcat(result_buff, date);
        strcat(result_buff, " ");

        // Output file name
        strcat(result_buff, namelist[i]->d_name);

        // Add '/' if it's a directory
        if (S_ISDIR(fileStat.st_mode)) {
            strcat(result_buff, "/");
        }

        strcat(result_buff, "\n");

        free(namelist[i]);
    }

    // Free memory and close directory
    free(namelist);
}
/////////////////////////ls -al fun END////////////////////////////////

/////////////////////////PWD function/////////////////////////////////
void PWDcmd(char *result_buff, char **argv) {
    // Initialize result_buff
    memset(result_buff, 0, SEND_BUFF);

    // ./cli pwd | tee cli.out | ./srv -> Print the current directory
    // ./cli pwd -e | tee cli.out | ./srv -> Print the current directory (ignore options)
    // ./cli pwd arg | tee cli.out | ./srv -> Print the current directory (ignore arguments)
    
    if (argv[1] != NULL && argv[1][0] == '-') {
        // If there's an option, handle error
        strcpy(result_buff, "Error: invalid option\n");
    } else if (argv[1] != NULL) {
        // If there's an argument, handle error
        strcpy(result_buff, "Error: argument is not required\n");
    } else {
        // If neither option nor argument is provided, print the current directory
        char cwd[256];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            // Write current directory to result_buff
            strcat(result_buff, cwd);
            strcat(result_buff, " is current directory\n");
        } else {
            perror("getcwd");
            exit(EXIT_FAILURE);
        }
    }
}
/////////////////////////PWD function END/////////////////////////////////


void MKDcmd(char *result_buff, char **argv) {
    // Initialize result_buff
    memset(result_buff, 0, SEND_BUFF);

    if (argv[1] == NULL) {
        // If there's no argument, handle error
        strcat(result_buff, "Error: argument is required\n");
        return;
    }
    if (argv[1][0] == '-') {
        // If the first argument starts with '-', handle error
        strcat(result_buff, "Error: invalid option\n");
        return;
    }

    // Create each directory provided as an argument
    for (int i = 1; argv[i] != NULL; i++) {
        if (mkdir(argv[i], 0775) == 0) {
            // If directory creation is successful, write to result_buff
            strcat(result_buff, argv[0]);
            strcat(result_buff, " ");
            strcat(result_buff, argv[i]);
            strcat(result_buff, "\n");
        } else {
            // If the directory already exists, handle error
            strcat(result_buff, "Error: cannot create directory '");
            strcat(result_buff, argv[i]);
            strcat(result_buff, "' : File exists\n");
        }
    }
}

void RMDcmd(char *result_buff, char **argv) {
    // Initialize result_buff
    memset(result_buff, 0, SEND_BUFF);

    if (argv[1] == NULL) {
        // Handle error if there's no argument
        strcat(result_buff, "Error: argument is required\n");
        return;
    }

    if (argv[1][0] == '-') {
        // Handle error if the first argument starts with '-'
        strcat(result_buff, "Error: invalid option\n");
        return;
    }

    for (int i = 1; argv[i] != NULL; i++) {
        if (rmdir(argv[i]) == 0) {
            // If directory removal is successful, print a message
            strcat(result_buff, argv[0]);
            strcat(result_buff, " ");
            strcat(result_buff, argv[i]);
            strcat(result_buff, "\n");
        } else {
            // If directory removal fails, handle error
            strcat(result_buff, "Error: failed to remove ");
            strcat(result_buff, argv[i]);
            strcat(result_buff, "\n");
        }
    }
    return;
}

void CWDcmd(char *result_buff, char **argv) {
    // Initialize result_buff
    memset(result_buff, 0, SEND_BUFF);

    // Check if an argument including an option is provided
    if (argv[1] != NULL && argv[1][0] == '-') {
        // Handle error if an option is provided
        strcat(result_buff, "Error: invalid option\n");
        return;
    }

    // Check if the number of arguments is more than one or "cd" only
    if (count_arguments>2 || (strcmp(argv[0], "cd") == 0 && argv[1] == NULL)) {
        // Handle error if more than one argument is provided or "cd" only
        strcat(result_buff, "Error: too many arguments\n");
        return;
    }

    // Change to the directory provided as an argument
    if (argv[1] == NULL || strcmp(argv[1], "~") == 0) {
        // If no directory is provided or "~" is provided, change to the home directory
        if (chdir(getenv("HOME")) == 0) {
            // If the change is successful
            char cwd[256];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                strcat(result_buff, cwd);
                strcat(result_buff, " is current directory\n");
            } else {
                // Handle error if getcwd function fails
                strcat(result_buff, "Error: failed to get current directory\n");
            }
        } else {
            // If the change fails, handle error
            strcat(result_buff, "Error: failed to change directory\n");
        }
    } else {
        // Change to the directory provided as an argument
        if (chdir(argv[1]) == 0) {
            // If the change is successful
            char cwd[256];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                strcat(result_buff, cwd);
                strcat(result_buff, " is current directory\n");
            } else {
                // Handle error if getcwd function fails
                strcat(result_buff, "Error: failed to get current directory\n");
            }
        } else {
            // If the change fails, handle error
            if (errno == EACCES) {
                // Permission denied error
                strcat(result_buff, "Error: permission denied\n");
            } else if (errno == ENOENT) {
                // Directory not found error
                strcat(result_buff, "Error: directory not found\n");
            } else {
                // Other errors
                strcat(result_buff, "Error: failed to change directory\n");
            }
        }
    }
}

void CDUPcmd(char *result_buff, char **argv) {
    // Initialize result_buff
    memset(result_buff, 0, SEND_BUFF);

    // Check if an argument including an option is provided
    if (argv[1] != NULL && argv[1][0] == '-') {
        // Handle error if an option is provided
        strcat(result_buff, "Error: invalid option\n");
        return;
    }

    // Check if the number of arguments is more than one
    if (argv[1] != NULL) {
        // Handle error if more than one argument is provided
        strcat(result_buff, "Error: too many arguments\n");
        return;
    }

    // Change to the parent directory
    if (chdir("..") == 0) {
        // If the change is successful
        char cwd[256];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            // Print success message and current directory
            strcat(result_buff, "CDUP\n");
            strcat(result_buff, cwd);
            strcat(result_buff, " is current directory\n");
        } else {
            // Handle error if getcwd function fails
            strcat(result_buff, "Error: failed to get current directory\n");
        }
    } else {
        // If the change fails, handle error
        if (errno == EACCES) {
            // Permission denied error
            strcat(result_buff, "Error: permission denied\n");
        } else if (errno == ENOENT) {
            // Directory not found error
            strcat(result_buff, "Error: directory not found\n");
        } else {
            // Other errors
            strcat(result_buff, "Error: failed to change directory\n");
        }
    }
}

void DELEcmd(char *result_buff, char **argv) {
    // Initialize result_buff
    memset(result_buff, 0, SEND_BUFF);

    // Check if the file to delete is specified
    if (argv[1] == NULL) {
        strcat(result_buff, "Error: missing operand\n");
        return;
    }
    // Check if an option is provided as the first argument
    if(argv[1][0]=='-'){
        strcat(result_buff, "Error: invalid option\n");
        return;
    }

    int i = 1;
    while (argv[i] != NULL) {
        struct stat statbuf;
        // Get the status of the file or directory
        if (stat(argv[i], &statbuf) != 0) {
            // If failed to get the status
            strcat(result_buff, "Error: ");
            strcat(result_buff, strerror(errno));
            strcat(result_buff, "\n");
            i++;
            continue;
        }

        // Attempt to delete only if it's a regular file
        if (S_ISREG(statbuf.st_mode)) {
            // Attempt to delete the file
            if (unlink(argv[i]) == 0) {
                // If file deletion is successful
                strcat(result_buff, argv[0]);
                strcat(result_buff, " ");
                strcat(result_buff, argv[i]);
                strcat(result_buff, "\n");
            } else {
                // If file deletion fails
                if (errno == ENOENT) {
                    // If the file doesn't exist
                    strcat(result_buff, "Error: file '");
                    strcat(result_buff, argv[i]);
                    strcat(result_buff, "' does not exist\n");
                    
                } else if (errno == EACCES) {
                    // If permission is denied for the file
                    strcat(result_buff, "Error: permission denied for file '");
                    strcat(result_buff, argv[i]);
                    strcat(result_buff, "'\n");
                    
                } else {
                    // Other errors
                    strcat(result_buff, "Error: ");
                    strcat(result_buff, strerror(errno));
                    strcat(result_buff, "\n");
                    
                }
            }
        } else {
            // If it's a directory, output error
            strcat(result_buff, "Error: '");
            strcat(result_buff, argv[i]);
            strcat(result_buff, "' is a directory, cannot delete\n");
            
        }
        i++;
    }
    return;
}

void RenameCmd(char *result_buff, char **argv) {
    // Initialize result_buff
    memset(result_buff, 0, SEND_BUFF);

    // Check the number of parameters
    if (argv[0] == NULL || argv[1] == NULL || argv[2] == NULL || argv[3] == NULL || argv[4] != NULL) {
        strcat(result_buff, "Error: invalid number of arguments\n");
        return;
    }

    // Check if the old name exists
    if (access(argv[1], F_OK) != 0) {
        strcat(result_buff, "Error: file name does not exist\n");
        return;
    }

    // Check if both RNFR and RNTO are given
    if (strcmp(argv[0], "RNFR") != 0 || strcmp(argv[2], "RNTO") != 0) {
        strcat(result_buff, "Error: invalid command format\n");
        return;
    }

    // Check if both old name and new name are given
    if (argv[1][0] == '\0' || argv[3][0] == '\0') {
        strcat(result_buff, "Error: invalid file names\n");
        return;
    }

    // Check if the new file or directory already exists
    if (access(argv[3], F_OK) == 0) {
        strcat(result_buff, "Error: name to change already exists\n");
        return;
    }

    // Perform file name change
    if (rename(argv[1], argv[3]) != 0) {
        strcat(result_buff, "Error renaming file: ");
        strcat(result_buff, strerror(errno));
        strcat(result_buff, "\n");
        return;
    }

    // Output success message
    strcat(result_buff, argv[0]);
    strcat(result_buff, " ");
    strcat(result_buff, argv[1]);
    strcat(result_buff, "\n");
    strcat(result_buff, argv[2]);
    strcat(result_buff, " ");
    strcat(result_buff, argv[3]);
    strcat(result_buff, "\n");
}


void LISTcmd(char *result_buff, char **argv) {
    memset(result_buff, 0, SEND_BUFF);
    // Check if the number of arguments exceeds one
    // Check if the number of arguments exceeds one

    if (count_arguments>2) {
        strcat(result_buff, "Error: too many arguments\n");
       return;
    }

    // Check if an option is provided as the first argument
    if (argv[1] != NULL && argv[1][0] == '-') {
        strcat(result_buff, "Error: invalid option\n");
        return;
    }

    // If no directory is specified, work in the current directory
    char *dirname = (argv[1] == NULL) ? "." : argv[1];

    // Open the directory
    struct dirent **namelist;
    int n;

    n = scandir(dirname, &namelist, NULL, compare);
    if (n < 0) {
        //if file case
        if(errno == ENOTDIR){
        strcat(result_buff, "Error: is not directory\n");
        return;
    
    }
    else if (errno == EACCES) { // If access is restricted
        strcat(result_buff, "Error: Permission denied\n");
        return;
        }
        
    
    else if (errno == ENOENT) { // no exist file or directory
            strcat(result_buff, "Error: Does not exist\n");
            return;
        }
    else{
        perror(result_buff + strlen(result_buff));
            return;
    }   
    }

     // Check if the directory is accessible
    if (access(dirname, R_OK) == -1) {
        strcat(result_buff, "Error: Permission denied\n");
        return;
    }

    // Output the sorted list
    for (int i = 0; i < n; i++) {
        // Get file details
        struct stat fileStat;
        char path[1024];
        sprintf(path, "%s/%s", dirname, namelist[i]->d_name);
        if (lstat(path, &fileStat) < 0) {
            perror(result_buff + strlen(result_buff));
            continue;
        }

        // Output file type and permissions
        char permissions[11];
        sprintf(permissions, "%s%s%s%s%s%s%s%s%s%s",
                (S_ISDIR(fileStat.st_mode)) ? "d" : "-",
                (fileStat.st_mode & S_IRUSR) ? "r" : "-",
                (fileStat.st_mode & S_IWUSR) ? "w" : "-",
                (fileStat.st_mode & S_IXUSR) ? "x" : "-",
                (fileStat.st_mode & S_IRGRP) ? "r" : "-",
                (fileStat.st_mode & S_IWGRP) ? "w" : "-",
                (fileStat.st_mode & S_IXGRP) ? "x" : "-",
                (fileStat.st_mode & S_IROTH) ? "r" : "-",
                (fileStat.st_mode & S_IWOTH) ? "w" : "-",
                (fileStat.st_mode & S_IXOTH) ? "x" : "-");
        strcat(result_buff, permissions);
        strcat(result_buff, " ");

        // Output link count
        char linkCount[32];
        int linkLength = sprintf(linkCount, "%lu", fileStat.st_nlink);
        strcat(result_buff, linkCount);
        strcat(result_buff, " ");

        // Output owner and group
        struct passwd *pwd = getpwuid(fileStat.st_uid);
        struct group *grp = getgrgid(fileStat.st_gid);
        strcat(result_buff, pwd->pw_name);
        strcat(result_buff, " ");
        strcat(result_buff, grp->gr_name);
        strcat(result_buff, " ");

        // Output file size
        char fileSize[32];
        int sizeLength = sprintf(fileSize, "%lu", fileStat.st_size);
        strcat(result_buff, fileSize);
        strcat(result_buff, " ");

        // Output modification time
        char date[100];
        strftime(date, sizeof(date), "%b %e %H:%M", localtime(&fileStat.st_mtime));
        strcat(result_buff, date);
        strcat(result_buff, " ");

        // Output file name
        strcat(result_buff, namelist[i]->d_name);

        // Add '/' if it's a directory
        if (S_ISDIR(fileStat.st_mode)) {
            strcat(result_buff, "/");
        }

        strcat(result_buff, "\n");

        free(namelist[i]);
    }

    // Free memory and close directory
    free(namelist);
}
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
                execute_NLST_a(result_buff, args);
            }
            // NLST command with "-l" option
            else if (strcmp(args[1], "-l") == 0) {
                count_arguments=0;
                for (int i = 0; i < argc; i++) {
                write(1, args[i], strlen(args[i]));
                write(1, " ", 1);
                count_arguments++;
                }
                execute_NLST_l(result_buff,args);
            }   
            // NLST command with "-al" option
            else if (strcmp(args[1], "-al") == 0) {
                count_arguments=0;
                for (int i = 0; i < argc; i++) {
                write(1, args[i], strlen(args[i]));
                write(1, " ", 1);
                 count_arguments++;
                }
                execute_NLST_al(result_buff,args);
            }

            // NLST command with path argument
            else if(argc==2 && args[1][0]!='-'){
                for (int i = 0; i < argc; i++) {
                write(1, args[i], strlen(args[i]));
                write(1, " ", 1);
                }
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
                }
                else{
                     // Too many path arguments
                    for (int i = 0; i < argc; i++) {
                    write(1, args[i], strlen(args[i]));
                    write(1, " ", 1);
                    strcpy(result_buff, "too many path arguments\n");
                    }
                }
            }

        }
        // NLST command without options or arguments
         else if(argc==1) {
            write(1,"NLST",strlen("NLST"));
            //write(1,"\n",1);
            execute_NLST(result_buff,args);
        }

         // Too many arguments case
        else{
            for (int i = 0; i < argc; i++) {
                write(1, args[i], strlen(args[i]));
                write(1, " ", 1);
            strcpy(result_buff, "too many arguments\n");
        }
        }
    }
    
    // process the PWD command
    else if(strcmp(args[0],"PWD")==0){
        for (int i = 0; i < argc; i++) {
                    write(1, args[i], strlen(args[i]));
                    write(1, " ", 2);
                    }
        PWDcmd(result_buff,args);
    }

    // process the MKD command
    else if(strcmp(args[0],"MKD")==0){
          for (int i = 0; i < argc; i++) {
                    write(1, args[i], strlen(args[i]));
                    write(1, " ", 2);
                    }
        MKDcmd(result_buff,args);
    }
      // process the RMD command
    else if(strcmp(args[0],"RMD")==0){
          for (int i = 0; i < argc; i++) {
                 write(1, args[i], strlen(args[i]));
                 write(1, " ", 2);
                }
        RMDcmd(result_buff,args);
    }

      // process the DELE command
    else if(strcmp(args[0],"DELE")==0){
          for (int i = 0; i < argc; i++) {
                 write(1, args[i], strlen(args[i]));
                 write(1, " ", 2);
                }
        DELEcmd(result_buff,args);
    }
          // process the Rename command
    else if(strcmp(args[0], "RNFR") == 0 && strcmp(args[2], "RNTO") == 0) {
          for (int i = 0; i < argc; i++) {
                 write(1, args[i], strlen(args[i]));
                 write(1, " ", 2);
                }
        RenameCmd(result_buff,args);
    }

    //rename exception
    else if(strcmp(args[0], "rename") == 0 ) {
         for (int i = 0; i < argc; i++) {
                 write(1, args[i], strlen(args[i]));
                 write(1, " ", 2);
                }
            strcat(result_buff,"rename is err :Two arguments are required\n");
    }
          // process the LISTcommand
    else if(strcmp(args[0], "LIST") == 0 ) {
        count_arguments=0;
         for (int i = 0; i < argc; i++) {
                 write(1, args[i], strlen(args[i]));
                 write(1, " ", 2);
                 count_arguments++;
                }
         LISTcmd(result_buff,args);
    }

     // Process the QUIT command
    else if (strcmp(args[0], "QUIT") == 0 && args[1]!=NULL) {
        if(argc>1){
            // QUIT command with additional arguments
            if(args[1][0]=='-'){
                 for (int i = 0; i < argc; i++) {
                    write(1, args[i], strlen(args[i]));
                    write(1, " ", 1);
                strcpy(result_buff, "QUIT is Not option\n");
                }
            }
            else{
                // QUIT command with arguments
                    for (int i = 0; i < argc; i++) {
                    write(1, args[i], strlen(args[i]));
                    write(1, " ", 1);
                    }
                    strcpy(result_buff, "QUIT is No argument\n");
            }
        }
        // QUIT command without additional arguments
        else{
        write(1,"QUIT\n",strlen("QUIT\n"));
        }
    }
          // process the cwd command
    else if(strcmp(args[0],"CWD")==0){
        count_arguments=0;
          for (int i = 0; i < argc; i++) {
                    write(1, args[i], strlen(args[i]));
                    write(1, " ", 2);
                    count_arguments++;
                    }
        CWDcmd(result_buff,args);
    }
    // process the cdup command
    else if(strcmp(args[0],"CDUP")==0){
          for (int i = 0; i < argc; i++) {
                write(1, args[i], strlen(args[i]));
                write(1, " ", 2);
                }
        CDUPcmd(result_buff,args);
    }

     // Unknown command
    else  {
        for (int i = 0; i < argc; i++) {
            write(1, args[i], strlen(args[i]));
            write(1, " ", 1);
            };
        strcpy(result_buff, "Invalid command\n");
    }

    return 0;
}

void sh_chld(int); //signal handler for SIGHLD
void sh_alrm(int);//signal handler for SIGALRM
void sh_int(int);//signal handler for ctrl+c

///////////////////////////main function//////////////////////////////
int main(int argc, char **argv) {
    int server_fd, client_fd, n;
    struct sockaddr_in server_addr, client_addr;
    char buff[MAX_BUFF], result_buff[SEND_BUFF];
    int len;
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

    // Register signal handlers
    signal(SIGALRM, sh_alrm);
    signal(SIGCHLD, sh_chld);
    signal(SIGINT, sh_int);

    // Socket Creation
    // Create a socket for the server
    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Server Address Configuration
    // Set up the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1])); // server fort.

    // Bind Socket to Address
    // Bind the socket to the server address
    if ((bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))) != 0) {
        perror("socket bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for Connection Requests
    // Start listening for incoming connections
    if ((listen(server_fd, 5)) != 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    // Accept Client Connections
    // Accept incoming client connections in a loop

    
    while(1){
        
        pid_t pid;
        len=sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
        if (client_fd < 0) {
            perror("server accept failed");
            exit(EXIT_FAILURE);
        }


          time_t current_time = time(NULL); // Get current time

        pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }

       if (pid == 0) {  // child process
             alarm(0);
            while (1) {
                n = read(client_fd, buff, MAX_BUFF);
                if (n <= 0) {
                    perror("read error or client closed connection");
                    close(client_fd);
                    exit(EXIT_SUCCESS);  // child exits
                }
                buff[n] = '\0';

                // Handle QUIT Command
                // If QUIT command is received, close the connection
                if (strcmp(buff, "QUIT") == 0) {
                   exit(0);
                   
                }

                cmd_process(buff, result_buff);
                printf(" [Child PID :%d]\n",getpid());


                n = write(client_fd, result_buff, strlen(result_buff) + 1);
                if (n < 0) {
                    perror("write error");
                    close(client_fd);
                    exit(EXIT_FAILURE);  // child exits
                }
              

            }
        } else {  // parent process
             // Print Client Information
            // Print information about the connected client
            if (client_info(&client_addr) < 0) {
                write(STDERR_FILENO, "client_info() err!!\n", sizeof("client_info() err!!\n"));
            }
            char C_buff[BUF_SIZE]; //Child buff
            sprintf(C_buff, "Child Process ID : %d\n", pid);
            write(STDOUT_FILENO, C_buff, strlen(C_buff));
            //add client and print client information
             add_client(pid, ntohs(client_addr.sin_port), current_time);
             print_connected_clients();
             alarm(10);
        }
         close(client_fd);
    
    }
    close(server_fd);
    return 0;
}
//////////////////////main fun END/////////////////////////////

void sh_chld(int signum) {
    pid_t wait_pid;
    int status;

    // Call the wait() function to wait for the terminated child process
    while ((wait_pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("Client( %d) s Release\n", wait_pid);
        remove_client(wait_pid);
    }
    //if waitpid err
    if (wait_pid == -1) {
        if (errno != ECHILD) {
            perror("waitpid error");
            exit(EXIT_FAILURE);
        }
    }
}


void sh_alrm(int signum) {
    //alarm call ->print client information
   print_connected_clients();
   alarm(10);
}


void sh_int(int signum){
    //if ctrl+c ->exit
    exit(0);
}