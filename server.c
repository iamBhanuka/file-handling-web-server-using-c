#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>

#define CONNMAX 1000
#define BYTES 1024

char *ROOT;
int listenfd, clients[CONNMAX];
void error(char *);

//this is use to start the server
void startServer(char *port)
{
    struct addrinfo server_addr, *res, *p;

    // getaddrinfo for host
    memset (&server_addr, 0, sizeof(server_addr));
    server_addr.ai_family = AF_INET;
    server_addr.ai_socktype = SOCK_STREAM;
    server_addr.ai_flags = AI_PASSIVE;
    if (getaddrinfo( NULL, port, &server_addr, &res) != 0)
    {
        perror ("g/etaddrinfo() error");
        exit(1);
    }
    // this is use socket and bind
    for (p = res; p!=NULL; p=p->ai_next)
    {
        listenfd = socket (p->ai_family, p->ai_socktype, 0);
        if (listenfd == -1) continue;
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
    }
    if (p==NULL)
    {
        perror ("socket() or bind() Error");
        exit(1);
    }

    freeaddrinfo(res);

    // listen for incoming connections
    if ( listen (listenfd, 1000000) != 0 )
    {
        perror("listen() error");
        exit(1);
    }
}
//this method use client connection
void respond(int clinet)
{
    char msg[99999], *reqline[3], data_to_send[BYTES], path[99999];
    int rcvd, fd, bytes_read;

    memset( (void*)msg, (int)'\0', 99999 );

    rcvd=recv(clients[clinet], msg, 99999, 0);

    if (rcvd<0)    // receive error
        fprintf(stderr,("recv() error\n"));
    else if (rcvd==0)    // receive socket closed
        fprintf(stderr,"Client disconnected upexpectedly.\n");
    else    // message received
    {
        printf("%s", msg);
        reqline[0] = strtok (msg, " \t\n");
        if ( strncmp(reqline[0], "GET\0", 4)==0 )
        {
            reqline[1] = strtok (NULL, " \t");
            reqline[2] = strtok (NULL, " \t\n");
            if ( strncmp( reqline[2], "HTTP/1.0", 8)!=0 && strncmp( reqline[2], "HTTP/1.1", 8)!=0 )
            {
                write(clients[clinet], "HTTP/1.0 400 Bad Request\n", 25);
            }
            else
            {
                if ( strncmp(reqline[1], "/\0", 2)==0 )
                    reqline[1] = "/index.html";        //Because if no file is specified, index.html will be opened by default (like it happens in APACHE...

                strcpy(path, ROOT);
                strcpy(&path[strlen(ROOT)], reqline[1]);
                printf("file: %s\n", path);

                if ( (fd=open(path, O_RDONLY))!=-1 )    //FILE FOUND
                {
                    send(clients[clinet], "HTTP/1.0 200 OK\n\n", 17, 0);
                    while ( (bytes_read=read(fd, data_to_send, BYTES))>0 )
                        write (clients[clinet], data_to_send, bytes_read);
                }
                else    write(clients[clinet], "HTTP/1.0 404 Not Found\n", 23); //FILE NOT FOUND
            }
        }
    }

    //Close the SOCKET
    shutdown (clients[clinet], SHUT_RDWR);         //All further send and recieve operations are DISABLED...
    close(clients[clinet]);
    clients[clinet]=-1;
}

int main(int argc, char* argv[])
{
    struct sockaddr_in client_addr;
    socklen_t addrlen;
    char c;    
    
    //Default Values PATH = ~/ and PORT=8080
    char PORT[6];
    ROOT = getenv("PWD");
    strcpy(PORT,"8080");

    int slot=0;

    //Parsing the command line arguments
    while ((c = getopt (argc, argv, "p:r:")) != -1)
        switch (c)
        {
            case 'r':
                ROOT = malloc(strlen(optarg));
                strcpy(ROOT,optarg);
                break;
            case 'p':
                strcpy(PORT,optarg);
                break;
            case '?':
                fprintf(stderr,"Wrong arguments given!!!\n");
                exit(1);
            default:
                exit(1);
        }
    
    printf("Server started at port no. %s%s%s with root directory as %s%s%s\n","\033[92m",PORT,"\033[0m","\033[92m",ROOT,"\033[0m");
    // Setting all elements to -1: signifies there is no client connected
    int i;
    for (i=0; i<CONNMAX; i++)
        clients[i]=-1;
    startServer(PORT);

    // Accept the connections
    while (1)
    {
        addrlen = sizeof(client_addr);
        clients[slot] = accept (listenfd, (struct sockaddr *) &client_addr, &addrlen);

        if (clients[slot]<0)
            error ("accept() error");
        else{
            if ( fork()==0 ){
                respond(slot);
                exit(0);
            }
        }

        while (clients[slot]!=-1) slot = (slot+1)%CONNMAX;
    }

    return 0;
}



