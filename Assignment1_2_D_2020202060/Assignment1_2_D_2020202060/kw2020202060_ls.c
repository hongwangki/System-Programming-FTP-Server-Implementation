#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/////////////////////////////////////////////////////////////////////
// File Name : kw2020202060.opt.c.c                                 //
// Date : 2024/03/30                                                //
// OS : Ubuntu 20.04.6 LTS 64bits                                   //
//                                                                  //
// Author : Hong Wang ki                                            //
// Student ID : 2020202060                                          //
// ----------------------------------------------------------------- //
// Title : System Programming Assignment #1-2 ( ftp server )        //
// Description: This program lists the contents of a specified directory// 
///////////////////////////////////////////////////////////////////////
// Function: main
// ================================================================= //
// Input: 
//     argc: number of command-line arguments
//     argv: array of command-line arguments
// 
// (Input parameter Description) //
//     - argc: number of command-line arguments
//     - argv: array of command-line arguments, where argv[0] is the program name
//             and argv[1] (if provided) is the directory path
// 
// Output: 
//     Returns 0 if successful, otherwise returns 1
//
// 
// Purpose: 
//     This function serves as the entry point for the program. It opens and reads
//     the content of a directory specified by the user. It then prints out the names
//     of all the files and directories found within that directory. Additionally,
//     it performs error checking to ensure that the directory can be accessed and
//     that the necessary permissions are granted.
// ================================================================= //
int main(int argc, char const *argv[])
{
    // Define a pointer to store the directory path
    const char *directory = "./";

   // Check if the user provided a directory path as a command-line argument
    if (argc == 2) {
    // Provided directory path as command-line argument, set directory to the provided path
        directory = argv[1];
    }
    else if(argc > 2) {
        printf("only one directory path can be processed\n");
        //Output an error when more than one argument is passed and exit the program
        return 1;    
    }

    // Declare a struct to hold information about the directory
    struct stat dir_stat;


    // Open the directory for reading
    DIR *dp = opendir(directory);
    if (dp == NULL) {
        // Check if the specified directory exists
        if (stat(directory, &dir_stat) == -1) {
            // If the directory does not exist, print an error message and return 1
            fprintf(stderr, "kw2020202060_ls: cannot access '%s': No such directory\n", directory);
            return 1;
        }
        //Check if not directory
        if(!S_ISDIR(dir_stat.st_mode)){
            fprintf(stderr, "kw2020202060_ls: cannot access '%s': No such directory\n", directory);
            return 1;
        }

    // Check if the program has read and execute permissions on the directory
        if (access(directory, R_OK) == -1) {
            // If the program does not have the necessary permissions, print an error message and return 1
            fprintf(stderr, "kw2020202060_ls: cannot access '%s': Access denied\n", directory);
            return 1;
        }
        return 1;
    }
    // Declare a pointer to a directory entry
    struct dirent *entry;

    // Iterate over each directory entry and print its name
    while ((entry = readdir(dp)) != NULL) {
        printf("%s\n", entry->d_name);
    }

    // Close the directory
    if (closedir(dp)) {
        // If there's an error closing the directory, print an error message and return 1
        perror("closedir");
        return 1;
    }

    // Return 0 to indicate successful execution of the program
    return 0;
}
