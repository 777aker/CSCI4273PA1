/*
 * Kelley Kelley
 * CSCI 4273 PA1
 * A programming assignment using UDP to transfer files
 * to/from client/server
 * This is the server portion
 */

// imports
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <dirent.h>
#include <fcntl.h>

// buffer size
#define BUFSIZE 1024

// error - wrapper for perror
void error(char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char **argv) {
    // c says you gotta define every variable first bc memory space
    // so here defining variables
    int sockfd; // socket
    int portno; // port of server
    int clientlen; // byte size of client's address
    struct sockaddr_in serveraddr; // server's address
    struct sockaddr_in clientaddr; // client's address
    struct hostent *hostp;
    char buf[BUFSIZE]; // message buffer
    char ret_buf[BUFSIZE]; // message to return to client
    char *hostaddrp; // dotted decimal host address string
    int optval; // flag value for setsockopt
    int n; // message byte size
    int placeholder; // current byte placeholder
    char *split_buf[2]; // going to have to split our messages on
    FILE *fp; // file pointer
    bool command_found = false; // boolean for if the command matched something
    
    // spaces to process commands
    // size is two since no command is more than two words
    
    // check command line arguments - should be 2
    if(argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    // get port from arguments
    portno = atoi(argv[1]);
    
    // create parent socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    // make sure socket worked
    if(sockfd < 0)
        error("error opening socket");
    
    /* setsockopt: Handy debugging trick that lets 
     * us rerun the server immediately after we kill it; 
     * otherwise we have to wait about 20 secs. 
     * Eliminates "ERROR on binding: Address already in use" error. 
     */
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
        (const void *)&optval , sizeof(int));
    
    // build server's internet address
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short) portno);
    
    // bind port, if -1 then error so handle that
    if(bind(sockfd, (struct sockaddr *) &serveraddr,
           sizeof(serveraddr)) < 0)
        error("error on binding");
    
    // main loop
    clientlen = sizeof(clientaddr);
    while (1) {
        
        // clear buffer
        bzero(buf, BUFSIZE);
        // recvfrom: reveive UDP datagram from client
        n = recvfrom(sockfd, buf, BUFSIZE, 0,
                    (struct sockaddr *) &clientaddr, &clientlen);
        if(n < 0)
            error("error in recvfrom");
        
        // gethostbyaddr: determine who sent datagram
        hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                             sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        if(hostp == NULL)
            error("error on gethostbyaddr");
        hostaddrp = inet_ntoa(clientaddr.sin_addr);
        if(hostaddrp == NULL)
            error("error on inet_nota");
        printf("server received datagram from %s (%s)\n",
              hostp->h_name, hostaddrp);
        printf("server received %ld/%d bytes: %s\n", strlen(buf), n, buf);
        
        // now gotta switch based on the command recieved
        // First check if command was exit
        // gonna use strncmp()
        // technically they could say like exitlol not actually
        // and it would still exit but that's what i want to happen
        if(strncmp("exit", buf, 4) == 0) {
            // exit gracefully
            // I don't really know how to do that other than close the socket
            printf("server exiting\n");
            n = sendto(sockfd, "server exiting", strlen("server exiting"), 0,
                      (struct sockaddr *) &clientaddr, clientlen);
            close(sockfd);
            exit(0);
        }
        
        // next is ls
        if(strncmp("ls", buf, 2) == 0) {
            // we found a command so change this to true
            command_found = true;
            // for testing
            //printf("inside ls\n");
            // reset our message to send buffer
            bzero(ret_buf, BUFSIZE);
            // copy our message into the buffer
            // testing
            //strncpy(ret_buf, "reading directory", 17);
            // testing
            //printf("message: %s\n", ret_buf);
            //printf("length: %ld\n", strlen(ret_buf));
            // testing
            /*
            n = sendto(sockfd, ret_buf, strlen(ret_buf), 0,
                      (struct sockaddr *) &clientaddr, clientlen);
            if(n < 0)
                error("error in sendto");
             */
            //TODO: copy directory into ret_buf and return ret_buf
            // figured out how to get directory from
            // https://www.geeksforgeeks.org/c-program-list-files-sub-directories-directory/
            struct dirent *de;
            // open current directory
            DIR *dr = opendir(".");
            if(dr == NULL)
                printf("Could not open current directory");
            // they just printed but I want to put all files into buffer
            // which means I need to keep track of where in the buffer
            // we currently are using placeholder
            placeholder = 0;
            // while there are directories copy them to the buffer
            while((de = readdir(dr)) != NULL) {
                // copy to the buffer at location buffer+placeholder
                strncpy(ret_buf+placeholder, 
                        de->d_name, strlen(de->d_name));
                // copy in a ", " just so it's formatted nicely
                strncpy(ret_buf+placeholder+strlen(de->d_name), ", ", 2);
                // update placeholder so it's at the end of our buffer
                placeholder = placeholder + strlen(de->d_name)+2;
                // testing
                //printf("ls ret_buf in while: %s\n", ret_buf);
                //printf("ls placeholder: %d\n", placeholder);
            }
            // reset placeholder
            placeholder = 0;
            // close directory
            closedir(dr);
            printf("files in local directory return to client: \n%s\n", ret_buf);
            // send to client, -2 is to get rid of ", " at end of ret_buf
            n = sendto(sockfd, ret_buf, strlen(ret_buf)-2, 0,
                       (struct sockaddr *) &clientaddr, clientlen);
            if(n < 0)
                error("error in sendto");
            // ls done yay
            // this does assume that your directory files are less than 1024 bytes
            // kind of scared we have to account for that but 
            // you have way too many files in one place if that's the case
        }
        
        // now I'm going to do delete
        if(strncmp("delete", buf, 6) == 0) {
            // command found
            command_found = true;
            // reset our message to return
            bzero(ret_buf, BUFSIZE);
            // get the file we are deleting
            // the +7 is to get rid of "delete "
            // the -7 is because the size is -7 since removing delete
            strncpy(ret_buf, buf+7, strlen(buf)-7);
            // for testing purposes
            //printf("file: %s\n", ret_buf);
            // for testing purposes
            /*
            n = sendto(sockfd, ret_buf, strlen(ret_buf), 0,
                      (struct sockaddr *) &clientaddr, clientlen);
            if(n < 0)
                error("error in sendto");
             */
            // TODO: delete the file and return file deleted
            if(remove(ret_buf) == 0) {
                printf("Deleted file: [%s]\n", ret_buf);
                n = sendto(sockfd, "Deleted file\n", 
                           strlen("Deleted file\n"), 0,
                      (struct sockaddr *) &clientaddr, clientlen);
                if(n < 0)
                    error("error in sendto");
            } else {
                printf("Couldn't delete file: [%s]\n", ret_buf);
                n = sendto(sockfd, "Couldn't delete file. File may not exist\n", 
                           strlen("Couldn't delete file. File may not exist\n"), 0,
                      (struct sockaddr *) &clientaddr, clientlen);
                if(n < 0)
                    error("error in sendto");
            }
        }
        
        // now get command
        if(strncmp("get", buf, 3) == 0) {
            // command found
            command_found = true;
            // reset our ret buffer
            bzero(ret_buf, BUFSIZE);
            // get file name
            strncpy(ret_buf, buf+4, strlen(buf)-4);
            //TODO: transmit the requested file to the client
            // open the file
            fp = fopen(ret_buf, "r");
            if(!fp) {
                n = sendto(sockfd, "failed", 6, 0,
                          (struct sockaddr *) &clientaddr, clientlen);
                if(n < 0)
                    error("error in sendto");
                printf("error getting file\n");
            } else {
                // read file and send one buffer size at a time
                while(fgets(buf, BUFSIZE, fp)) {
                    n = sendto(sockfd, buf, strlen(buf), 0,
                              (struct sockaddr *) &clientaddr, clientlen);
                    if(n < 0)
                        error("error in sendto");
                    // testing
                    //printf("oh no");
                }
                // send end to client so it knows file transfer over
                n = sendto(sockfd, "end", 3, 0,
                          (struct sockaddr *) &clientaddr, clientlen);
                if(n < 0)
                    error("error in sendto");
                printf("finished getting file\n");
                //close the file
                fclose(fp);
            }
        }
        
        // finally put command
        if(strncmp("put", buf, 3) == 0) {
            // command found
            command_found = true;
            // reset our ret buffer
            bzero(ret_buf, BUFSIZE);
            // get file name
            strncpy(ret_buf, buf+4, strlen(buf)-4);
            //TODO: receive transmitted file from client and store it locally
            // clear ret_buf
            bzero(ret_buf, BUFSIZE);
            // get filename
            strncpy(ret_buf, buf+4, strlen(buf)-4);
            printf("putting file: %s\n", ret_buf);
            // open file
            fp = fopen(ret_buf, "w");
            // clear buffer
            bzero(buf, BUFSIZE);
            // get first client datagram
            n = recvfrom(sockfd, buf, BUFSIZE, 0,
                        (struct sockaddr *) &clientaddr, &clientlen);
            if(n < 0)
                error("error in recvfrom");
            // I probably should've written a function for n error check but oh well, too late now
            // while we haven't sent final transmission
            while(strncmp("end", buf, 3) != 0) {
                // put data into file
                fputs(buf, fp);
                // clear buffer
                bzero(buf, BUFSIZE);
                // get next datagram
                n = recvfrom(sockfd, buf, BUFSIZE, 0,
                            (struct sockaddr *) &clientaddr, &clientlen);
                if(n < 0)
                    error("error in recvfrom");
            }
            printf("finished putting file\n");
            // close file
            fclose(fp);
            
        }
        
        if(command_found) {
            // reset command found for next loop
            command_found = false;
        } else {
            // if the command didn't match anything
            // then echo what the client said
            n = sendto(sockfd, buf, strlen(buf), 0,
                      (struct sockaddr *) &clientaddr, clientlen);
            if(n < 0)
                error("error in sendto");
        }
          // testing  
        //printf("made it to end of loop\n");
        
    }
}