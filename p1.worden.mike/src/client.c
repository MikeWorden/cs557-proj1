

#include "../include/manager.h"
#include "timers-c.h"


/*
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(1);
}

/******************************************************************************
 * client_tracker_comms:   do a group update with tracker, getting current 
 *                         group membership G.
 *
 ******************************************************************************/


void client_tracker_comms(  struct client client_info, char *logfile) {
    int sockfd;
    struct sockaddr_in tracker_servaddr;
    int trackerlen;
    int response;
    char log_message[80];
    
    struct sockaddr_in client_addr;
    socklen_t client_addr_length;
    
    unsigned char client_buf[256], *bufptr;
    struct group_show_interest grpmsg;
    struct group_assign     ga;
    
    if (!client_info.has_task ) {
        TDEBUG_PRINT(("CLIENT %d HAS NO TASKS\n", client_info.node_id ));

        return;
    }
    TDEBUG_PRINT(("CLIENT %d sending a SHOWING INTEREST messsage\n", client_info.node_id ));

    
    
    
    grpmsg.node_id = client_info.node_id;
    grpmsg.msgtype  = GROUP_SHOW_INTEREST;
    grpmsg.numfiles = 1;
    memset(grpmsg.filename, '\0', sizeof(grpmsg.filename));
    strncpy(grpmsg.filename, client_info.filename, sizeof(grpmsg.filename));
    grpmsg.type = GROUP_REQ_SHARE;
    grpmsg.client_port=client_info.node_port;
    
    
    
    
    CDEBUG_PRINT(("Client %d:  Connecting to tracker on port %d for group update\n", client_info.node_id, client_info.tracker_port));
    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        perror("ERROR opening socket");
    // bzero seems to do the same thing as memset(buf, '/0',sizeof(buf))
    bzero(&tracker_servaddr, sizeof tracker_servaddr);
    
    
    //build the trackers address
    tracker_servaddr.sin_family = AF_INET;
    tracker_servaddr.sin_port=htons(client_info.tracker_port);
    inet_pton(AF_INET,"127.0.0.1", &(tracker_servaddr.sin_addr));
    
    //Send our message to the Tracker
    trackerlen = sizeof(tracker_servaddr);
    memset(client_buf, '\0', sizeof(client_buf));
    bufptr=serialize_group_req(client_buf, &grpmsg);
    
    
    response = (int)sendto(sockfd,(char *)client_buf, sizeof(grpmsg), 0, (struct sockaddr *)&tracker_servaddr, trackerlen);
    sprintf(log_message, "%d to T  GROUP_SHOW_INTEREST %s", client_info.node_id, grpmsg	.filename);
    log_entry(logfile, log_message);
    
    bzero(client_buf, sizeof(client_buf));
    client_addr_length = sizeof(client_addr);
    response = (int)recvfrom(sockfd, client_buf, sizeof(client_buf), 0,
                                   (struct sockaddr *) &client_addr, &client_addr_length);
    
    CDEBUG_PRINT(("Client:   Got datagram of %d bytes from %s port %d\n", response, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port)));
    deserialize_group_assign(client_buf, &ga);
    CDEBUG_PRINT(("\n****************GROUP Assign***********************\n"));
    CDEBUG_PRINT(("Client %d:  message type %d\n", client_info.node_id, ga.msgtype));
    CDEBUG_PRINT(("Client %d:  num_files %d\n", client_info.node_id, ga.num_files));
    CDEBUG_PRINT(("Client %d:  file_name %s\n", client_info.node_id, ga.filename));
    CDEBUG_PRINT(("Client %d:  Num_Neighbors %d\n", client_info.node_id, ga.num_neighbors));
    
    for (int i = 0; i<ga.num_neighbors; i++) {
        CDEBUG_PRINT(("Client %d:  Neighbor[%d] %d\n", client_info.node_id, i, ga.neighbor_id[i]));
        CDEBUG_PRINT(("Client %d:  Neighbor[%d] IP %d.%d.%d.%d\n", client_info.node_id, i, ga.neighbor_ip[i][0], ga.neighbor_ip[i][1], ga.neighbor_ip[i][2], ga.neighbor_ip[i][3]));
        CDEBUG_PRINT(("Client %d:  Port[%d] %d\n", client_info.node_id, i, ga.neighbor_port[i]));
    }
    
    
    CDEBUG_PRINT(("\n****************GROUP Assign***********************\n"));
    
    
    return;
    
    
}

void client_client_comms (int manager_port,  int client_id) {
    struct timeval tmv;
    int status;
    
    /* Change while condition to reflect what is required for Project 1
     ex: Routing table stabalization.  */
    while (1) {
        Timers_NextTimerTime(&tmv);
        if (tmv.tv_sec == 0 && tmv.tv_usec == 0) {
            /* The timer at the head on the queue has expired  */
            Timers_ExecuteNextTimer();
            continue;
        }
        if (tmv.tv_sec == MAXVALUE && tmv.tv_usec == 0){
            /* There are no timers in the event queue */
            break;
        }
        /* The select call here will wait for tv seconds before expiring
         * You need to  modifiy it to listen to multiple sockets and add code for
         * packet processing. Refer to the select man pages or "Unix Network
         * Programming" by R. Stevens Pg 156.
         */
        status = select(0, NULL, NULL, NULL, &tmv);
        
        if (status < 0){
            /* This should not happen */
            fprintf(stderr, "Select returned %d\n", status);
        }else{
            if (status == 0){
                /* Timer expired, Hence process it  */
                Timers_ExecuteNextTimer();
                /* Execute all timers that have expired.*/
                Timers_NextTimerTime(&tmv);
                while(tmv.tv_sec == 0 && tmv.tv_usec == 0){
                    /* Timer at the head of the queue has expired  */
                    Timers_ExecuteNextTimer();
                    Timers_NextTimerTime(&tmv);
                    
                }
            }
            if (status > 0){
                /* The socket has received data.
                 Perform packet processing. */
                
            }
        }
    }
    return;
}




int CProcessTimer(u_char *args)
{
    struct timeval tv;
    //struct client *client = (struct client *)args;
    
    
    getTime(&tv);
    //fprintf(stderr, "Timer %d has expired! Time = %d:%d\n",
    //        client->node_id, (int)tv.tv_sec,(int)tv.tv_usec);
    fflush(NULL);
    return 0;
}



void client_listener(int sockfd, struct client *client, char *logfile)
{
    struct timeval tmv;
    int status;
    fd_set rset;
    int maxfdpl, stdineof;
    unsigned char file_buf[MAX_FILE_SIZE];  //Max size is 20K
    int file_size;
    int record_num = 0;
    
    
    memset(file_buf, '\0', MAX_FILE_SIZE);
    char  send_buf[FILE_BUF_SIZE];
    
    FILE *ofp = fopen(client->filename, "rb");
    if (!ofp) {
        //error ("Error opening outfile\n");
    } else {
        file_size = (int)fread(file_buf, MAX_FILE_SIZE, 1, ofp);
    }
    
    fclose(ofp);
    
    char   client_buf[FILE_BUF_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_addr_length;
    int bytes_recieved;
    
    FILE *fp = fopen(logfile, "a");
    if (!fp) {
        perror("Error appending to  logfile!\n");
        
    }
    stdineof = 0;
    FD_ZERO(&rset);
    /* Change while condition to reflect what is required for Project 1
     ex: Routing table stabalization.  */
    while (1) {
        Timers_NextTimerTime(&tmv);
        if (tmv.tv_sec == 0 && tmv.tv_usec == 0) {
            /* The timer at the head on the queue has expired  */
            Timers_ExecuteNextTimer();
            continue;
        }
        if (tmv.tv_sec == MAXVALUE && tmv.tv_usec == 0){
            /* There are no timers in the event queue */
            break;
        }
        
        /* The select call here will wait for tv seconds before expiring
         * You need to  modifiy it to listen to multiple sockets and add code for
         * packet processing. Refer to the select man pages or "Unix Network
         * Programming" by R. Stevens Pg 156.
         */
        
        FD_SET(fileno(fp), &rset);
        FD_SET(sockfd, &rset);
        maxfdpl = max(sockfd, fileno(fp))+1;
        status = select(maxfdpl, &rset, NULL, NULL, &tmv);

        
        if (status < 0){
            /* This should not happen */
            fprintf(stderr, "Select returned %d\n", status);
        }else{
            if (status == 0){
                /* Timer expired, Hence process it  */
                Timers_ExecuteNextTimer();
                /* Execute all timers that have expired.*/
                Timers_NextTimerTime(&tmv);
                while(tmv.tv_sec == 0 && tmv.tv_usec == 0){
                    /* Timer at the head of the queue has expired  */
                    Timers_ExecuteNextTimer();
                    Timers_NextTimerTime(&tmv);
                    
                }
            }
            if (status > 0){
                /* The socket has received data.
                 Perform packet processing. */
                if (FD_ISSET(sockfd, &rset)) {
                    bzero(client_buf, FILE_BUF_SIZE);
                    bytes_recieved = (int)recvfrom(sockfd, client_buf, FILE_BUF_SIZE, 0,
                                                   (struct sockaddr *) &client_addr, &client_addr_length);
                    
                    if (bytes_recieved <0 ) {
                        perror("Tracker:  Error in recvfrom\n");
                    } else {
                        //MSG TYPE 1:   Group update
                        //	MSG_TYPE  FromNode 	Start_Segment   End_Segment	
                        
                        
                        

                        
                    }
                    //TDEBUG_PRINT(("Tracker:   Got datagram of %d bytes from %s port %d\n", bytes_recieved, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port)));
                    
                    
                    
                    
                    
                    /*TDEBUG_PRINT(("msgtype is %d\n", group_request.msgtype));
                     TDEBUG_PRINT(("node_id is %d\n", group_request.node_id));
                     TDEBUG_PRINT(("numfiles is %d\n", group_request.numfiles));
                     TDEBUG_PRINT(("Filename is %s\n", group_request.filename));
                     TDEBUG_PRINT(("Type is %d\n", group_request.type));
                     sendto(sockfd,client_buf,sizeof(client_buf), MSG_EOR, (struct sockaddr *) &client_addr, client_addr_length);
                     */
                    
                    
                    
                }

                
            }
        }
    }
    return;
}


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
    int         client_port  = 0;
    
    long         read_size;
    char         client_helloline[80];
    char        client_message[5000];

    struct client tmp_client;
  //  char        buffer[BUFSIZE];
  //  int         bytes_recieved;

   // struct      sockaddr_in client_addr;
   // socklen_t   client_addr_length;


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
        client_port = ntohs(sin.sin_port);
       
        CDEBUG_PRINT(("Client  %d:  Client is listening on port number %d\n", client_id, client_port));
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
    tmp_client.node_port=client_port;
    //read_size = recv(client_sockfd , client_message , sizeof(client_message) , 0);
    //tracker_port = (int)strtol(client_message,NULL,10);
    tracker_port = tmp_client.tracker_port;
    CDEBUG_PRINT(("Client: %d Tracker port is %d\n", client_id, tracker_port));


    //Mandatory log Message
    sprintf(log_message, "tport %d\n", tracker_port);
    log_entry( log_file, log_message);
    sprintf(log_message, "myPort %d\n", client_port);
    log_entry(log_file, log_message);

    CDEBUG_PRINT(("Client:%02d has completed Phase 1 logging.\n", client_id));
    
    CDEBUG_PRINT(("Client %d going into Client_tracker_comms\n", tmp_client.node_id));
    client_tracker_comms(  tmp_client, log_file);

    int (*cpt)();   /* Define a pointer to the function */
    cpt = CProcessTimer;  /* Gets the address of ProcessTimer */
    if (tmp_client.has_task) {
        printf("Adding timer for %d\n", tmp_client.node_id);
        Timers_AddTimer(1000 ,cpt,(u_char *)&tmp_client);
        //client_listener(socket_desc, &tmp_client, log_file);
        //start();
   	 }
    

   // should put a close for our server udp, but we'll never get here
   // close(socket_desc);

}
