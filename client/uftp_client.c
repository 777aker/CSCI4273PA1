/*
 * Kelley Kelley
 * CSCI 4273 PA1
 * client portion of network systems udp client/server assignment
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>

#define BUFSIZE 1024

// error - wrapper for perror
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    char filename[BUFSIZE];
    FILE *fp; // file pointer
    struct timeval read_timeout; // for socket time out
    
    // check command line arguments
    if(argc != 3) {
        fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
        exit(0);
    }
    // get stuff from command line arguments
    hostname = argv[1];
    portno = atoi(argv[2]);
    
    // create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
        error("error opening socket");
    
    // get server's DNS entry
    server = gethostbyname(hostname);
    if(server == NULL) {
        fprintf(stderr, "error, no such host as %s\n", hostname);
        exit(0);
    }
    
    // build server's internet address
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);
    serverlen = sizeof(serveraddr);
    
     // me trying to creat socket time outs
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 10;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
    
    // prompt user with possible commands
    printf("Type any of the following commands:\n");
    printf("get [file_name] - get file from server\n");
    printf("put [file_name] - send file to server\n");
    printf("delete [file_name] - delete file from server\n");
    printf("ls - list files in directory\n");
    printf("exit - exit server and client\n");
    printf("quit - exit client\n");
    
    // take commands repeatedly
    while(1) {
        // clear message buffer
        bzero(buf, BUFSIZE);
        // prompt user for command
        printf("command: ");
        // get user input
        fgets(buf, BUFSIZE, stdin);
        // remove \n from input
        buf[strcspn(buf, "\n")] = 0;
        
        /* for reference
        // send the message to the server
        serverlen = sizeof(serveraddr);
        n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
        if (n < 0) 
          error("ERROR in sendto");

        // print the server's reply
        n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
        if (n < 0) 
          error("ERROR in recvfrom");
        printf("Echo from server: %s", buf);
        return 0;
        */
        
        // I'll be switching on command so
        // I actually remembered that C has regular else if so server
        // is weird but this won't be
        
        if(strncmp("exit", buf, 4) == 0) {
            // send exit command to server
            n = sendto(sockfd, buf, strlen(buf), 0, 
                       (struct sockaddr *) &serveraddr, serverlen);
            if(n < 0)
                error("error in sendto");
            // receive server's response
            n = recvfrom(sockfd, buf, BUFSIZE, 0, 
                         (struct sockaddr *) &serveraddr, &serverlen);
            if(n < 0)
                error("error in recvfrom");
            // if server returned exiting then successful so exit client
            if(strncmp("server exiting", buf, strlen(buf)) == 0) {
                printf("Server and client exiting\n");
                close(sockfd);
                exit(0);
            } else {
                // server may have failed to exit
                printf("Didn't receive confirmation from server\n");
                printf("Please try again\n");
            }
            
        } else if(strncmp("quit", buf, 4) == 0) {
            // exit client
            printf("client exiting\n");
            close(sockfd);
            exit(0);
            
        } else if(strncmp("ls", buf, 2) == 0) {
            // send command to server
            n = sendto(sockfd, buf, strlen(buf), 0, 
                       (struct sockaddr *) &serveraddr, serverlen);
            if(n < 0)
                error("error in sendto ls");
            // prompt
            printf("Files in directory:\n");
            // receive files from server
            n = recvfrom(sockfd, buf, BUFSIZE, 0, 
                         (struct sockaddr *) &serveraddr, &serverlen);
            if(n < 0)
                error("error in recvfrom");
            // print files
            printf("%s\n", buf);
            
        } else if(strncmp("delete", buf, 6) == 0) {
            // they could've type delete and no file name so handle that
            if(strlen(buf) < 8) {
                printf("error: usage - delete [file_name]\n");
            } else {
                // send command to server
                n = sendto(sockfd, buf, strlen(buf), 0, 
                           (struct sockaddr *) &serveraddr, serverlen);
                if(n < 0)
                    error("error in sendto");
                // receive server's response
                n = recvfrom(sockfd, buf, BUFSIZE, 0, 
                             (struct sockaddr *) &serveraddr, &serverlen);
                if(n < 0)
                    error("error in recvfrom");
                // print server's resposne
                printf("%s", buf);
            }
            
        } else if(strncmp("get", buf, 3) == 0) {
            // they could've type get and no file name so handle that
            if(strlen(buf) < 5) {
                printf("error: usage - get [file_name]\n");
            } else {
                // send command to server
                n = sendto(sockfd, buf, strlen(buf), 0, 
                           (struct sockaddr *) &serveraddr, serverlen);
                if(n < 0)
                    error("error in sendto");
                //TODO: handle receiving file from server
                // clear filename
                bzero(filename, BUFSIZE);
                // get filename from buf
                strncpy(filename, buf+4, strlen(buf)-4);
                printf("getting file: %s\n", filename);
                // get first server response
                n = recvfrom(sockfd, buf, BUFSIZE, 0,
                            (struct sockaddr *) &serveraddr, &serverlen);
                if(n < 0)
                    error("error in recvfrom");
                if(strncmp("failed", buf, 6) == 0) {
                    printf("failed to retrieve file\n");
                } else if(strncmp("end", buf, 3) == 0) {
                    printf("didn't receive any file info, may be empty file\n");
                } else {
                    // open the file
                    fp = fopen(filename, "w");
                    // clear message buffer
                    bzero(buf, BUFSIZE);
                    // while server not transmitting "end"
                    while(strncmp("end", buf, 3) != 0) {
                        // put data into file
                        fputs(buf, fp);
                        // clear message buffer
                        bzero(buf, BUFSIZE);
                        // get next server transmission
                        n = recvfrom(sockfd, buf, BUFSIZE, 0,
                                    (struct sockaddr *) &serveraddr, &serverlen);
                        if(n < 0)
                            error("error in recvfrom");
                    }
                    printf("finished getting file\n");
                    // close the file
                    fclose(fp);
                }  
            }
            
            
        } else if(strncmp("put", buf, 3) == 0) {
            // they could've type put and no file name so handle that
            if(strlen(buf) < 5) {
                printf("error: usage - put [file_name]\n");
            } else {
                //TODO: handle sending file to server
                // clear filename
                bzero(filename, BUFSIZE);
                // copy filename to filename
                strncpy(filename, buf+4, strlen(buf)-4);
                // print statement
                printf("putting file: %s\n", filename);
                // open the file
                fp = fopen(filename, "r");
                // if failed to get file then print error
                if(!fp) {
                    printf("error putting file\n");
                } else {
                    // send the command to the server
                    n = sendto(sockfd, buf, strlen(buf), 0,
                              (struct sockaddr *) &serveraddr, serverlen);
                    if(n < 0)
                        error("error in sendto");
                    // transfer the file to the server in buffer sized packets
                    while(fgets(buf, BUFSIZE, fp)) {
                        n = sendto(sockfd, buf, strlen(buf), 0,
                                  (struct sockaddr *) &serveraddr, serverlen);
                        if(n < 0)
                            error("error in sendto");
                    }
                    // send that we finished sending file
                    n = sendto(sockfd, "end", 3, 0,
                              (struct sockaddr *) &serveraddr, serverlen);
                    if(n < 0)
                        error("error in sendto");
                    // close the file
                    fclose(fp);
                    printf("finished putting file\n");
                }
            }
            
            
        } else {
            // wasn't a command
            // send command to server
            n = sendto(sockfd, buf, strlen(buf), 0, 
                       (struct sockaddr *) &serveraddr, serverlen);
            if(n < 0)
                error("error in sendto");
            // receive server's response
            n = recvfrom(sockfd, buf, BUFSIZE, 0, 
                         (struct sockaddr *) &serveraddr, &serverlen);
            if(n < 0)
                error("error in recvfrom");
            // print server's resposne
            printf("Command not found. Server's response: %s\n", buf);
        }
        
    }
    
}