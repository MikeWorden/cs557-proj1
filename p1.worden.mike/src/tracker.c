

#include "../include/manager.h"
#include <pthread.h>
#include "timers-c.h"
#include <ctype.h>


/******************************************************************************
* connect_to_manager:  Connects to manager to pull the configs
*
******************************************************************************/


void connect_to_manager(int manager_port, int tracker_port, struct client clients[], int num_clients) {


    struct client client_list[MAX_CLIENTS];
    struct client tmp_client;
    int client_sockfd;
    struct sockaddr_in manager_servaddr;
    int read_size;
    char client_message[5000];
    char client_helloline[80];
    int index = 0;
    
    struct timeval timeout;
    timeout.tv_sec = 2;  /* 5 Secs Timeout */
    timeout.tv_usec = 0;



    client_sockfd = socket(AF_INET, SOCK_STREAM,0);
    bzero(&manager_servaddr, sizeof manager_servaddr);
    manager_servaddr.sin_family = AF_INET;
    manager_servaddr.sin_port=htons(manager_port);

    inet_pton(AF_INET,"127.0.0.1", &(manager_servaddr.sin_addr));

    connect(client_sockfd, (struct sockaddr *)&manager_servaddr, sizeof(manager_servaddr));

    
    sprintf(client_helloline, "%d: Hello from Tracker\n", tracker_port );



    write(client_sockfd,client_helloline, strlen(client_helloline));
    setsockopt(client_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout,sizeof(struct timeval));
    while(((index < 25 ) && (read_size = (int)recv(client_sockfd , client_message , sizeof(client_message) , 0)) > 2 ) ) {
        init_client(&tmp_client);
        client_record_deserialization(client_message, &tmp_client);
        copy_client(&tmp_client, &clients[index]);
        index++;
    }
    TDEBUG_PRINT(("TRACKER:  Got all clients\n\n\n"));
    for (int i =0; i<MAX_CLIENTS; i++) {

    }


    close(client_sockfd);
}

/******************************************************************************
 * ProcessTimer: Provided by Instructor
 * This function can be use to process the timer.
 * The return value from expire is used
 * to indicate what should be done with the timer
 *  = 0   Add timer again to queue
 *  > 0   Set new_timeout as timeout value for timer
 *  < 0   Discard timer
 ******************************************************************************/
int TrackerProcessTimer(int p)
{
    
    static bool basetime_set = false;
    static struct timeval base_tv;
    
    if (!basetime_set) {
        getTime(&base_tv);
        basetime_set = true;
    }
    struct timeval tv;
    
    getTime(&tv);
    //fprintf(stderr, "Timer %d has expired! Time = %d\n",
    //        p, (int)(tv.tv_sec - base_tv.tv_sec)+1);
    fflush(NULL);
    return 0;
}

/******************************************************************************
 * init_group_assign():  Responds to calls from clients                        *
 ******************************************************************************/

void init_group_assign(struct group_assign ga[]) {
    
    for (int i=0; i<MAX_FILES; i++) {
        memset(ga[i].filename,'\0', sizeof(ga[i].filename));
        ga[i].msgtype=2;
        for (int j=0; j<MAX_CLIENTS; j++) {
            ga[i].neighbor_id[j] =-1;
            for (int k=0; k<4; k++)
                ga[i].neighbor_ip[j][k] = 0;
            ga[i].neighbor_port[j]=0;
        }
        ga[i].num_files=0;
        ga[i].num_neighbors=0;
    }
}
/******************************************************************************
 * update group assign():  For message types 2 and 3                      *
 ******************************************************************************/
void update_group_assign(struct group_assign ga[], struct group_show_interest gr) {
    bool updated =false;
    // if it's a group update & we don't want to share, no update is needed
    if (gr.type == GROUP_REQ_NO_SHARE) {
        return;
    }
    
    for  (int i=0; i<MAX_FILES; i++) {
        if (strcmp(ga[i].filename, gr.filename) ==0) {
            for (int j=0; j<ga[i].num_neighbors; j++) {
                if(ga[i].neighbor_id[j] == gr.node_id)
                    updated=true;
            }
            if (!updated) {
                ga[i].neighbor_id[ga[i].num_neighbors] = gr.node_id;
                ga[i].neighbor_port[ga[i].num_neighbors] = gr.client_port;
                for (int j =0; j<4; j++)
                    ga[i].neighbor_ip[ga[i].num_neighbors][j] = (int)gr.ip_address[j];
                ga[i].num_neighbors = ga[i].num_neighbors +1;
                updated=true;
            }
        } 
    }
    for (int i=0; i<MAX_FILES; i++) {
        if ((ga[i].num_files == 0) && !(updated)) {
            ga[i].num_files =1;
            strncpy(ga[i].filename, gr.filename, sizeof(ga[i].filename));
            ga[i].neighbor_id[0] = gr.node_id;
            ga[i].neighbor_port[0] = gr.client_port;
            for (int j =0; j<4; j++)
                ga[i].neighbor_ip[ga[i].num_neighbors][j] = (int)gr.ip_address[j];
            ga[i].num_neighbors=1;
            updated=true;
        }
    }
}
/******************************************************************************
 * find group assign:  find the right group assign record                        *
 ******************************************************************************/
void find_group_assign(struct group_assign ga[], struct group_show_interest gr, int* index) {
    *index=0;
    
    for (int i=0; i<MAX_FILES; i++) {
        if (strcmp(ga[i].filename, gr.filename) ==0) {
            *index = i;
        }
    }

}

/******************************************************************************
 * find group assign:  find the right group assign record                     *
 *                     example pulled from
 * http://stackoverflow.com/questions/9211601/parsing-ip-adress-string-in-4-single-bytes
 ******************************************************************************/
void ipstrtobyte(char *ip_addr, unsigned char value[]) {
    
    char *str2;
    for (int i=0; i<4; i++)
        value[i] = 0;
    
    size_t index = 0;
    
    str2 = ip_addr; /* save the pointer */
    while (*ip_addr) {
        if (isdigit((unsigned char)*ip_addr)) {
            value[index] *= 10;
            value[index] += *ip_addr - '0';
        } else {
            index++;
        }
        ip_addr++;
    }
   
}

/******************************************************************************
 * tracker_listener():  Responds to calls from clients                        *
 ******************************************************************************/

void tracker_listener(int sockfd, struct client clients[], char *logfile)
{
    struct timeval tmv;
    int status;
    fd_set rset;
    char   client_buf[256];
    
    char log_message[80];
    
    int maxfdpl, stdineof;
    int bytes_recieved;
    
    struct sockaddr_in client_addr;
    socklen_t client_addr_length;
    struct group_show_interest group_request;
    struct group_assign group_assign_list[MAX_FILES];
    
    init_group_assign(group_assign_list);
    int index=0;
    

    
    FILE *fp = fopen(logfile, "a");
    if (!fp) {
        perror("Error appending to  logfile!\n");
        
    }
    printf("Tracker is listening\n");
    
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
        //FD_SET(fileno(fp), &rset);
        FD_SET(sockfd, &rset);
        maxfdpl = max(sockfd, fileno(fp))+1;
        status = select(maxfdpl, &rset, NULL, NULL, &tmv);
        
        if (status < 0){
            /* This should not happen */
            fprintf(stderr, "Select returned %d\n", status);
        }else{
            if (status == 0){
                //fprintf(stderr, " Timer expired\n");
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
                    bzero(client_buf, sizeof(client_buf));
                    client_addr_length = sizeof(client_addr);
                    bytes_recieved = (int)recvfrom(sockfd, client_buf, sizeof(client_buf), 0,
                                                   (struct sockaddr *) &client_addr, &client_addr_length);
                    
                    if (bytes_recieved <0 ) {
                        perror("Tracker:  Error in recvfrom\n");
                    }
                    TDEBUG_PRINT(("Tracker:   Got datagram of %d bytes from %s port %d\n", bytes_recieved, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port)));
                    
                    
                    deserialize_group_req((unsigned char *)client_buf, &group_request);
                    ipstrtobyte(inet_ntoa(client_addr.sin_addr), group_request.ip_address);
                    clients[group_request.node_id].node_port=group_request.client_port;
                    sprintf(log_message, "%d    %d    %s\n", group_request.node_id, 1, group_request.filename);
                    log_entry(logfile, log_message);
                    
                    
                    
                    TDEBUG_PRINT(("\n****************GROUP SHOW INTEREST***********************\n"));
                    TDEBUG_PRINT(("Tracker:  msgtype is %d\n", group_request.msgtype));
                    TDEBUG_PRINT(("Tracker:  node_id is %d\n", group_request.node_id));
                    TDEBUG_PRINT(("Tracker:  numfiles is %d\n", group_request.numfiles));
                    TDEBUG_PRINT(("Tracker:  Filename is %s\n", group_request.filename));
                    TDEBUG_PRINT(("Tracker:  Type is %d\n", group_request.type));
                    TDEBUG_PRINT(("Node Port is %d\n", group_request.client_port));
                    TDEBUG_PRINT(("Tracker:  IP address is %d.%d.%d.%d\n",group_request.ip_address[0], group_request.ip_address[1], group_request.ip_address[2], group_request.ip_address[3]));
                    
                    
                    update_group_assign(group_assign_list, group_request);
                    find_group_assign(group_assign_list, group_request, &index);
                    bzero(client_buf, sizeof(client_buf));
                    serialize_group_assign ((unsigned char *)client_buf, &group_assign_list[index]);
                                        
                    
                    log_hex_entry(logfile, client_buf, sizeof(client_buf));
                    sendto(sockfd,client_buf,sizeof(client_buf), MSG_EOR, (struct sockaddr *) &client_addr, client_addr_length);
                    
                    
                    
                    
                    
                    
                    
                 
                }
                
            }
        }
    }
    return;
}
         
         
         

/******************************************************************************
* launch_tracker:  Initialize the tracker process
*
******************************************************************************/

void launch_tracker( int manager_port) {


    int port = 0;
    struct client tracker_clients[MAX_CLIENTS];

    void *tracker_connection_handler(void *);


    TDEBUG_PRINT(("Tracker:  Launching Tracker & logging to tracker.out\n"));


    int socket_desc  ;
    struct sockaddr_in server_addr ;

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



    
    
    int (*tpt)();   /* Define a pointer to the function */
    tpt = TrackerProcessTimer;  /* Gets the address of ProcessTimer */

    Timers_AddTimer(3000 ,tpt,(int*)99);
    //TDEBUG_PRINT(("Done connecting to manager and launching tracker listener\n"));
    tracker_listener(socket_desc, tracker_clients, log_file);


    TDEBUG_PRINT(("Tracker:  Launch_Tracker() complete\n"));
    return ;

}



