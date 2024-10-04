/////////////////////////////////////////////////////////////////////
// File Name : srv.c                                               //
// Date : 2024/05/16                                               //
// OS : Ubuntu 20.04.6 LTS 64bits                                  //
//                                                                 //
// Author : Hong Wang ki                                           //
// Student ID : 2020202060                                         //
// ---------------------------------------------------------------- //
// Title : System Programming Assignment #3-1 ftp server           //
// Description: Project on ID and password by                      //
//              communicating with server through socket           //
/////////////////////////////////////////////////////////////////////
// Function: main, load_users, user_match, match_ip,               //
//           is_ip_allowed, log_auth                               //
// =================================================================
// Input: (main Criteria)                                           //
//     argc: number of command-line arguments                      //
//     argv: array of command-line arguments                       //
//                                                                  //
// (Input parameter Description)                                    //
//     - argc: number of command-line arguments                    //
//     - argv: array of command-line arguments, where argv[0] is    //
//             the program name and argv[1] (if provided) is the    //
//             port number                                          //
//                                                                  //
// Output:                                                          //
//     success or fail                                              //
//                                                                  //
// Purpose:                                                         //
//     This code implements a TCP server for user authentication    //
//     and IP-based access control. The main functionalities        //
//     include:                                                     //
//                                                                  //
//     User Data Management:                                        //
//     - Define a structure to store user information.              //
//     - Load user data from a file (passwd), populating the user   //
//       information structure.                                     //
//                                                                  //
//     User Authentication:                                         //
//     - A function to authenticate users based on their username   //
//       and password.                                              //
//     - A login authentication function that allows up to three    //
//       login attempts before disconnecting.                      //
//                                                                  //
//     IP Address Matching:                                         //
//     - Functions to parse and match IP addresses against          //
//       patterns, supporting wildcards (*).                        //
//     - A function to check if a client's IP address is allowed    //
//       based on an access control file (access.txt).              //
//                                                                  //
//     Server Operations:                                           //
//     - Set up a server socket to listen for incoming connections. //
//     - Accept and handle client connections.                      //
//     - Perform IP address validation and user authentication.     //
//     - Send appropriate responses to the client based on          //
//       authentication and access control results.                 //
//                                                                  //
//     This setup allows a server to control access based on        //
//     predefined user credentials and IP address patterns,         //
//     enhancing security by ensuring only authenticated users      //
//     from allowed IP addresses can connect.                       //
////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pwd.h>

#define MAX_BUF 100
#define MAX_USERS 10

//struch information
typedef struct {
    char username[MAX_BUF];
    char password[MAX_BUF];
    char user_id[MAX_BUF];
    char group_id[MAX_BUF];
    char real_name[MAX_BUF];
    char home_directory[MAX_BUF];
    char shell_program[MAX_BUF];
} User;

User users[MAX_USERS];
int user_count = 0;

///////// Function to load users from the passwd file/////////
int load_users(const char *filename) {
    FILE *fp;
    struct passwd *pwd;
    // Open the passwd file
    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen");
        return 0;
    }
    // Read each entry from the passwd file
    while ((pwd = fgetpwent(fp)) != NULL) {
        // Copying user information to our structure
        strncpy(users[user_count].username, pwd->pw_name, MAX_BUF);
        strncpy(users[user_count].password, pwd->pw_passwd, MAX_BUF);
        snprintf(users[user_count].user_id, MAX_BUF, "%d", pwd->pw_uid);
        snprintf(users[user_count].group_id, MAX_BUF, "%d", pwd->pw_gid);
        strncpy(users[user_count].real_name, pwd->pw_gecos, MAX_BUF);
        strncpy(users[user_count].home_directory, pwd->pw_dir, MAX_BUF);
        strncpy(users[user_count].shell_program, pwd->pw_shell, MAX_BUF);

        user_count++;
    }
    // Close the passwd file
    fclose(fp);
    endpwent(); // Close the password database

    return 1;
}
////////////////load users fun end///////////////////

//////////Function to match user credentials///////////
int user_match(char *user, char *passwd) {
    for (int i = 0; i < user_count; i++) {        
        int user_cmp = strcmp(user, users[i].username);
        int passwd_cmp = strcmp(passwd, users[i].password);
        if (user_cmp == 0 && passwd_cmp == 0) {
            return 1; // Authentication success
        }
    }
    return 0; // Authentication failure
}
////////////user_match end/////////////

///// Function to match IP against a pattern/////
int match_ip(char *ip, char *pattern) {
    char ip_copy[MAX_BUF];
    strcpy(ip_copy, ip); // Copy IP to avoid modifying the original


    char ip_parts[4][4], pattern_parts[4][4];
    char *ip_token, *pattern_token;

    // Parse the IP address
    ip_token = strtok(ip_copy, ".");
    for (int i = 0; i < 4; i++) { // Missing parts in pattern
        strncpy(ip_parts[i], ip_token, 4);
        ip_token = strtok(NULL, ".");
    }
     // Parse the pattern
    pattern_token = strtok(pattern, ".");
    for (int i = 0; i < 4; i++) {
       if (pattern_token == NULL) // Missing parts in pattern
            return 0;
        strncpy(pattern_parts[i], pattern_token, 4);
        pattern_token = strtok(NULL, ".");
    }
    // pattern over
    if (pattern_token != NULL)
        return 0;

     // Compare IP parts with pattern parts
    for (int i = 0; i < 4; i++) {
        if (strcmp(pattern_parts[i], "*") == 0)
            continue; // Skip wildcard parts
        if (strcmp(ip_parts[i], pattern_parts[i]) != 0)
            return 0; // Parts do not match
    }

    return 1; // All parts match
}
//////////////////match_ip/////////////////////

// Function to check if an IP is allowed
int is_ip_allowed( char *ip) {
    FILE *fp;
    char line[MAX_BUF]; 
    // Open the access control file
    fp = fopen("access.txt", "r");
    if (fp == NULL) {
        perror("fopen");
        return 0;
    }

    int allowed = 0; // Variable to store allowance state
      // Check each line in the access file
    while (fgets(line, sizeof(line), fp) != NULL) {
        line[strcspn(line, "\n")] = '\0'; // Remove trailing newline
        if (match_ip(ip, line)) {
            allowed = 1; // IP is allowed
            break; // Stop checking further
        }
    }
    
    fclose(fp);

    if (allowed)
        return 1; 
    else
        return 0; 
}
///////////////////is_ip_allowed fun end/////////////////////


/////////////////log_auth fun start/////////////////////
int log_auth(int connfd) {
    char user[MAX_BUF], passwd[MAX_BUF],user_passwd[MAX_BUF];
    int n, count = 0; 
    // Read user credentials from the client
    while (1) { 
        if((n = read(connfd, user_passwd, MAX_BUF))<=0){
            perror("read err: ");
            return 0;
        }
        user_passwd[n-1] = '\0';

        // Separate username and password
        char *user = strtok(user_passwd, ":");
         if (user == NULL) {
            user ="";
        }
        char *passwd = strtok(NULL, ":");
          if (passwd == NULL) {
            passwd="";
        }
        // Send acknowledgment to the client
        write(connfd, "OK", 3);
        printf("** User is trying to log-in (%d/3) **\n",count+1);
        char buf[10];
        if(read(connfd, buf, 10)<=0){
            perror("read err: ");
            return 0;
        }

         // Attempt to authenticate the user
        if (user_match(user, passwd) == 1) {
            // Authentication successful
            write(connfd, "OK", strlen("OK"));
            printf("** User '%s' logged in **\n", user);
            return 1; // Login success
        } 
            
        else if (count >= 2) { 
           // Disconnect after 3 failed attempts
            write(connfd, "DISCONNECTION", strlen("DISCONNECTION"));
            return 0; // 3 attempts failed
            }
        else {
            // Authentication failed
            write(connfd, "FAIL", 5);
            printf("** Log-in failed **\n");
            count++;
        }
    } 
    // This line is never reached
    return 1; 
}
//////////////////////log_auth/////////////////////////

///////////////////////main start/////////////////////
int main(int argc, char *argv[]) {
     // Check if port number is provided
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Port number>\n", argv[0]);
        exit(1);
    }
    // Load users from the passwd file
    if (!load_users("passwd")) {
        fprintf(stderr, "Error: Failed to load users from passwd\n");
        exit(1);
    }

    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    char client_ip[INET_ADDRSTRLEN];
    int client_port;
    // Create a socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket");
        exit(1);
    }
     // Initialize the server address structure
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));
     // Bind the socket to the specified port
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        close(listenfd);
        exit(1);
    }
    // Listen for incoming connections
    listen(listenfd, 5);
    // Server main loop
    for (;;) {
        clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (connfd < 0) {
            perror("accept");
            continue;
        }
        // Get the client's IP address and port
        inet_ntop(AF_INET, &cliaddr.sin_addr, client_ip, sizeof(client_ip));
        client_port = ntohs(cliaddr.sin_port);

        printf("** Client is trying to connect **\n");
        printf("- IP: %s\n", client_ip);
        printf("- Port: %d\n", client_port);
        printf("** Client is connected **\n");
       // Check if the client's IP is allowed
        if (!is_ip_allowed(client_ip)) {
            write(connfd, "REJECTION", strlen("REJECTION"));
            printf("** It is NOT an authenticated client **\n");
            close(connfd);
            continue;
        } else {
            write(connfd, "ACCEPTED",strlen("ACCEPTED"));
        }
        // Perform login authentication
        if (log_auth(connfd) == 0) {
           printf("** Fail to log-in **\n");
           continue;
        }
        printf("** Success to log-in **\n"); 
        close(connfd);
    }

    close(listenfd);
    return 0;
}
/////////////////////main fun end///////////////////////