///////////////////////////////////////////////////////////////////////
// File Name : srv.c                                               //
// Date : 2024/06/01                                                //
// OS : Ubuntu 20.04.6 LTS 64bits                                   //
//                                                                  //
// Author : Hong Wang ki                                            //
// Student ID : 2020202060                                          //
// ----------------------------------------------------------------- //
// Title : System Programming Assignment #3-3 (ftp server)           //
// Description: Final ftp server implementation completed              //
///////////////////////////////////////////////////////////////////////
// Function: client_info, execute_NLST normal -l -al -a,cmd_process, main//
//           cwd,pwd,cdup,renmae,list,rmdir,mkdir cmd fun             //
//             compare, log command , convert_str_to_add  RETR STORcmd//                 
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
//          When the program receives 12 instructions from the client,                                  //
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
#include <fcntl.h>

#define MAX_BUFF 5012
#define SEND_BUFF 5012
#define MAX_BUF 50
#define BUF_SIZE 100
#define MAX_MESSAGE_LENGTH 100
#define MAX_USERS 10
#define SA struct sockaddr

size_t bytes_read;
pid_t save_pid; // Declare as a global variable
int count_arguments=0;
int err=0;
 char *g_client_ip;
 uint16_t g_client_port;
char *g_user;
char PutBuff[MAX_BUFF];
size_t bytes_written;
int putCheak=0;

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

size_t bytes_read;
size_t bytes_read1;
char TypeChoose[1]={'x'};

////////////////// log fun///////////////////////
void log_command(char *comment) {
    static FILE *logfile = 0;
    if(logfile == 0)
    //logfile open
        logfile = fopen("logfile", "a");
    // time setting
    time_t current_time = time(NULL);
    struct tm *local_time = localtime(&current_time);

    // current time format
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);

    //log file doesnt opent
    if (logfile == NULL) {
        write(1,"log file doesn't open.\n",strlen("log file doesn't open.\n"));
        return;
    }


    //write log
    fprintf(logfile, "%s | [%s:%hu] | %s | %s\n", time_str, g_client_ip, g_client_port, g_user, comment);
    fflush(logfile);
} 
///////////////////////////log fun end//////////////////////////////




// Function to convert IP address and port number from FTP PORT command format
char* convert_str_to_addr( char *str, unsigned int *port) {
    static char addr[32];
    unsigned int ip[4]; // Changed to unsigned int array
     unsigned int p1, p2; // Changed to unsigned int
    //printf("str: %s\n",str);
    // Corrected format specifier to match the type of ip array
    sscanf(str, "PORT %u,%u,%u,%u,%d,%d", &ip[0], &ip[1], &ip[2], &ip[3], &p1, &p2);
    log_command(str);

    // Using snprintf to prevent buffer overflow
    snprintf(addr, sizeof(addr), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    *port = (p1 << 8) | p2;

    return addr;
}
//////////////////////convert_str_to_addr fun end//////////////////////////


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
            
            strcpy(result_buff, "550 Directory does not exist\n");
            log_command("550 Directory does not exist");
            return; // If not exist return
        }

        // Access check
        if (access(path, R_OK) == -1 && (stat(path, &st) == 0 && S_ISDIR(st.st_mode))) {
            // Send Permission denied ->cli.c
            strcpy(result_buff, "550 Permission denied\n");
            log_command("550 Permission denied");
            return; // If not access return.
        }
        // Open dir
        dir = opendir(path);
    } else {
        // Directory is current
        dir = opendir(".");
    }

    if (dir == NULL) {
    strcat(result_buff, "550 is not directory\n");
    log_command("550 is not directory");
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
        bytes_read=strlen(result_buff);
        
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
            strcat(result_buff, "550 is not directory\n");
            log_command("550 is not directory");
            return;
        } else if (errno == EACCES) { // If access is restricted
            strcat(result_buff, "550 Permission denied\n");
            log_command("550 Permission denied");
            return;
        } else if (errno == ENOENT) { // Non-existent files or directories
            strcat(result_buff, "550 Does not exist\n");
            log_command("550 Does not exist");
            return;
        } else {
            perror(result_buff + strlen(result_buff));
            return;
        }
    }

    // Check if the directory is accessible
    if (access(dirname, R_OK) == -1) {
        strcat(result_buff, "550 Permission denied\n");
        log_command("550 Permission denied");
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
        bytes_read=strlen(result_buff);
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
            strcpy(result_buff, "550 Directory does not exist\n");
            log_command("550 Directory does not exist");
            return; // If not exist return
        }

        // Access check
           // Access check
        if (access(path, R_OK) == -1 && (stat(path, &st) == 0 && S_ISDIR(st.st_mode))) {
            // Send Permission denied ->cli.c
            strcpy(result_buff, "550 Permission denied\n");
            log_command("550 Permission denied");
            return; // If not access return.
        }
        // Open dir
        dir = opendir(path);
    } else {
        // Directory is current
        dir = opendir(".");
    }

       if (dir == NULL) {
        strcat(result_buff, "550 is not directory\n");
        log_command("550 is not directory");
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
        bytes_read=strlen(result_buff);
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
        strcat(result_buff, "501 too many arguments\n");
        log_command("501 too many arguments");
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
        strcat(result_buff, "550 is not directory\n");
        log_command("550 is not directory");
        return;
    }
    else if (errno == EACCES) {// If access is restricted
        strcat(result_buff, "550 Permission denied\n");
        log_command("550 Permission denied");
        return;
        }
        
    
    else if (errno == ENOENT) { // // Non-existent files or directories
            strcat(result_buff, "550 Does not exist\n");
            log_command("550 Does not exist");
            return;
        }
    else{
        perror(result_buff + strlen(result_buff));
            return;
    }   
    }

     // Check if the directory is accessible
    if (access(dirname, R_OK) == -1) {
        strcat(result_buff, "550 Permission denied\n");
        log_command("550 Permission denied");
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
        bytes_read=strlen(result_buff);
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

    
    if (argv[1] != NULL && argv[1][0] == '-') {
        // If there's an option, handle error
        strcpy(result_buff, "501: invalid option\n");
        log_command("550: invalid option");
    } else if (argv[1] != NULL) {
        // If there's an argument, handle error
        strcpy(result_buff, "501: argument is not required\n");
        log_command("550: argument is not required");
    } else {
        // If neither option nor argument is provided, print the current directory
        char cwd[256];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            // Write current directory to result_buff
            strcat(result_buff, "257 ");
            strcat(result_buff, cwd);
            strcat(result_buff, " is current directory");
            log_command(result_buff); 
            strcat(result_buff,"\n");
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
        strcat(result_buff, "550: argument is required\n");
         log_command("550: argument is required");
        return;
    }
    if (argv[1][0] == '-') {
        // If the first argument starts with '-', handle error
        strcat(result_buff, "502: invalid option\n");
        log_command("502: invalid option");
        return;
    }
    char Arr1[100];

    // Create each directory provided as an argument
    for (int i = 1; argv[i] != NULL; i++) {
        if (mkdir(argv[i], 0775) == 0) {
            //memset(result_buff,0,sizeof(result_buff));
            // If directory creation is successful, write to result_buff
            strcat(result_buff,"250 MKD command performed successfully.\n");
             log_command("250 MKD command performed successfully.");
        } else {
           // memset(result_buff,0,sizeof(result_buff));
            // If the directory already exists, handle error
            strcat(result_buff, "550: cannot create directory '");
            strcat(result_buff, argv[i]);
            strcat(result_buff, "' : File exists");
             sprintf(Arr1, "550: failed to remove %s", argv[i]);
              log_command(Arr1);
              memset(Arr1,0,sizeof(Arr1));
            strcat(result_buff,"\n");
        }
    }
}

void RMDcmd(char *result_buff, char **argv) {
    // Initialize result_buff
    memset(result_buff, 0, SEND_BUFF);

    if (argv[1] == NULL) {
        // Handle error if there's no argument
        strcat(result_buff, "550: argument is required\n");
        log_command("550: argument is required");
        return;
    }

    if (argv[1][0] == '-') {
        // Handle error if the first argument starts with '-'
        strcat(result_buff, "502: invalid option\n");
         log_command("502: invalid option");
        return;
    }

    for (int i = 1; argv[i] != NULL; i++) {
        char copyArr[100];
        if (rmdir(argv[i]) == 0) {
            //memset(result_buff,0,sizeof(result_buff));
            // If directory removal is successful, print a message
             strcat(result_buff,"250 RMD command performed successfully.\n");
             log_command("250 RMD command performed successfully.");
        } else {
            //memset(result_buff,0,sizeof(result_buff));
            // If directory removal fails, handle error
            strcat(result_buff, "550: failed to remove ");
            strcat(result_buff, argv[i]);
            sprintf(copyArr, "550: failed to remove %s", argv[i]);
              log_command(copyArr);
              memset(copyArr,0,sizeof(copyArr));
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
        strcat(result_buff, "501 : invalid option\n");
        log_command("501 : invalid option");
        return;
    }

    // Check if the number of arguments is more than one or "cd" only
    if (count_arguments>2 || (strcmp(argv[0], "cd") == 0 && argv[1] == NULL)) {
        // Handle error if more than one argument is provided or "cd" only
        strcat(result_buff, "501: too many arguments\n");
        log_command("501: too many arguments");
        return;
    }

    // Change to the directory provided as an argument
    if (argv[1] == NULL || strcmp(argv[1], "~") == 0) {
        // If no directory is provided or "~" is provided, change to the home directory
        if (chdir(getenv("HOME")) == 0) {
            // If the change is successful
            char cwd[256];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                memset(result_buff, 0, SEND_BUFF);
                strcat(result_buff, "250 CWD command performed successfully.\n");
                log_command("250 CWD command performed successfully.");

            } else {
                // Handle error if getcwd function fails
                strcat(result_buff, "550 failed to get current directory\n");
                log_command("550 failed to get current directory");
            }
        } else {
            // If the change fails, handle error
            strcat(result_buff, "550 failed to change directory\n");
            log_command("550 failed to change directory");
        }
    } else {
        // Change to the directory provided as an argument
        if (chdir(argv[1]) == 0) {
            // If the change is successful
            char cwd[256];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                memset(result_buff, 0, SEND_BUFF);
                strcat(result_buff, "250 CWD command performed successfully.\n");
                log_command("250 CWD command performed successfully");

            } else {
                // Handle error if getcwd function fails
                strcat(result_buff, "550: failed to get current directory\n");
                log_command("550: failed to get current directory");
            }
        } else {
            // If the change fails, handle error
            if (errno == EACCES) {
                // Permission denied error
                strcat(result_buff, "550: permission denied\n");
                log_command("550: permission denied");
            } else if (errno == ENOENT) {
                // Directory not found error
                strcat(result_buff, "550: directory not found\n");
                log_command("550: directory not found");
            } else {
                // Other errors
                strcat(result_buff, "550: failed to change directory\n");
                log_command("550: failed to change directory");
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
        strcat(result_buff, "501: invalid option\n");
        log_command("501: invalid option");
        return;
    }

    // Check if the number of arguments is more than one
    if (argv[1] != NULL) {
        // Handle error if more than one argument is provided
        strcat(result_buff, "501: too many arguments\n");
        log_command("501: too many arguments");
        return;
    }

    // Change to the parent directory
    if (chdir("..") == 0) {
        // If the change is successful
        char cwd[256];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            // Print success message and current directory
            memset(result_buff, 0, SEND_BUFF);
            strcat(result_buff, "250 CDUP command performed successfully.\n");
            log_command("250 CDUP command performed successfully.");
        } else {
            // Handle error if getcwd function fails
            strcat(result_buff, "550: failed to get current directory\n");
            log_command("550: failed to get current directory");
        }
    } else {
        // If the change fails, handle error
        if (errno == EACCES) {
            // Permission denied error
            strcat(result_buff, "550: permission denied\n");
            log_command("550: permission denied");
        } else if (errno == ENOENT) {
            // Directory not found error
            strcat(result_buff, "550: directory not found\n");
            log_command("550: directory not found");
        } else {
            // Other errors
            strcat(result_buff, "550: failed to change directory\n");
            log_command("550: failed to change directory");
        }
    }
}

void DELEcmd(char *result_buff, char **argv) {
    // Initialize result_buff
    memset(result_buff, 0, SEND_BUFF);

    // Check if the file to delete is specified
    if (argv[1] == NULL) {
        strcat(result_buff, "550: missing operand\n");
        log_command("550: missing operand");
        return;
    }
    // Check if an option is provided as the first argument
    if(argv[1][0]=='-'){
        strcat(result_buff, "501: invalid option\n");
        log_command("501: invalid option");
        return;
    }

    int i = 1;
    while (argv[i] != NULL) {
        struct stat statbuf;
        // Get the status of the file or directory
        if (stat(argv[i], &statbuf) != 0) {
            // If failed to get the status
            strcat(result_buff, "550: ");
            strcat(result_buff, strerror(errno));
            log_command(result_buff);
            strcat(result_buff, "\n");
            i++;
            continue;
        }

        // Attempt to delete only if it's a regular file
        if (S_ISREG(statbuf.st_mode)) {
            // Attempt to delete the file
            if (unlink(argv[i]) == 0) {
                // If file deletion is successful
                strcat(result_buff,"250 DELE command performed successfully.\n");
                log_command("250 DELE command performed successfully.");

            } else {
                // If file deletion fails
                if (errno == ENOENT) {
                    // If the file doesn't exist
                    strcat(result_buff, "550: file '");
                    strcat(result_buff, argv[i]);
                    strcat(result_buff, "' does not exist");
                    log_command(result_buff);
                    strcat(result_buff,"\n");
                    
                } else if (errno == EACCES) {
                    // If permission is denied for the file
                    strcat(result_buff, "550: permission denied for file '");
                    strcat(result_buff, argv[i]);
                     log_command(result_buff);
                    strcat(result_buff, "'\n");
                    
                } else {
                    // Other errors
                    strcat(result_buff, "550: ");
                    strcat(result_buff, strerror(errno));
                     log_command(result_buff);
                    strcat(result_buff, "\n");
                    
                }
            }
        } else {
            // If it's a directory, output error
            strcat(result_buff, "550: '");
            strcat(result_buff, argv[i]);
            strcat(result_buff, "' is a directory, cannot delete");
             log_command(result_buff);
             strcat(result_buff,"\n");
            
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
        strcat(result_buff, "550: invalid number of arguments\n");
        log_command(result_buff);
        return;
    }

    // Check if the old name exists
    if (access(argv[1], F_OK) != 0) {
        strcat(result_buff, "550: file name does not exist\n");
        log_command(result_buff);
        return;
    }

    // Check if both RNFR and RNTO are given
    if (strcmp(argv[0], "RNFR") != 0 || strcmp(argv[2], "RNTO") != 0) {
        strcat(result_buff, "550: invalid command format\n");
        log_command(result_buff);
        return;
    }

    // Check if both old name and new name are given
    if (argv[1][0] == '\0' || argv[3][0] == '\0') {
        strcat(result_buff, "550: invalid file names\n");
        log_command(result_buff);
        return;
    }

    // Check if the new file or directory already exists
    if (access(argv[3], F_OK) == 0) {
        strcat(result_buff, "350: name to change already exists\n");
        log_command(result_buff);
        return;
    }

    // Perform file name change
    if (rename(argv[1], argv[3]) != 0) {
        strcat(result_buff, "550 renaming file: ");
        strcat(result_buff, strerror(errno));
        log_command(result_buff);
        strcat(result_buff, "\n");
        return;
    }

    // Output success message
    strcat(result_buff,"350 File exists, ready to rename\n");
    log_command("350 File exists, ready to rename");
    strcat(result_buff, "250 RNTO command performed successfully.\n");
    log_command("250 RNTO command performed successfully.");
}
void RETR(char *result_buff, char **argv) {
    // Clear result_buff
    memset(result_buff, 0, SEND_BUFF);

    // Get the requested file path from arguments
    char *filename = argv[1];

    // Open the file in the appropriate mode
    FILE *file;
    if (TypeChoose[0] == 'A') {
        file = fopen(filename, "r");  // ASCII mode
    } else if (TypeChoose[0] == 'I') {
        file = fopen(filename, "rb");  // Binary mode
    } else {
        file = fopen(filename, "rb");  // Default to binary mode
    }

    if (file == NULL) {
        perror("Error opening file");
        strcpy(result_buff, "550 File not found\n");
        log_command("550 File not found");
        return;
    }

    // Determine the file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Check if there is enough space in result_buff
    if (file_size >= SEND_BUFF) {
        strcpy(result_buff, "550 File too large to fit in buffer\n");
        log_command(result_buff);
        fclose(file);
        return;
    }

    // Read the file content into result_buff
     bytes_read = fread(result_buff, 1, file_size, file);
    if (bytes_read != file_size) {
        perror("Error reading file");
        strcpy(result_buff, "550 File read error\n");
        log_command(result_buff);
        fclose(file);
        return;
    }

    // Null-terminate the buffer (optional, based on usage)
    result_buff[bytes_read] = '\0';

    // Close the file
    fclose(file);

}


void STORcmd(const char *data, const char *filename, char mode) {
    const char *file_mode = (mode == 'A') ? "w" : "wb"; // 'A'=ascii ,else binary
    FILE *file;
        file = fopen(filename, file_mode); // file open
    //if file doesnt open case
    if (file == NULL) {
        fprintf(stderr, "550 opening file for writing.\n");
        return;
    }
    
    //write file
    bytes_written = fwrite(data, sizeof(char), strlen(data), file);
    if (bytes_written != strlen(data)) {
        fprintf(stderr, "550 writing data to file.\n");
    }
    
    fclose(file); // file close
    
}




void LISTcmd(char *result_buff, char **argv) {
    memset(result_buff, 0, SEND_BUFF);
    // Check if the number of arguments exceeds one
    // Check if the number of arguments exceeds one

    if (count_arguments>2) {
        strcat(result_buff, "501 too many arguments\n");
        log_command("501 too many arguments");
       return;
    }

    // Check if an option is provided as the first argument
    if (argv[1] != NULL && argv[1][0] == '-') {
        strcat(result_buff, "501 invalid option\n");
        log_command("501 invalid option");
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
        strcat(result_buff, "550 is not directory\n");
        log_command("550 is not directory");
        return;
    
    }
    else if (errno == EACCES) { // If access is restricted
        strcat(result_buff, "550 Permission denied\n");
        log_command("550 Permission denied");
        return;
        }
        
    
    else if (errno == ENOENT) { // no exist file or directory
            strcat(result_buff, "550: Does not exist\n");
            log_command("550: Does not exist");
            return;
        }
    else{
        perror(result_buff + strlen(result_buff));
            return;
    }   
    }

     // Check if the directory is accessible
    if (access(dirname, R_OK) == -1) {
        strcat(result_buff, "550: Permission denied\n");
        log_command("550: Permission denied");
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
         bytes_read=strlen(result_buff);

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

    char message[MAX_MESSAGE_LENGTH]; // message buff

    //message= recived: +buff
    snprintf(message, sizeof(message), "received: %s", buff);

    // log write
    log_command(message);
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
                write(1,"\n",1);
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
                write(1,"\n",1);
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
                write(1,"\n",1);
            }

            // NLST command with path argument
            else if(argc==2 && args[1][0]!='-'){
                for (int i = 0; i < argc; i++) {
                write(1, args[i], strlen(args[i]));
                write(1, " ", 1);
                }
                execute_NLST(result_buff,args);
                write(1,"\n",1);
            }
        
        
            else {
                 // Invalid options
                if(args[1][0]=='-'){
                    for (int i = 0; i < argc; i++) {
                    write(1, args[i], strlen(args[i]));
                    write(1, " ", 1);
                    strcpy(result_buff, "501 Invalid option\n");
                    log_command("501 Invalid option");
                    }
                    write(1,"\n",1);
                    
                }
                else{
                     // Too many path arguments
                    for (int i = 0; i < argc; i++) {
                    write(1, args[i], strlen(args[i]));
                    write(1, " ", 1);
                    strcpy(result_buff, "501 too many path arguments\n");
                    log_command("501 too many path arguments");
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
        }
        strcpy(result_buff, "501 too many arguments\n");
        log_command("501 too many arguments");
        write(1,"\n",1);
        }
    }
    
    // process the PWD command
    else if(strcmp(args[0],"PWD")==0){
        for (int i = 0; i < argc; i++) {
                    write(1, args[i], strlen(args[i]));
                    write(1, " ", 2);
                    }
                    write(1,"\n",1);
        PWDcmd(result_buff,args);
    }

    // process the MKD command
    else if(strcmp(args[0],"MKD")==0){
          for (int i = 0; i < argc; i++) {
                    write(1, args[i], strlen(args[i]));
                    write(1, " ", 2);
                    }
                    write(1,"\n",1);
        MKDcmd(result_buff,args);
    }
      // process the RMD command
    else if(strcmp(args[0],"RMD")==0){
          for (int i = 0; i < argc; i++) {
                 write(1, args[i], strlen(args[i]));
                 write(1, " ", 2);
                }
                write(1,"\n",1);
        RMDcmd(result_buff,args);
    }

      // process the DELE command
    else if(strcmp(args[0],"DELE")==0){
          for (int i = 0; i < argc; i++) {
                 write(1, args[i], strlen(args[i]));
                 write(1, " ", 2);
                }
                write(1,"\n",1);
        DELEcmd(result_buff,args);
    }
          // process the Rename command
    else if(strcmp(args[0], "RNFR") == 0 && strcmp(args[2], "RNTO") == 0) {
          for (int i = 0; i < argc; i++) {
                 write(1, args[i], strlen(args[i]));
                 write(1, " ", 2);
                }
                write(1,"\n",1);
        RenameCmd(result_buff,args);
    }

    //rename exception
    else if(strcmp(args[0], "rename") == 0 ) {
         for (int i = 0; i < argc; i++) {
                 write(1, args[i], strlen(args[i]));
                 write(1, " ", 2);
                }
            strcat(result_buff,"550 rename is err :Two arguments are required\n");
            log_command("550 rename is err :Two arguments are required");
    }
          // process the LISTcommand
    else if(strcmp(args[0], "LIST") == 0 ) {
        count_arguments=0;
         for (int i = 0; i < argc; i++) {
                 write(1, args[i], strlen(args[i]));
                 write(1, " ", 2);
                 count_arguments++;
                }
                 write(1,"\n",1);
         LISTcmd(result_buff,args);
    }

    else if(strcmp(args[0],"RETR")==0){
          for (int i = 0; i < argc; i++) {
                    write(1, args[i], strlen(args[i]));
                    write(1, " ", 2);
                    }
                    write(1,"\n",1);
        RETR(result_buff,args);
    }

     else if(strcmp(args[0],"STOR")==0){
        for (int i = 0; i < argc; i++) {
                    write(1, args[i], strlen(args[i]));
                    write(1, " ", 2);
                    }
                    write(1,"\n",1);
        //STORcmd(result_buff,args);
        if (argc!=2) {
            putCheak=1;
         } 
    }



    else if (strcmp(args[0], "TYPE") == 0 && strcmp(args[1], "I") == 0) {
    // print command
    for (int i = 0; args[i] != NULL; i++) {
        write(1, args[i], strlen(args[i]));
        write(1, " ", 1);
    }
    write(1, "\n", 1);

    // "TYPE I" after more argument case
    if (args[2] != NULL) {
        strcpy(result_buff, "502 Type doesn't set.\n");
        log_command("502 Type doesn't set.");
    } else {
        strcpy(result_buff, "201 Type set to I.\n");
        log_command("201 Type set to I.");
        TypeChoose[0] = 'I';
        write(1,result_buff,strlen(result_buff));

    }
    }

    else if (strcmp(args[0], "TYPE") == 0 && strcmp(args[1], "A") == 0) {
   // print command
    for (int i = 0; args[i] != NULL; i++) {
        write(1, args[i], strlen(args[i]));
        write(1, " ", 1);
    }
    write(1, "\n", 1);

   // "TYPE A" after more argument case
    if (args[2] != NULL) {
         strcpy(result_buff, "502 Type doesn't set.\n");
         log_command("502 Type doesn't set.");
    } else {
        strcpy(result_buff, "201 Type set to A.\n");
        log_command("201 Type set to A.");
        TypeChoose[0] = 'A';
        write(1,result_buff,strlen(result_buff));
        
    }
    }



     // Process the QUIT command
    else if (strcmp(args[0], "QUIT") == 0 && args[1]!=NULL) {
        if(argc>1){
            // QUIT command with additional arguments
            if(args[1][0]=='-'){
                 for (int i = 0; i < argc; i++) {
                    write(1, args[i], strlen(args[i]));
                    write(1, " ", 1);
                strcpy(result_buff, "501 QUIT is Not option\n");
                }
                 log_command("501 QUIT is Not option");
            write(1,"\n",1);
            }
            else{
                // QUIT command with arguments
                    for (int i = 0; i < argc; i++) {
                    write(1, args[i], strlen(args[i]));
                    write(1, " ", 1);
                    }
                    write(1,"\n",1);
                    strcpy(result_buff, "550 QUIT is No argument\n");
                    log_command("550 QUIT is No argument");
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
                    write(1,"\n",1);
        CWDcmd(result_buff,args);
    }
    // process the cdup command
    else if(strcmp(args[0],"CDUP")==0){
          for (int i = 0; i < argc; i++) {
                write(1, args[i], strlen(args[i]));
                write(1, " ", 2);
                }
                 write(1,"\n",1);
        CDUPcmd(result_buff,args);
    }

     // Unknown command
    else  {
        for (int i = 0; i < argc; i++) {
            write(1, args[i], strlen(args[i]));
            write(1, " ", 1);
            }
             write(1,"\n",1);
        strcpy(result_buff, "501 Syntax Error\n");
        log_command("501 Syntax Error");
    }

    return 0;
}
///////////////cmd_process fun end///////////////


int main(int argc, char **argv) {
    int server_fd, client_fd, n;
    int data_sockfd = -1, data_conn;
    struct sockaddr_in server_addr, client_addr, data_servaddr;
    char buff[MAX_BUFF], result_buff[SEND_BUFF], cmd[MAX_BUFF];
    char *host_ip;
    socklen_t len;
    unsigned int port_num;
    
    // less arument error

    if (argc == 1) {
        write(1,"550 less argument\n",strlen("550 less argument\n"));
        exit(1); 
    }

    if(argc >2){
        write(1,"550 too many argument\n",strlen("550 too many argument\n"));
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

    while(1){
        
        pid_t pid;
        len=sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
        if (client_fd < 0) {
            perror("server accept failed");
            exit(EXIT_FAILURE);
        }
            g_client_ip = inet_ntoa(client_addr.sin_addr);
            g_client_port = ntohs(client_addr.sin_port);
            struct passwd *pw = getpwuid(getuid());
                if (pw != NULL) {
                    g_user = pw->pw_name;
                } else {
                    g_user = "unknown"; // don't get user name
                }
            ////////
          time_t current_time = time(NULL); // Get current time
          log_command("Server is started");



        pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }

       if (pid == 0) {  // child process

            while (1) {
                 memset(buff, 0, sizeof(buff));
              //n = read(client_fd, buff, MAX_BUFF);
               if ((n = read(client_fd, buff, MAX_BUFF)) <= 0) {
                    perror("read error or client closed connection");
                    close(client_fd);
                    exit(EXIT_SUCCESS);  // child exits
                }
                buff[n] = '\0';

                if (strncmp(buff, "PORT", 4) == 0) {
                    if (data_sockfd != -1) {
                        close(data_sockfd);
                    }
                    host_ip = convert_str_to_addr(buff, &port_num);

                    // Create data connection socket
                    data_sockfd = socket(AF_INET, SOCK_STREAM, 0);
                    if (data_sockfd == -1) {
                        perror("data socket creation failed");
                        close(client_fd);
                        close(server_fd);
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
                        close(client_fd);
                        close(server_fd);
                        exit(EXIT_FAILURE);
                    }
                    // Acknowledge the PORT command
                    write(1, "200 Port command successful\n", 28);
                    write(client_fd, "200 Port command successful\n", 28);
                    log_command("200 Port command successful");
                    
                    } 
                //BUFF = NLST, RETR ,STOR case
                else if (strncmp(buff, "NLST", 4) == 0 || strncmp(buff,"RETR",4)==0 || strncmp(buff,"STOR",4)==0 || strncmp(buff,"LIST",4)==0) {
                    if (data_sockfd == -1) {
                    write(client_fd, "425 No data connection established\n", 35);
                    log_command("425 No data connection established");
                    continue;
                    }
                    //buff = NLST Case
                    if (strncmp(buff, "NLST", 4) == 0 || strncmp(buff,"LIST",4)==0){
                        //success 150 message
                        write(1, "150 Opening data connection for directory list.\n", 48);
                        write(client_fd, "150 Opening data connection for directory list.\n", 48);
                        log_command("150 Opening data connection for directory list.");
                    }
                     //buff = STOR Case
                    if (strncmp(buff, "STOR", 4) == 0 ){
                        if(TypeChoose[0]=='A'){
                             //success 150 message -> ascii case
                        write(1, "150 Opening ASCII mode data\n", strlen("150 Opening ASCII mode data\n"));
                        write(client_fd, "150 Opening ASCII mode data\n", strlen("150 Opening ASCII mode data\n"));
                        log_command("150 Opening ASCII mode data.");
                        }
                        else{
                              //success 150 message -> binary case
                        write(1, "150 Opening BINARY mode data\n", strlen("150 Opening BINARY mode data\n"));
                        write(client_fd, "150 Opening BINARY mode data\n", strlen("150 Opening BINARY mode data\n"));
                        log_command("150 Opening BINARY mode data.");
                        }
                     }


                    if (strncmp(buff, "STOR", 4) == 0 ) {
                            memset(PutBuff,0,sizeof(PutBuff));
                            //read data -> PutBuff
                            n = read(data_sockfd, PutBuff, MAX_BUFF);
                            if (n <= 0) {
                            fprintf(stderr, "Error reading data from client.\n");
                            } 
                            else {
                            // Transfer data stored in PutBuff to STORcmd function
                            PutBuff[n] = '\0'; 
                            // Extract the filename after "STOR"
                            char *filename = buff + 5; 
                            STORcmd(PutBuff, filename,TypeChoose[0]);
                                //err case
                                if(strncmp(PutBuff,"550",3)==0){
                                    write(1, "550 Err Code Complete transmission.\n", strlen("550 Err Code Complete transmission.\n"));
                                    write(client_fd, "550 Err Code Complete transmission.\n", strlen("550 Err Code Complete transmission.\n"));
                                    log_command("550 Err Code Complete transmission.");
                                }
                                //sucess case
                                else{
                                    write(1, "226 Complete transmission.\n", strlen("226 Complete transmission.\n"));
                                    write(client_fd, "226 Complete transmission.\n", strlen("226 Complete transmission.\n"));
                                    log_command("226 Complete transmission.");
                                }
                            }
                        }
                        
                       
                        if (strncmp(buff, "RETR", 4) == 0) {
                                   // Extract the file name that follows the "RETR" command
                                    char filename[256];
                                    char extra[256];
                                    if (sscanf(buff + 5, "%s %s", filename, extra) != 1 || strlen(filename) == 0) {
                                        // Invalid file name or more than one path
                                        char error_message[] = "550 No filename specified or invalid filename.\n";
                                        log_command("550 No filename specified or invalid filename.");
                                        write(1, error_message, strlen(error_message)); // 서버의 표준 출력 (1)으로 오류 메시지를 출력
                                        write(client_fd, error_message, strlen(error_message)); // 클라이언트 소켓으로 오류 메시지를 전송
                                        err=1;
                                    } else {
                                       // Create a message if it is a valid file name
                                        char message[512];
                                        if (TypeChoose[0] == 'A') {
                                            snprintf(message, sizeof(message), "150 Opening ASCII mode data connection for %s.", filename);
                                            log_command(message);
                                            strcat(message,"\n");
                                        } else {
                                            snprintf(message, sizeof(message), "150 Opening BINARY mode data connection for %s.", filename);
                                            log_command(message);
                                            strcat(message,"\n");
                                        }

                                       // Output and send messages
                                        write(1, message, strlen(message));
                                        write(client_fd, message, strlen(message));
                                    }
                }
                 
                


                // Send directory listing to the client
                cmd_process(buff, result_buff); 
                write(data_sockfd, result_buff, MAX_BUFF);
                read(data_sockfd,"",1);
                if(strncmp(buff,"STOR",4)!=0){
                    //sucess message pritf
                write(1, "226 Complete transmission.\n", strlen("226 Complete transmission.\n"));
                write(client_fd, "226 Complete transmission.\n", strlen("226 Complete transmission.\n"));
                log_command("226 Complete transmission.");
                }
                // NLST recive byte
                if (strncmp(buff, "NLST", 4) == 0 || strncmp(buff,"LIST",4)==0){
                    char byte_msg[50];
                    snprintf(byte_msg, sizeof(byte_msg), "OK. %zu bytes is received",bytes_read);
                    log_command(byte_msg);
                    bytes_read=0;
                } 
                //RETR recive byte
                if (strncmp(buff, "RETR", 4) == 0 ){
                    char final_msg[MAX_BUFF];
                    if(err==1){
                        char error_msg[] = "550 Invalid number of arguments for RETR command\n";
                        write(client_fd, error_msg, strlen(error_msg));
                        write(1, error_msg, strlen(error_msg));
                        //log_command(error_msg);
                    }

                    else{
                        //send byte for client
                        snprintf(final_msg, sizeof(final_msg), "OK. %zu bytes is received", bytes_read);
                        log_command(final_msg);
                        strcat(final_msg,"\n");
                        write(client_fd, final_msg, strlen(final_msg));
                        write(1, final_msg, strlen(final_msg));
                        }
                        bytes_read=0;
                         err=0;
                 } 
        
                // Close the data connection
               close(data_sockfd);
                data_sockfd = -1;
                memset(buff, 0, sizeof(buff));
            }
                        

                // Handle QUIT Command
                // If QUIT command is received, close the connection
                else if (strcmp(buff, "QUIT") == 0) {
                    memset(result_buff, 0, SEND_BUFF);
                    write(1,"QUIT\n",strlen("QUIT\n"));
                    strcat(result_buff,"221 Goodbye");
                    log_command("221 Goodbye");
                    n = write(client_fd, result_buff, strlen(result_buff) + 1);
                    if (n < 0) {
                    perror("write error");
                    close(client_fd);
                    exit(EXIT_FAILURE);  // child exits
                }
                   exit(0);
                   
                }
                
                
                
                else{
                memset(result_buff,0,sizeof(result_buff));
                cmd_process(buff, result_buff); 

                //Send the results to the client
                n = write(client_fd, result_buff, strlen(result_buff) + 1);
                if (n < 0) {
                    perror("write error");
                    close(client_fd);
                    exit(EXIT_FAILURE);  // child exits
                }
                }
              

            }
        } 
        else { 
             //parent process
        }
        //close cli socket
         close(client_fd);
    
    }
    //close server socket
    close(server_fd);
    return 0;
}
//////////////////////main fun END/////////////////////////////



