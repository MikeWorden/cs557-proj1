

#include "../include/manager.h"
#include <pthread.h>




/******************************************************************************
* connect_to_manager:  Connects to manager to pull the configs
*
******************************************************************************/


void connect_to_manager(int manager_port, int tracker_port, struct client *clients, int num_clients) {


    struct client client_list[MAX_CLIENTS];
    struct client tmp_client;
    int client_sockfd;
    struct sockaddr_in manager_servaddr;
    int read_size;
    char client_message[5000];
    int index = 0;


    client_sockfd = socket(AF_INET, SOCK_STREAM,0);
    bzero(&manager_servaddr, sizeof manager_servaddr);
    manager_servaddr.sin_family = AF_INET;
    manager_servaddr.sin_port=htons(manager_port);

    inet_pton(AF_INET,"127.0.0.1", &(manager_servaddr.sin_addr));

    connect(client_sockfd, (struct sockaddr *)&manager_servaddr, sizeof(manager_servaddr));

    char client_helloline[80];
    sprintf(client_helloline, "%d: Hello from Tracker\n", tracker_port );



    write(client_sockfd,client_helloline, strlen(client_helloline));
    while(((index < 25 ) && (read_size = (int)recv(client_sockfd , client_message , sizeof(client_message) , 0)) > 2 ) ) {

        TDEBUG_PRINT(("Tracker:  Getting Client %d\n", index));
        
        init_client(&tmp_client);
        client_record_deserialization(client_message, &tmp_client);
        copy_client(&tmp_client, &client_list[index]);
        index++;
    }
    TDEBUG_PRINT(("TRACKER:  Got all clients\n\n\n"));
    for (int i =0; i<MAX_CLIENTS; i++) {

    }


    close(client_sockfd);
}

/******************************************************************************
* launch_tracker:  Initialize the tracker process
*
******************************************************************************/

void launch_tracker( int manager_port) {


    int port;
    struct client tracker_clients[MAX_CLIENTS];

    void *tracker_connection_handler(void *);


    TDEBUG_PRINT(("Tracker:  Launching Tracker & logging to tracker.out\n"));


    int socket_desc  ;
    struct sockaddr_in server_addr ;
    struct sockaddr_in client_addr;
    socklen_t client_addr_length;
    char buffer[BUFSIZE];
    int bytes_recieved;
    char *log_file = "tracker.out";
    char log_message[80];


    create_log(log_file);

    log_entry(log_file, "type Tracker\n\n");
    sprintf(log_message, "pid %d\n", getpid());
    log_entry(log_file, log_message);

    //Create socket
    socket_desc = socket(AF_INET , SOCK_DGRAM , 0);
    if (socket_desc == -1) {
        perror("Tracker:  Could not create socket\n");
    } else {
        TDEBUG_PRINT(("Tracker:  Socket created\n"));
    }


    //Prepare the sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons( 0 );
    int optval = 1;


    //setsockopt: Handy debugging trick that lets us rerun the server
    //Not sure if it's necessary as we're dynamically allocating sockets
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server_addr , sizeof(server_addr)) < 0)
    {
        //print the error message
        perror("Tracker:  bind failed. Error\n");
        return ; //
    }
    TDEBUG_PRINT(("Tracker:  bind done\n"));


    // Get port number for our client & announce it.
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(socket_desc, (struct sockaddr *)&sin, &len) == -1)
        perror("Tracker:  getsockname error in Tracker\n");
    else {
        port = ntohs(sin.sin_port);
        TDEBUG_PRINT(("Tracker:  Tracker is listening on port number %d\n", port));
    }
    sprintf(log_message, "tport %d\n", port);
    log_entry(log_file, log_message);
    connect_to_manager(manager_port, port, tracker_clients, MAX_CLIENTS);



    // Barebones UDP Server
    // Learned this from an excellent example at http://web.cs.wpi.edu/~cs3516/b09/samples/listen-udp.c

    // initialize this with size of a sockaddr_in
    client_addr_length = sizeof(client_addr);
    TDEBUG_PRINT(("Tracker:  Entering Listen Loop\n "));
    while (1){
        bzero(buffer, BUFSIZE);
        bytes_recieved = (int)recvfrom(socket_desc, buffer, BUFSIZE, 0,
                            (struct sockaddr *) &client_addr, &client_addr_length);

        if (bytes_recieved <0 ) {
            perror("Tracker:  Error in recvfrom\n");
        }
        TDEBUG_PRINT(("Tracker:   Got datagram of %d bytes\n", bytes_recieved));
        TDEBUG_PRINT(("Tracker:  Recieved:  %s\n", buffer ));
        printf("Phase 1 Tracker:  Recieved:  %s\n", buffer );

    }


    TDEBUG_PRINT(("Tracker:  Launch_Tracker() complete\n"));
    return ;

}


/******************************************************************************
* tracker_connection_handler:  Kinda superfulous;
*
******************************************************************************/
void *tracker_connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char *message , client_message[2000];

    //Send some messages to the client
    message = "Greetings! I am your Tracker\n";
    write(sock , message , strlen(message));

    message = "Now type something and i shall repeat what you type \n";
    write(sock , message , strlen(message));

    //Receive a message from client
    while( (read_size = (int)recv(sock , client_message , 2000 , 0)) > 2 )
    {
        //Send the message back to client
        write(sock , client_message , strlen(client_message));
        TDEBUG_PRINT(("Tracker Received %zu char msg:  %s ", strlen(client_message), client_message));
    }

    if(read_size == 2)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("Tracker Recv failed");
    }

    close(sock);

    //Free the socket pointer
    free(socket_desc);

    return 0;
}
