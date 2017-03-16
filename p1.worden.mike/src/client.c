

#include "../include/manager.h"

/******************************************************************************
* launch_client:   Launches every client
*
******************************************************************************/


void launch_client( int manager_port,  int client_id) {






    char        log_file[80] ;
    char        log_message[80];


    int         client_sockfd = 0;
    int         socket_desc = 0;
    int         tracker_port = -1;
    int         port  = 0;
    
    long         read_size;
    char         client_helloline[80];
    char        client_message[5000];

    struct client tmp_client;
    char        buffer[BUFSIZE];
    int         bytes_recieved;

    struct      sockaddr_in client_addr;
    socklen_t   client_addr_length;


    struct sockaddr_in server_addr ;
    struct timeval tv;
    tv.tv_sec = 5;  /* 5 Secs Timeout */
    tv.tv_usec = 0;


    /*  Simple Hack to make sure the clients don't connect before the manager
        has information on the Tracker, including it's port!                 */
    sleep(5);
    CDEBUG_PRINT(("Client:  Launching Client: %d & logging to file %02d.out\n", client_id, client_id));


    sprintf(log_file, "%02d.out", client_id);
    create_log(log_file);

    log_entry(log_file, "type Client\n\n");

    sprintf(log_message, "myID %02d\n", client_id);
    log_entry(log_file, log_message);
    sprintf(log_message, "pid %d\n", getpid());
    log_entry(log_file, log_message);



    CDEBUG_PRINT(("Client %d:  Launching our UDP Server\n", client_id));
    //Create socket for our UDP listener
    socket_desc = socket(AF_INET , SOCK_DGRAM , 0);
    if (socket_desc == -1) {
        perror("Client:  Could not create socket\n");
    } else {
        CDEBUG_PRINT(("Client %d:  Socket created\n", client_id));
    }

  
    //Prepare the sockaddr_in structure for our UDP listener
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons( 0 );
    int optval = 1;

    //setsockopt: Handy debugging trick that lets us quickly rerun the server
    //Not sure if it's necessary, since we're dynamically allocating ports
    
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
    CDEBUG_PRINT(("Client %d:  Binding our server\n", client_id));
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server_addr , sizeof(server_addr)) < 0)
    {
        //print the error message
        perror("Client:  bind failed. Error\n");
        return ; //
    }
    
    // Get port number for our client & announce it.
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(socket_desc, (struct sockaddr *)&sin, &len) == -1)
        perror("Client:  getsockname error in Tracker\n");
    else {
        port = ntohs(sin.sin_port);
        CDEBUG_PRINT(("Client  %d:  Client is listening on port number %d\n", client_id, port));
    }


    CDEBUG_PRINT(("Client %d:  Connecting to manager\n", client_id));
    // create client connection to manager process
    struct sockaddr_in manager_servaddr;

     // bzero seems to do the same thing as memset(buf, '/0',sizeof(buf))
    bzero(&manager_servaddr, sizeof manager_servaddr);


    
    manager_servaddr.sin_family = AF_INET;
    manager_servaddr.sin_port=htons(manager_port);
    inet_pton(AF_INET,"127.0.0.1", &(manager_servaddr.sin_addr));




    // Chat with our manager!
    sprintf(client_helloline, "%d: Hello from Client\n", client_id);
    

    do {
        int randomTime = rand() %5 + 1;
        sleep (randomTime);
        client_sockfd = socket(AF_INET, SOCK_STREAM,0);
        CDEBUG_PRINT(("Client %d:  Sending %s to manager at port:  %d\n", client_id, client_helloline, manager_port));
        setsockopt(client_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));
        connect(client_sockfd, (struct sockaddr *)&manager_servaddr, sizeof(manager_servaddr));
        
        write(client_sockfd,client_helloline, strlen(client_helloline));
        CDEBUG_PRINT(("Client %d:  Waiting to Receive something\n", client_id));
        read_size = recv(client_sockfd , client_message , sizeof(client_message) , 0);
        CDEBUG_PRINT(("Client %d:  received %s:  %ld bytes\n", client_id, client_message, read_size));
        close(client_sockfd);
        
    } while (read_size <5000);
    
    
    init_client(&tmp_client);
    client_record_deserialization(client_message, &tmp_client);
    read_size = recv(client_sockfd , client_message , sizeof(client_message) , 0);
    tracker_port = (int)strtol(client_message,NULL,10);
    CDEBUG_PRINT(("Client:  Tracker port is %d\n", tracker_port));

    //dummy logic to make read_size warning go away
    // we'll use it in the next phase
    if (read_size != read_size) {
        printf("REMEMBER TO DELETE THESE LINES!!!\n");
    }

    //Mandatory log Message
    sprintf(log_message, "tport %d\n", tracker_port);
    log_entry( log_file, log_message);
    sprintf(log_message, "myPort %d\n", port);
    log_entry(log_file, log_message);

    CDEBUG_PRINT(("Client:%02d has completed Phase 1 logging.\n", client_id));

    // Barebones UDP Server
    // Learned this from an excellent example at http://web.cs.wpi.edu/~cs3516/b09/samples/listen-udp.c

    // initialize this with size of a sockaddr_in
    client_addr_length = sizeof(client_addr);

    while (1){
        bzero(buffer, BUFSIZE);
        bytes_recieved = (int)recvfrom(socket_desc, buffer, BUFSIZE, 0,
                            (struct sockaddr *) &client_addr, &client_addr_length);

        if (bytes_recieved <0 ) {
            //Bad error handling -- spews errors to console when things go awry
            perror("Tracker:  Error in recvfrom\n");
        }
        CDEBUG_PRINT(("Client[%d]:   Recieved datagram of %d bytes\n", client_id, bytes_recieved));
        CDEBUG_PRINT(("Client[%d]:  Message:  %s\n", client_id, buffer ));
        

    }

   // should put a close for our server udp, but we'll never get here
   // close(socket_desc);

}
