#include "../include/manager.h"
#include <stdlib.h>
#include <stdio.h>

/******************************************************************************
* launch_manager:   Launches the tracker & every client
*
******************************************************************************/
void launch_manager(struct client *clients, int *num_clients, int *timeout, int  *management_port) {



    void *manager_connection_handler(void *);
    //void *manager_connection_handler(void *socket_desc, struct client *clients);
    //void connect_to_manager();

    MDEBUG_PRINT(("Manager:  Launching Manager \n"));


    int socket_desc =0;
    int client_sock =0;
    int *new_sock;
    int manager_port = 0;
    int tracker_pid;
    int client_pid;
    //int client_pid;
    struct sockaddr_in server ;
    bool all_configurations_delivered = false;


    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1) {
        MDEBUG_PRINT(("Manager:  Could not create socket"));
    } else {
        MDEBUG_PRINT(("Manager:  Socket created"));
    }


    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 0 );

        //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
        //print the error message
        perror("Manager:  bind failed. Error");
        return ; //
    } else {
        MDEBUG_PRINT(("Manager:  bind done\n"));
    }



    // Get manager port
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(socket_desc, (struct sockaddr *)&sin, &len) == -1)
        perror("Manager:  getsockname");
    else {

        manager_port = ntohs(sin.sin_port);
        MDEBUG_PRINT(("Manager:  Manager is listening on port number %d\n", manager_port));
    }

    tracker_pid = fork();

    if (tracker_pid == 0 ) // Child Process
    {
        MDEBUG_PRINT(("Manager:  Tracker spawned successfully, process %d\n", getpid()));
       launch_tracker(manager_port);
    }
    else if (tracker_pid < 0 )
    {
        perror("Manager:  Forking Tracker Error!\n");
        return ;
    }
    else // Parent Process (holds the manager
    {
        for (int i =0; i < MAX_CLIENTS; i++) {
            if (clients[i].enabled == true ) {
                client_pid = fork();
                //Child Process
                if (client_pid == 0)
                {
                    launch_client(manager_port,  i);
                    MDEBUG_PRINT(("Manager:  Client %d spawned child process %d successfully!\n", i, getpid()));
                    //sleep (5);
                    return ;
                } else if (client_pid < 0) {
                    perror("Manager:  Forking Client Error!\n");
                    return;
                } else {
                // Parent process
                    MDEBUG_PRINT(("Manager:  Manager process %d was forking successful\n", getpid()));

                }
            }
        }

    }

    if (tracker_pid > 0) {
        //Listen
        listen(socket_desc , 3);

        
        struct client_args *client_list = malloc(sizeof(struct client_args));
        client_list->client_list = clients;
        
        
        
        //Accept and incoming connection
        MDEBUG_PRINT(("Manager:  Manager is waiting for incoming connections...\n"));

        
        
        while( (!all_configurations_delivered) && (client_sock = accept(socket_desc, NULL, NULL)) )
        {
            MDEBUG_PRINT(("Manager:  Connection accepted by Manager\n"));

            pthread_t manager_thread;
            new_sock = malloc(1);
            *new_sock = client_sock;
            client_list->client_sock = client_sock;


            if( pthread_create( &manager_thread , NULL ,  manager_connection_handler , (void*) client_list) < 0)
            {
                perror("Manager:  Manager could not create thread for new connection");
                return ;
            }

            //Now join the thread , so that we dont terminate before the thread ends
            //Wait:  For this scale server, this isn't an issue.
            //pthread_join( sniffer_thread , NULL);
            MDEBUG_PRINT(("Manager:  Handler assigned\n"));
            all_configurations_delivered = check_agent_status(client_list);
            
        }

        if (client_sock < 0)
        {
            perror("Manager:  Accept failed in Manager\n");
            return ;
        }


    }


}



/******************************************************************************
* launch_connection_handler:  Handles parceling out configs to Tracker & Clients
*
******************************************************************************/
void *manager_connection_handler(void *args)
{
    const char *tracker_hello = "Hello from Tracker";
    const char *client_hello = "Hello from Client";
    static int tracker_port = -1;
    int client_id = -1;
    struct client_args *clients;
    struct client client_record;
    int sock ;
    int read_size;
    char client_message[80];
    char buffer[5000];
    
   
 

    clients = (struct client_args*)args;
    sock = clients->client_sock;

    
    
    while((read_size = (int)recv(sock , client_message , 80 , 0)) > 2 )
    {
        
        MDEBUG_PRINT(("Manager Received %zu char msg:  %s \n", strlen(client_message), client_message));
        MDEBUG_PRINT(("Tracker Port is %d\n\n", tracker_port));
        
        //recieve a message from tracker
        if (strstr(client_message, tracker_hello) != NULL) {
            
            // Get the tracker port from the Hello Message
            tracker_port = (int)strtol(client_message,NULL,10);
            MDEBUG_PRINT(("Manager got Tracker Port as %d\n", tracker_port));
            clients->tracker_port = tracker_port;
            
            MDEBUG_PRINT(("Manager:  tracker listening on port %d \n", tracker_port));
            
            for (int index =0; index<MAX_CLIENTS; index++)  {
                // Initialize the buffer
                memset(buffer, '\0', sizeof(buffer));
                
                // The serialize the struct to a byte stream
                // (Necessary for reliable transport of a variable-sized struct)
                // Note:  Only works for send/recieve on litte-endian systems
                client_record = clients->client_list[index];
                client_record_serialization(buffer, &client_record);
                
                // Now that we have a byte stream, we know how big our struct is.  (Hint it's always 5000 bytes)
                // And we can write it.
                write(sock, buffer, sizeof(buffer));
            }
            close(sock);
        }
        
        //recieve a message from a client
        if (strstr(client_message, client_hello) != NULL) {
            MDEBUG_PRINT(("INSIDE client logic\n"));
            
            // Get the tracker port from the Hello Message
            client_id = (int) strtol(client_message,NULL,10);
            MDEBUG_PRINT(("Manager:  Client ID:   %d connected\n", client_id));
            
            if ((client_id >= 0) && (client_id<MAX_CLIENTS)) {
                MDEBUG_PRINT(("Sending Client_ID Record %d\n",client_id));
                // Initialize the buffer
                memset(buffer, '\0', sizeof(buffer));
                client_record = clients->client_list[client_id];
                client_record_serialization(buffer, &client_record);
                
                // Now that we have a byte stream, we know how big our struct is.  (Hint it's always 5000 bytes)
                // And we can write it.
                write(sock, buffer, sizeof(buffer));
                memset(buffer, '\0', sizeof(buffer));
                MDEBUG_PRINT(("Manager:  Sending Tracker Port:  %d\n", tracker_port));
                sprintf(buffer, "%d\n", tracker_port);
                write(sock, buffer, sizeof(buffer));
                // mark this client as configured, once we pass the string
                clients->client_list[client_id].configured = true;
            
                
                close(sock);
                
            }
            
            
            
        }
        MDEBUG_PRINT(("Manager:  %d Tracker and %d out of %d Clients configured\n", num_trackers_configured, num_clients_configured, num_clients_enabled));
        // Error handling
        if(read_size == 2)
        {
            puts("Client disconnected");
            fflush(stdout);
        }
        else if(read_size == -1)
        {
            perror("Manager Recv failed");
        }
        
        close(sock);
        
        
    }

    
    
    return(0);
}



