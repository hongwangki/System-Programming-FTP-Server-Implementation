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
// Function: main
// ================================================================= //
// Input:                                                             //
//     argc: number of command-line arguments                         //
//     argv: array of command-line arguments                          //
//                                                                    //
// (Input parameter Description)                                      //
//     - argc: number of command-line arguments                         ////////////////////
//     - argv: array of command-line arguments, where argv[0] is the program name       //
//             and argv[1] (if provided) is the directory path                           //
//                                                                                       //  
// Output:                                                                               //
//     Wipe the buffer and forward it to the server                                     //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////  
// Purpose:                                                                                             //
//     This program translates command line input into FTP commands for specific operations.            //
//     It takes user-provided commands and arguments and converts them into corresponding FTP commands. //
//     The program handles various FTP commands such as listing files/directories, changing directories,//
//     creating/deleting directories, renaming files, and quitting the FTP session.                     //
//     It ensures proper formatting of FTP commands and provides error messages for invalid commands    //
//     or insufficient arguments.                                                                       //
// =====================================================================================================//



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

//buf max size
#define BUFFER_SIZE 512

int main(int argc, char const **argv){
    //buffer declaration
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [arguments]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char buf[BUFFER_SIZE]={0};
////////////////////Converting Linux commands to ftp commands////////////////////////////
    //change ls to NLST
    if (strcmp(argv[1], "ls") == 0) {
        strcpy(buf, "NLST"); // Copy "NLST" string to buff

        // Outputs from index 2 to argc-1 of the argv array and adds to the buff
        for (int i = 2; i < argc; i++) {
            strcat(buf, " ");
            strcat(buf, argv[i]);// Add string from current index to buff
        }
    }

    //change dir to LIST
    else if (strcmp(argv[1], "dir") == 0){
         strcpy(buf, "LIST"); //Copy "LIST" string to buff
           // Outputs from index 2 to argc-1 of the argv array and adds to the buff
        for (int i = 2; i < argc; i++) {
            strcat(buf, " ");
            strcat(buf, argv[i]); // Add string from current index to buff
        }
    }

    //change pwd to PWD
    else if (strcmp(argv[1], "pwd") == 0){
        strcpy(buf, "PWD"); //Copy "PWD" string to buff
          // Outputs from index 2 to argc-1 of the argv array and adds to the buff
        for (int i = 2; i < argc; i++) {
            strcat(buf, " ");
            strcat(buf, argv[i]); // Add string from current index to buff
        }
    }

    //change cd to CWD
    else if (strcmp(argv[1], "cd") == 0 && strcmp(argv[2], "..") != 0) {
        strcpy(buf, "CWD"); //Copy "Cwd" string to buff
         // Outputs from index 2 to argc-1 of the argv array and adds to the buff
        for (int i = 2; i < argc; i++) {
            strcat(buf, " ");
            strcat(buf, argv[i]); // Add string from current index to buff
        }
    }

    //change cd .. to CDUP
    else if (strcmp(argv[1], "cd") == 0 && strcmp(argv[2], "..") == 0) {
        strcpy(buf, "CDUP"); //Copy "CDUP" string to buff
        // Outputs from index 3 to argc-1 of the argv array and adds to the buff
        //cd... Since this CDUP should be tied up, from 3
        for (int i = 3; i < argc; i++) { 
            strcat(buf, " ");
            strcat(buf, argv[i]); // Add string from current index to buff
        }

    }

    //change mldir to MKD
    else if (strcmp(argv[1], "mkdir") == 0){
        strcpy(buf, "MKD");//Copy "MKD" stirng to buff
        // Outputs from index 2 to argc-1 of the argv array and adds to the buff
        for (int i = 2; i < argc; i++) {
            strcat(buf, " ");
            strcat(buf, argv[i]); // Add string from current index to buff
        }
    }

    //change delete to DELE
    else if (strcmp(argv[1], "delete") == 0){
        strcpy(buf, "DELE");//Copy "DELE" string to buff
        // Outputs from index 2 to argc-1 of the argv array and adds to the buff
        for (int i = 2; i < argc; i++) {
            strcat(buf, " ");
            strcat(buf, argv[i]); // Add string from current index to buff
        }
    }    

    //change rmdir to RWD
    else if (strcmp(argv[1], "rmdir") == 0){
        strcpy(buf, "RMD"); //Copy "RMD" string to buff
        // Outputs from index 2 to argc-1 of the argv array and adds to the buff
        for (int i = 2; i < argc; i++) {
            strcat(buf, " ");
            strcat(buf, argv[i]); // Add string from current index to buff
        }
    }

    //change rename to RNFR A RNTO B 
    else if (strcmp(argv[1], "rename") == 0) {
        //If the factor comes in normally
        if(argc == 4){
            //save buff RNFR A RNTO B 
            snprintf(buf, sizeof(buf), "RNFR %s RNTO %s", argv[2], argv[3]);
        }
        //If there are fewer factors
        else if(argc<4) {
            //print error
            write(2,"two arguments are required\n",strlen("two arguments are required\n"));
            exit(EXIT_FAILURE);
        }
        //If there are many factors'
        else if(argc>4){
        //print error
            write(2,"two arguments are required\n",strlen("two arguments are required\n"));
            exit(EXIT_FAILURE);
        }
        }
    

    //change quit to QUIT
    else if (strcmp(argv[1], "quit") == 0){
        strcpy(buf, "QUIT"); //Copy "QUIT" stirng to buffer
        // Outputs from index 2 to argc-1 of the argv array and adds to the buff
        for (int i = 2; i < argc; i++) {
            strcat(buf, " ");
            strcat(buf, argv[i]); // Add string from current index to buff
        }
    }
    //Not in Linux command form
    else {
        fprintf(stderr, "%s: command not found\n", argv[1]);
        //fail exit
        exit(EXIT_FAILURE);
    }
///////////////////////Converting Linux commands to ftp commands END///////////////////
    // Forward the commands stored in the buff to src.v
    if (write(1, buf, strlen(buf)) == -1) {
        perror("write"); //error case
        exit(EXIT_FAILURE); //return fail
    } 
    write(1,"\n",1);      
    //return success
    return EXIT_SUCCESS;
}
