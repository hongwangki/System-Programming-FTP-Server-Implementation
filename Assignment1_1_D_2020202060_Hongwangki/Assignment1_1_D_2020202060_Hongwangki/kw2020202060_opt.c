#include <stdio.h>
#include <unistd.h>

//////////////////////////////////////////////////////////////////////
// File Name : kw2020202060.opt.c.c                                 //
// Date : 2024/03/30                                                //
// OS : Ubuntu 20.04.6 LTS 64bits                                   //
//                                                                  //
// Author : Hong Wang ki                                            //
// Student ID : 2020202060                                          //
// ----------------------------------------------------------------- //
// Title : System Programming Assignment #1-1 ( ftp server )        //
// Description : ...                                                //
///////////////////////////////////////////////////////////////////////
// main function
// ================================================================= //
// Input: argc - Number of command-line arguments //
//        argv - Array of command-line arguments //
// (Description of input parameters) //
//      argc: Total number of arguments passed in the command line.
//      argv: Array containing pointers to strings, each representing an argument passed in the command line.
// Output: int - 0 for success, 1 for failure //
// (Description of output parameter) //
//      Return value:
//      - An integer indicating the successful completion of the program, returning 0 upon successful completion.
//      - Returns 1 on failure.
//Purpose: This function serves to parse the command line based on the presence of options. //
///////////////////////////////////////////////////////////////////////


/*
   This program demonstrates the usage of command-line arguments parsing in C using getopt().
   It defines several flags (-a, -b) and an option (-c) that can be passed through the command line.
   Additionally, it handles the case where an unknown option is provided and prints non-option arguments.
*/

#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
    int aflag = 0, bflag = 0; // Flags for options -a and -b
    char *cvalue = NULL; // Value for option -c
    int index, c; // Index for iteration and variable to store parsed option

    opterr = 0; // Disable default error messages by getopt()

    // Parse command-line options
    while ((c = getopt(argc, argv, "abdc:")) != -1) {
        switch (c) {
            case 'a':
                aflag++; // Increment aflag if option -a is present
                break;
            case 'b':
                bflag++; // Increment bflag if option -b is present
                break;
            case 'c':
                cvalue = optarg; // Store the value of option -c
                break;
            case 'd':
                opterr = 0; // Handle option -d, setting opterr to 0
                break;
            case '?':
                printf("Unknown option character\n"); // Handle unknown option
                break;    
        }
    }

    // Print the values of flags and options after parsing
    printf("aflag = %d, bflag = %d, cvalue = %s\n", aflag, bflag, cvalue);

    // Print any non-option arguments
    for (index = optind; index < argc; index++) {
        printf("Non-option argument %s\n", argv[index]);
    }

    return 0;
}

