/////////////////////////////////////////////////////////////////////
// File Name : cli.c                                               //
// Date : 2024/04/16                                                //
// OS : Ubuntu 20.04.6 LTS 64bits                                   //
//                                                                  //
// Author : Hong Wang ki                                            //
// Student ID : 2020202060                                          //
// ----------------------------------------------------------------- //
// Title : System Programming Assignment #1-3 ( ftp server )        //
//Description: This program switches Linux commands to ftp           //
//             commands and sends them to srv.                       //            
///////////////////////////////////////////////////////////////////////
// Function: NLSTcmd, LISTcmd, DELEcmd, MKDcmd, RMDcmd, Renamecmd,  //
//   QUITcmd compare_file,main,perform_ls, perform_ls-l, CWDcmd CDUPcmd//
// ================================================================= //
// Input:                                                             //
//     argc: number of command-line arguments                         //
//     argv: array of command-line arguments                          //
//     buf: Array containing ftp commands                             //
//      path The path to which you want to perform the task            //
//                                                                    //
// (Input parameter Description)                                      //
//     - argc: number of command-line arguments                         ////////////////////
//     - argv: array of command-line arguments, where argv[0] is the program name       //
//             and argv[1] (if provided) is the directory path                           //
//      - buf:Array containing ftp commands    
//     -path The path to which you want to perform the task                             //
//                                                                                       //  
// Output:                                                                               //
//    Perform FTP commands directly by command                                          //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////  
// Purpose:                                                                                             //
//     It reads the buff received from the cli and directly implements 10 FTP commands.                 //                                                    //
// =====================================================================================================//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h> 
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <dirent.h>

#define BUFFER_SIZE 512
#define MAX_FILES 1024
//flag
int aflag = 0; 
int lflag = 0; 

// file struct
typedef struct {
    char name[256];
    struct stat st;
} FileInfo;

// file name compare fun
int compare_files(const void *a, const void *b) {
     // Casting the void pointers to FileInfo pointers
    const FileInfo *file_a = (const FileInfo *)a;
    const FileInfo *file_b = (const FileInfo *)b;
    // Comparing the names of the files using strcmp function
    return strcmp(file_a->name, file_b->name);
}

void perform_ls(const char *path) {
    DIR *dir; // Pointer to directory stream
    struct dirent *ent; // Pointer to directory entry
    struct stat st; // File information structure
    char full_path[PATH_MAX]; // Array to store full file path
    FileInfo files[MAX_FILES];  // Array to store file information
    int file_count = 0; // Counter for the number of files

    // Open directory
    if ((dir = opendir(path)) != NULL) {
         // Read file information from the directory
        while ((ent = readdir(dir)) != NULL && file_count < MAX_FILES) {
             // Skip hidden files unless -a option is provided
            if (ent->d_name[0] == '.' && !aflag) {
                continue;
            }
            // Construct full file path
            snprintf(full_path, sizeof(full_path), "%s/%s", path, ent->d_name);
             // Get file information
            if (stat(full_path, &st) != -1) {
                 // Store file name and information in the files array
                snprintf(files[file_count].name, sizeof(files[file_count].name), "%s", ent->d_name);
                files[file_count].st = st;
                file_count++;
            }
        }
        closedir(dir);
    } else {
            // If failed to open directory, print access restriction error
            char error_msg[BUFFER_SIZE];
            int error_msg_len = snprintf(error_msg, sizeof(error_msg), "Error: Cannot access \n");
            write(2, error_msg, error_msg_len);
            exit(EXIT_FAILURE);
        }

    // Sort file names
    qsort(files, file_count, sizeof(FileInfo), compare_files);

    // Print sorted file information
    int fd = open("/dev/stdout", O_WRONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < file_count; i++) {
        char file_msg[BUFFER_SIZE];
          // Prepare message to print file name with appropriate formatting
        int file_msg_len = snprintf(file_msg, sizeof(file_msg), "%s%s%s", files[i].name, S_ISDIR(files[i].st.st_mode) ? "/" : "", (i + 1) % 5 == 0 || i == file_count - 1 ? "\n" : "\t");
          // Write message to file descriptor
        if (write(fd, file_msg, file_msg_len) == -1) {
            perror("write");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    close(fd);
}

void perform_ls_l(const char *path) {
    DIR *dir; // Directory stream pointer
    struct dirent *ent; // Directory entry pointer
    struct stat st; // File information structure
    struct passwd *pw; // Password structure pointer
    struct group *gr;  // Group structure pointer
    char full_path[PATH_MAX]; // Array to store full file path
    char date[20]; // Array to store date in string format
    FileInfo files[MAX_FILES]; // Array to store file information
    int file_count = 0; // Counter for the number of files

    // Open directory
    if ((dir = opendir(path)) != NULL) {
        // Read file information from the directory
        while ((ent = readdir(dir)) != NULL && file_count < MAX_FILES) {
            snprintf(full_path, sizeof(full_path), "%s/%s", path, ent->d_name);
             // Get file information
            if (stat(full_path, &st) != -1) {
               // Store file name and information in the files array
               snprintf(files[file_count].name, sizeof(files[file_count].name), "%s", ent->d_name);
                files[file_count].st = st;
                file_count++;
            }
        }
        closedir(dir);
    } else {
     // If it's not a directory, print file information
    if ((stat(path, &st) != -1) && S_ISREG(st.st_mode) && (access(path, R_OK) == 0)) {
        // If the read file is a regular file
        write(1, (S_ISDIR(st.st_mode)) ? "d" : "-", 1);     // Print file type
        write(1, (st.st_mode & S_IRUSR) ? "r" : "-", 1);   // Print owner's read permission
        write(1, (st.st_mode & S_IWUSR) ? "w" : "-", 1); // Print owner's write permission
        write(1, (st.st_mode & S_IXUSR) ? "x" : "-", 1); // Print owner's execute permission
        write(1, (st.st_mode & S_IRGRP) ? "r" : "-", 1);  // Print group's read permission
        write(1, (st.st_mode & S_IWGRP) ? "w" : "-", 1);  // Print group's write permission
        write(1, (st.st_mode & S_IXGRP) ? "x" : "-", 1); // Print group's execute permission
        write(1, (st.st_mode & S_IROTH) ? "r" : "-", 1); // Print others' read permission
        write(1, (st.st_mode & S_IWOTH) ? "w" : "-", 1);   // Print others' write permission
        write(1, (st.st_mode & S_IXOTH) ? "x" : "-", 1); // Print others' execute permission

        write(1, " ", 1); // Space

        char nlink_str[4];  // Buffer for link count string
        int nlink_len = snprintf(nlink_str, sizeof(nlink_str), "%ld", st.st_nlink); 
        write(1, nlink_str, nlink_len); // Print link count

        write(1, " ", 1); 

        write(1, getpwuid(st.st_uid)->pw_name, strlen(getpwuid(st.st_uid)->pw_name));// Print owner's name


        write(1, " ", 1); 

        write(1, getgrgid(st.st_gid)->gr_name, strlen(getgrgid(st.st_gid)->gr_name)); // Print group's name

        write(1, " ", 1); 

        char size_str[9]; // Buffer for file size string
        int size_len = snprintf(size_str, sizeof(size_str), "%8ld", st.st_size); // Convert file size to string
        write(1, size_str, size_len); // Print file size

        write(1, " ", 1); 

        char date[20];// Buffer for modified date string
        strftime(date, sizeof(date), "%b %d %H:%M", localtime(&st.st_mtime)); // Convert modified date to formatted string
        write(1, date, strlen(date));// Print modified date

        write(1, " ", 1); 

        write(1, path, strlen(path)); // Print file path

        write(1, "\n", 1); 
    }
    
    else {
        // If access permission denied or not a directory
        // Print error message
        write(1, "Error: cannot access\n", strlen("Error: cannot access\n"));
        exit(EXIT_FAILURE);
    }
}
    

    // Sort file names
    qsort(files, file_count, sizeof(FileInfo), compare_files);

    // Print sorted file information
    for (int i = 0; i < file_count; i++) {
        if (!aflag && files[i].name[0] == '.') {
            continue; 
        }
        pw = getpwuid(files[i].st.st_uid);// Get owner's information
        gr = getgrgid(files[i].st.st_gid);// Get group's information
        strftime(date, sizeof(date), "%b %d %H:%M", localtime(&files[i].st.st_mtime));
            // Print file type and permissions
        write(1, (S_ISDIR(files[i].st.st_mode)) ? "d" : "-", 1);
        write(1, (files[i].st.st_mode & S_IRUSR) ? "r" : "-", 1);
        write(1, (files[i].st.st_mode & S_IWUSR) ? "w" : "-", 1);
        write(1, (files[i].st.st_mode & S_IXUSR) ? "x" : "-", 1);
        write(1, (files[i].st.st_mode & S_IRGRP) ? "r" : "-", 1);
        write(1, (files[i].st.st_mode & S_IWGRP) ? "w" : "-", 1);
        write(1, (files[i].st.st_mode & S_IXGRP) ? "x" : "-", 1);
        write(1, (files[i].st.st_mode & S_IROTH) ? "r" : "-", 1);
        write(1, (files[i].st.st_mode & S_IWOTH) ? "w" : "-", 1);
        write(1, (files[i].st.st_mode & S_IXOTH) ? "x" : "-", 1);
        
        write(1, " ", 1);
        write(1, " ", 1);
        
        char nlink_str[4]; // Buffer for link count string
        int nlink_len = snprintf(nlink_str, sizeof(nlink_str), "%ld", files[i].st.st_nlink);// Convert link count to string
        write(1, nlink_str, nlink_len);
        
        write(1, " ", 1);
        
        write(1, pw->pw_name, strlen(pw->pw_name));
        
        write(1, " ", 1);
        
        write(1, gr->gr_name, strlen(gr->gr_name));
        
        write(1, " ", 1);
        
        char size_str[9];
        int size_len = snprintf(size_str, sizeof(size_str), "%8ld", files[i].st.st_size);
        write(1, size_str, size_len);
        
        write(1, " ", 1);
        
        write(1, date, strlen(date)); // Print modified date
        
        write(1, " ", 1);
        
        write(1, files[i].name, strlen(files[i].name));
        
        write(1, (S_ISDIR(files[i].st.st_mode)) ? "/" : "", S_ISDIR(files[i].st.st_mode) ? 1 : 0);  // Append '/' for directories
        
        write(1, "\n", 1);
    }
}

void NLSTcmd(int argc, char **argv) {
    int opt; // Variable to store option returned by getopt function
    char *path = "."; // By default, use the current director

   // Analyze options using getopt function
    while ((opt = getopt(argc, argv, "al")) != -1) {
        switch(opt) {   
            case 'a':
                aflag = 1;
                break;
            case 'l':
                lflag = 1;
                break;
            default:
            // Print error message for invalid option and exit
               write(1,"Error: invalid option\n",strlen("Error: invalid option\n"));
               exit(EXIT_FAILURE);

        }
    }

// If a path is provided when calling NLSTcmd function, use that path
if (optind < argc) {
    // If there are more than one path arguments, print error message and exit
    if (optind < argc - 1) {
        char error_msg[BUFFER_SIZE];
        int error_msg_len = snprintf(error_msg, sizeof(error_msg), "Too many arguments\n");
        write(2, error_msg, error_msg_len);
        exit(EXIT_FAILURE);
    }
    path = argv[optind];
     // Check if the provided path is valid
    struct stat st;
    if (stat(path, &st) == -1) {
          // Print error message and exit if the path is invalid
        write(1,argv[0],strlen(argv[0]));
        write(1," ",1);
        if(argv[1][0]=='-'){
        write(1,argv[1],strlen(argv[1]));
        }
        write(1,"\n",1);
        perror("Error: ");
        exit(EXIT_FAILURE);
    }
     // Check if the provided path is a directory
    if (!S_ISDIR(st.st_mode)&& !lflag) {
        // Print error message and exit if the path is not a directory
        char error_msg[BUFFER_SIZE];
        int error_msg_len = snprintf(error_msg, sizeof(error_msg), "No such file or directory\n");
        write(2, error_msg, error_msg_len);
        exit(EXIT_FAILURE);
    }
}

     // Perform appropriate actions based on each option
    if (aflag && lflag) {
          // If both '-a' and '-l' options are provided
        write(1, "NLST -al\n", strlen("NLST -al\n")); 
        perform_ls_l(path);
    } else if (aflag) {
        // If only the '-a' option is provided
        write(1, "NLST -a\n", strlen("NLST -a\n")); 
        perform_ls(path);
    } else if (lflag) {
         // If only the '-l' option is provided
        write(1, "NLST -l\n", strlen("NLST -l\n")); 
        perform_ls_l(path);
    } else {
          // If no option is provided
        write(1, "NLST\n", strlen("NLST\n")); 
        perform_ls(path);
    }
    }

void PWDcmd(char **argv){
    // ./cli pwd | tee cli.out | ./srv -> Print the current directory
    // ./cli pwd -e | tee cli.out | ./srv -> Print the current directory (ignore options)
    // ./cli pwd arg | tee cli.out | ./srv -> Print the current directory (ignore arguments)
    
    if (argv[1] != NULL && argv[1][0] == '-') {
       // If there's an option, handle error
        write(2, "Error: invalid option\n", strlen("Error: invalid option\n"));
        
    } else if (argv[1] != NULL) {
       // If there's an argument, handle error
        write(2, "Error: argument is not required\n", strlen("Error: argument is not required\n"));
    } else {
         // If neither option nor argument is provided, print the current directory
        char cwd[256];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            write(1, cwd, strlen(cwd));
            write(1," is current directory",strlen(" is current directory"));
            write(1, "\n", 1);
        } else {
            perror("getcwd");
            exit(EXIT_FAILURE);
        }
    }
}

void MKDcmd(char **argv){
    if (argv[1] == NULL) {
         // If there's no argument, handle error
        write(2, "Error: argument is required\n", strlen("Error: argument is required\n"));
        write(2,"\n",1);       
        return;
    }
    if(argv[1][0]=='-'){
         // If the first argument starts with '-', handle error
        write(2,"Error: invalid option",strlen("Error: invalid option"));
        write(2,"\n",1);   
        return ;
    }

     // Create each directory provided as an argument
    for (int i = 1; argv[i] != NULL; i++) {
        if (mkdir(argv[i], 0755) == 0) {
            write(1, argv[0],strlen (argv[0]));
            write(1, " ",strlen (" "));
            write(1, argv[i], strlen(argv[i]));
            write(1, "\n", 1);
        } else {
              // If the directory already exists, handle error
            write(2, "Error: cannot create directory '", strlen("Error: cannot create directory '"));
            write(2, argv[i], strlen(argv[i]));
            write(2, "' : File exists", strlen("': File exists"));
            write(2,"\n",1);
        }
    }
}

void RMDcmd(char **argv){
    if (argv[1] == NULL) {
            // Handle error if there's no argument
        write(2, "Error: argument is required\n", strlen("Error: argument is required\n"));
        return;
    }

    if(argv[1][0]=='-'){
        // Handle error if the first argument starts with '-'
        write(2,"Error: invalid option",strlen("Error: invalid option"));
        write(2,"\n",1);   
        return ;
    }
    for (int i = 1; argv[i] != NULL; i++) {
        if(rmdir(argv[i])==0){
              // If directory removal is successful, print a message
            write(1, argv[0],strlen (argv[0]));
            write(1, " ",strlen (" "));
            write(1, argv[i], strlen(argv[i]));
            write(1, "\n", 1);
        }
        else{
             // If directory removal fails, handle error
            write(2, "Error: failed to remove ", strlen("Error: failed to remove "));
            write(2, argv[i], strlen(argv[i]));
            write(2, "\n", 1);
        }
    }
}

void QUITcmd(char **argv){
    if (argv[1] != NULL && argv[1][0] == '-') {
         // Handle error if there's an option
        write(2, "Error: invalid option\n", strlen("Error: invalid option\n"));
        
    } else if (argv[1] != NULL) {
        // Handle error if there's an argument
        write(2, "Error: argument is not required\n", strlen("Error: argument is not required\n"));
    } else {
         // When QUIT is received exactly
        write(1, "QUIT success\n", strlen("QUIT success\n"));
        exit(0); // Exit the program
    }
    
}
void CWDcmd(char **argv) {

     // Check if an argument including an option is provided
    if (argv[1] != NULL && argv[1][0] == '-') {
         // Handle error if an option is provided
        write(2, "Error: invalid option\n", strlen("Error: invalid option\n"));
        return;
    }

     // Check if the number of arguments is more than one
    if (argv[2] != NULL) {
           // Handle error if more than one argument is provided
        write(2, "Error: too many arguments\n", strlen("Error: too many arguments\n"));
        return;
    }

    // Change to the directory provided as an argument
    if (chdir(argv[1]) == 0) {
         // If the change is successful
        char cwd[256];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
         // Print success message and current directory
            write(1,"CWD ",strlen("CWD "));
            write(1,argv[1],strlen(argv[1]));
            write(1,"\n",1);
            write(1, cwd, strlen(cwd));
            write(1, " is current direcotry\n", strlen(" is current direcotry\n"));
        } else {
               // Handle error if getcwd function fails
            perror("getcwd");
            exit(EXIT_FAILURE);
        }
    } else {
         // If the change fails, handle error
        if (errno == EACCES) {
             // Permission denied error
            write(2, "Error: permission denied\n", strlen("Error: permission denied\n"));
        } else if (errno == ENOENT) {
             // Directory not found error
            write(2, "Error: directory not found\n", strlen("Error: directory not found\n"));
        } else {
            // Other errors
            perror("chdir");
        }
    }
}

void CDUPcmd(char** argv) {

     // Check if an argument including an option is provided
    if (argv[1] != NULL && argv[1][0] == '-') {
        // Handle error if an option is provided
        write(2, "Error: invalid option\n", strlen("Error: invalid option\n"));
        return;
    }

      // Check if the number of arguments is more than one
    if (argv[1] != NULL) {
        // Handle error if more than one argument is provided
        write(2, "Error: too many arguments\n", strlen("Error: too many arguments\n"));
        return;
    }
    
     // Change to the parent directory
    if (chdir("..") == 0) {
          // If the change is successful
        char cwd[256];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
             // Print success message and current directory
            write(1,"CDUP",strlen("CDUP"));
            write(1,"\n",1);
            write(1, cwd, strlen(cwd));
            write(1, " is current direcotry\n", strlen(" is current direcotry\n"));
        } else {
            // Handle error if getcwd function fails
            perror("getcwd");
            exit(EXIT_FAILURE);
        }
    } else {
        // If the change fails, handle error
        if (errno == EACCES) {
          // Permission denied error
            write(2, "Error: permission denied\n", strlen("Error: permission denied\n"));
        } else if (errno == ENOENT) {
             // Directory not found error
            write(2, "Error: directory not found\n", strlen("Error: directory not found\n"));
        } else {
              // Other errors
            perror("chdir");
        }
    }
}

void DELEcmd(char** argv) {
    // Check if the file to delete is specified
    if (argv[1] == NULL) {
        write(2, "Error: missing operand\n", strlen("Error: missing operand\n"));
        return;
    }
    // Check if an option is provided as the first argument
    if(argv[1][0]=='-'){
        write(2, "Error: invalid option\n", strlen("Error: invalid option\n"));
         return;
    }

    int i = 1;
    while (argv[i] != NULL) {
        struct stat statbuf;
         // Get the status of the file or directory
        if (stat(argv[i], &statbuf) != 0) {
        // If failed to get the status
            perror("Error: ");
            i++;
            continue;
        }

       // Attempt to delete only if it's a regular file
        if (S_ISREG(statbuf.st_mode)) {
           // Attempt to delete the file
            if (unlink(argv[i]) == 0) {
                // If file deletion is successful
                write(1, argv[0], strlen(argv[0]));
                write(1, " ", 1);
                write(1, argv[i], strlen(argv[i]));
                write(1, "\n", 1);
            } else {
                // If file deletion fails
                if (errno == ENOENT) {
                 // If the file doesn't exist
                    write(2, "Error: file '", strlen("Error: file '"));
                    write(2, argv[i], strlen(argv[i]));
                    write(2, "' does not exist\n", strlen("' does not exist\n"));
                } else if (errno == EACCES) {
                      // If permission is denied for the file
                    write(2, "Error: permission denied for file '", strlen("Error: permission denied for file '"));
                    write(2, argv[i], strlen(argv[i]));
                    write(2, "'\n", strlen("'\n"));
                } else {
                    // Other errors
                    perror("unlink");

                }
            }
        } else {
            // If it's a directory, output error
            write(2, "Error: '", strlen("Error: '"));
            write(2, argv[i], strlen(argv[i]));
            write(2, "' is a directory, cannot delete\n", strlen("' is a directory, cannot delete\n"));
        }
        i++;
    }
}

// Comparison function
int compare(const struct dirent **a, const struct dirent **b) {
    return strcmp((*a)->d_name, (*b)->d_name);
}

void LISTcmd(char **argv) {
    // Check if the number of arguments exceeds one
    if (argv[2] != NULL) {
        write(2, "Error: too many arguments\n", strlen("Error: too many arguments\n"));
        return;
    }

    // Check if an option is provided as the first argument
    if (argv[1] != NULL && argv[1][0] == '-') {
         // If an option is provided, output an error
        write(2, "Error: invalid option\n", strlen("Error: invalid option\n"));
        return;
    }

   // If no directory is specified, work in the current directory
    char *dirname = (argv[1] == NULL) ? "." : argv[1];

  // Open the directory
    struct dirent **namelist;
    int n;

    n = scandir(dirname, &namelist, NULL, compare);
    if (n < 0) {
        write(1,"LIST",strlen("LIST"));
        write(1,"\n",1);
        perror("Error: ");
        return;
    }
    write(1,"LIST",strlen("LIST"));
    write(1,"\n",1);

    // Output the sorted list
    for (int i = 0; i < n; i++) {
         // Get file details
        struct stat fileStat;
        char path[1024];
        sprintf(path, "%s/%s", dirname, namelist[i]->d_name);
        if (lstat(path, &fileStat) < 0) {
            perror("Error: ");
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
        write(1, permissions, strlen(permissions));
        write(1, " ", 1);

       // Output link count
        char linkCount[32];
        int linkLength = sprintf(linkCount, "%lu", fileStat.st_nlink);
        write(1, linkCount, linkLength);
        write(1, " ", 1);

         // Output owner and group
        struct passwd *pwd = getpwuid(fileStat.st_uid);
        struct group *grp = getgrgid(fileStat.st_gid);
        write(1, pwd->pw_name, strlen(pwd->pw_name));
        write(1, " ", 1);
        write(1, grp->gr_name, strlen(grp->gr_name));
        write(1, " ", 1);

        // Output file size
        char fileSize[32];
        int sizeLength = sprintf(fileSize, "%lu", fileStat.st_size);
        write(1, fileSize, sizeLength);
        write(1, " ", 1);

        // Output modification time
        char date[100];
        strftime(date, sizeof(date), "%b %e %H:%M", localtime(&fileStat.st_mtime));
        write(1, date, strlen(date));
        write(1, " ", 1);

         // Output file name
        write(1, namelist[i]->d_name, strlen(namelist[i]->d_name));

         // Add '/' if it's a directory
        if (S_ISDIR(fileStat.st_mode)) {
            write(1, "/", 1);
        }

        write(1, "\n", 1);

        free(namelist[i]);
    }

    // Free memory and close directory
    free(namelist);
}

void RenameCmd(char **argv) {
     // Check the number of parameters
    if (argv[0] == NULL || argv[1] == NULL || argv[2] == NULL || argv[3] == NULL || argv[4] != NULL) {
        write(2, "Error: invalid number of arguments\n", strlen("Error: invalid number of arguments\n"));
        return;
    }

   // Check if the old name exists
    if (access(argv[1], F_OK) != 0) {
        write(2, "Error: file name does not exist\n", strlen("Error: file name does not exist\n"));
        return;
    }

    // Check if both RNFR and RNTO are given
    if (strcmp(argv[0], "RNFR") != 0 || strcmp(argv[2], "RNTO") != 0) {
        write(2, "Error: invalid command format\n", strlen("Error: invalid command format\n"));
        return;
    }

    // Check if both old name and new name are given
    if (argv[1][0] == '\0' || argv[3][0] == '\0') {
        write(2, "Error: invalid file names\n", strlen("Error: invalid file names\n"));
        return;
    }

    // Check if the new file or directory already exists
    if (access(argv[3], F_OK) == 0) {
        write(2, "Error: name to change already exists\n", strlen("Error: name to change already exists\n"));
        return;
    }

   // Perform file name change
    if (rename(argv[1], argv[3]) != 0) {
        perror("Error renaming file");
        return;
    }

   // Output success message
    write(1, argv[0], strlen(argv[0]));
    write(1, " ", 1);
    write(1, argv[1], strlen(argv[1]));
    write(1, "\n", 1);
    write(1, argv[2], strlen(argv[2]));
    write(1, " ", 1);
    write(1, argv[3], strlen(argv[3]));
    write(1, "\n", 1);
}

int main() {
    char buf[BUFFER_SIZE]={0}; 
    ssize_t bytes_read;
    char *command;
    char **args;// Pointer to dynamically allocated argument array
    int i = 0;

 // Read data from the buffer
    bytes_read = read(0, buf, sizeof(buf));

    if (bytes_read > 0) {
        if (buf[bytes_read - 1] == '\n')
        buf[bytes_read - 1] = '\0'; // Remove the newline character

      // Parse input into command and arguments
        command = strtok(buf, " ");

        // Dynamically allocate memory for the argument array
        args = (char**)malloc(sizeof(char*) * 20);
        if (args == NULL) {
             perror("malloc");
            exit(EXIT_FAILURE);
        }
        //args = command 
        while (command != NULL) {
            args[i++] = command;
            command = strtok(NULL, " ");
        }

        
        // Execute the appropriate command based on the input
        if(strcmp(args[0], "NLST") == 0){
            NLSTcmd(i, args);
        }
        else if(strcmp(args[0], "PWD") == 0){
            PWDcmd(args);
        }
        else if(strcmp(args[0],"MKD")==0){
            MKDcmd(args);
        }
        else if(strcmp(args[0],"RMD")==0){
            RMDcmd(args);
        }
        else if(strcmp(args[0],"QUIT")==0){
            QUITcmd(args);
        }
        else if(strcmp(args[0],"CWD")==0){
            CWDcmd(args);
        }
        else if(strcmp(args[0],"CDUP")==0){
            CDUPcmd(args);
        }
         else if(strcmp(args[0],"DELE")==0){
            DELEcmd(args);
        }
         else if(strcmp(args[0],"LIST")==0){
            LISTcmd(args);
        }
        else if(strcmp(args[0],"RNFR")==0 && (strcmp(args[2],"RNTO"))==0){
            RenameCmd(args);
        }
         else {
            // Unknown command
            write(2, "Error: Unknown command\n", strlen("Error: Unknown command\n"));
        }
        
       // Free dynamically allocated memory
            free(args);

    } else if (bytes_read == -1) { // Error handling for read operation
        perror("read");
        return 1;
    } else { // EOF (End of File)
        return 1;
    }

    return 0;
}