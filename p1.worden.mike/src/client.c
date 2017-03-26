

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


void client_tracker_comms(  struct client client_info, struct group_assign *fga, char *logfile, int timestamp) {
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
    
    
    if (client_info.num_tasks ==0 ) {
        CDEBUG_PRINT(("CLIENT %d HAS NO TASKS\n", client_info.node_id ));

        return;
    }
    if (client_info.task_start_time > timestamp) {
        CDEBUG_PRINT(("CLIENT %d Not time to start yet\n", client_info.node_id ));
        return;
    }
    CDEBUG_PRINT(("CLIENT %d sending a SHOWING INTEREST messsage\n", client_info.node_id ));

    
    
    
    grpmsg.node_id = client_info.node_id;
    grpmsg.msgtype  = GROUP_SHOW_INTEREST;
    grpmsg.numfiles = 1;
    memset(grpmsg.filename, '\0', sizeof(grpmsg.filename));
    strncpy(grpmsg.filename, client_info.taskname[0], sizeof(grpmsg.filename));
    if (client_info.task_share == 1) {
        grpmsg.type = GROUP_REQ_SHARE;
    } else {
        grpmsg.type = GROUP_REQ_NO_SHARE	;
    }
    
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
    
    struct timeval timeout;
    timeout.tv_sec = 1;  /* 5 Secs Timeout */
    timeout.tv_usec = 0;
    
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout,sizeof(struct timeval));
    
    
    response = (int)sendto(sockfd,(char *)client_buf, sizeof(grpmsg), 0, (struct sockaddr *)&tracker_servaddr, trackerlen);
    if (response <0 ) {
        perror("Client  :  Error connecting to tracker in recvfrom\n");
        sleep(3);
    } else {
        sprintf(log_message, "%d to T  GROUP_SHOW_INTEREST %s\n", timestamp, grpmsg.filename);
        log_entry(logfile, log_message);
        
        bzero(client_buf, sizeof(client_buf));
        client_addr_length = sizeof(client_addr);
        response = (int)recvfrom(sockfd, client_buf, sizeof(client_buf), 0,
                                 (struct sockaddr *) &client_addr, &client_addr_length);
        
        //CDEBUG_PRINT(("Client %d:   Got datagram of %d bytes from %s port %d\n", client_info.node_id, response, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port)));
        deserialize_group_assign(client_buf, &ga, client_info.node_id);
        /*CDEBUG_PRINT(("\n****************GROUP Assign***********************\n"));
         CDEBUG_PRINT(("Client %d:  message type %d\n", client_info.node_id, ga.msgtype));
         CDEBUG_PRINT(("Client %d:  num_files %d\n", client_info.node_id, ga.num_files));
         CDEBUG_PRINT(("Client %d:  file_name %s\n", client_info.node_id, ga.filename));
         CDEBUG_PRINT(("Client %d:  Num_Neighbors %d\n", client_info.node_id, ga.num_neighbors));
         
         for (int i = 0; i<ga.num_neighbors; i++) {
         CDEBUG_PRINT(("Client %d:  Neighbor[%d] %d\n", client_info.node_id, i, ga.neighbor_id[i]));
         CDEBUG_PRINT(("Client %d:  Neighbor[%d] IP %d.%d.%d.%d\n", client_info.node_id, i, ga.neighbor_ip[i][0], ga.neighbor_ip[i][1], ga.neighbor_ip[i][2], ga.neighbor_ip[i][3]));
         CDEBUG_PRINT(("Client %d:  Port[%d] %d\n", client_info.node_id, i, ga.neighbor_port[i]));
         }
         */
        
        CDEBUG_PRINT(("\n****************GROUP Assign***********************\n"));
        char nodelist[100];
        sprintf(nodelist, " ");
        for (int i = 0; i<ga.num_neighbors; i++) {
            if (ga.neighbor_id[i] != client_info.node_id)
                sprintf(nodelist + strlen(nodelist), "%d ", ga.neighbor_id[i]);
        }
        ;
        *fga=ga;
        sprintf(log_message, "%d From T  GROUP_ASSIGN %s\n", timestamp, nodelist);
        log_entry(logfile, log_message);
    }

    return;
    
    
}


/******************************************************************************
* client_client_comms:   download files
 *
******************************************************************************/

void client_client_comms (  struct client *client, char *logfile, int timestamp) {
    int sockfd;
    struct sockaddr_in otherclient_servaddr;
    socklen_t otherclientlen;
    int response;
    char log_message[80];
    struct file_record fr, downloaded_fr;
    int num_records_max = 625;
    static int num_records_file = 625;
    
    
    unsigned char client_buf[256];
    //struct group_show_interest grpmsg;
    struct group_assign         ga ={1,0,"",0,0, 0, 0};;
    static int filesize =0;
    int current_records_downloaded = 0;
    int max_downloads_session=800;
    unsigned char file_buff[FILE_BUF_RECORDS][FILE_BUF_SIZE];
    unsigned char *file_buff_ptr = *file_buff;
    
    
    static int num_records_downloaded = 0;

    static char file_to_download[256] = "";
    static char download_ip[16] ="";
    static int  download_port =0;
    
    
    if ((num_records_downloaded % 8) ==0 ) {
        //Show Interest
        client_tracker_comms(*client, &ga, logfile, timestamp);
    }

   
    CDEBUG_PRINT(("Client %d:   Has %d neighbors\n", client->node_id, ga.num_neighbors));
    if (ga.num_neighbors == 0) {
        CDEBUG_PRINT(("Client %d No neighbors yet!!\n", client->node_id	));
        // No neighbors yet
        return;
    } else {
        strncpy(file_to_download, ga.filename, sizeof(file_to_download));
        if (ga.num_neighbors == 1) {
            sprintf(download_ip, "%d.%d.%d.%d", ga.neighbor_ip[0][0],ga.neighbor_ip[0][1],ga.neighbor_ip[0][2],ga.neighbor_ip[0][3]);
            download_port = ga.neighbor_port[0];
        } else if (ga.num_neighbors >1 ) {
            /* Intializes random number generator */
            time_t t;
            srand((unsigned) time(&t));
            int rand_node = rand() % ga.num_neighbors;
            sprintf(download_ip, "%d.%d.%d.%d", ga.neighbor_ip[rand_node][0],ga.neighbor_ip[rand_node][1],ga.neighbor_ip[rand_node][2],ga.neighbor_ip[rand_node][3]);
            download_port = ga.neighbor_port[rand_node];
        }
        
    }

    
    
   
    
    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        perror("ERROR opening socket");
    // bzero seems to do the same thing as memset(buf, '/0',sizeof(buf))
    bzero(&otherclient_servaddr, sizeof otherclient_servaddr);
    
    
    // Build the client's address
    otherclient_servaddr.sin_family = AF_INET;
    otherclient_servaddr.sin_port=htons(download_port);
    

    inet_pton(AF_INET,download_ip, &(otherclient_servaddr.sin_addr));
    

    
    
    //Send our message to the otherclient
    
    
   
    while ((num_records_downloaded < num_records_max ) && (num_records_downloaded < num_records_file) && (current_records_downloaded < max_downloads_session)){
        fr.record_number = num_records_downloaded;
        fr.num_records = 1;
        strcpy(fr.filename, client->filename);
        memset(fr.buf, '\0', sizeof(fr.buf));
        memset(client_buf, '\0', sizeof(client_buf));
        serialize_record(client_buf, &fr);
        
        otherclientlen = sizeof(otherclient_servaddr);
         //CDEBUG_PRINT(("Client %d:  Connecting to %s port %d\n", client->node_id, download_ip, download_port));
        response = (int)sendto(sockfd,(char *)client_buf, sizeof(client_buf), 0, (struct sockaddr *)&otherclient_servaddr, otherclientlen);
        
        current_records_downloaded++;
        bzero(client_buf, sizeof(client_buf));
        //CDEBUG_PRINT(("Client %d:   Waiting to recieve\n", client->node_id));
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout,sizeof(struct timeval));
        response = (int)recv(sockfd, client_buf, sizeof(client_buf), 0);
        
        
        
        
        
        if (response <0 ) {
            perror("Client  :  Error in recvfrom\n");
            sleep(3);
        } else {
            //CDEBUG_PRINT(("Client %d:   Got a reply of %d bytes from %s port %d\n", client->node_id, response, inet_ntoa(otherclient_servaddr.sin_addr), ntohs(otherclient_servaddr.sin_port)));
            deserialize_record(client_buf, &downloaded_fr);
 /*           CDEBUG_PRINT(("Client %d:   Client Record %d\n", client->node_id, downloaded_fr.record_number));
            CDEBUG_PRINT(("Client %d:   File Size %d\n", client->node_id, downloaded_fr.filesize));
            CDEBUG_PRINT(("Client %d:   Total Number of Records:  %d\n", client->node_id, downloaded_fr.num_records));
            CDEBUG_PRINT(("Client %d:   Filename:  %s\n", client->node_id, downloaded_fr.filename));
            CDEBUG_PRINT(("BUF\n"));
            
            for (int i=0; i<sizeof(fr.buf); i++) {
                printf("%c", downloaded_fr.buf[i]);
            }
            CDEBUG_PRINT(("BUF\n"));*/
            num_records_file = downloaded_fr.num_records;
            filesize=downloaded_fr.filesize;
            //CDEBUG_PRINT(("Client %d:  FileSize %s is %d records\n", client->node_id, downloaded_fr.filename, downloaded_fr.num_records));
            bzero(file_buff[fr.record_number], sizeof(file_buff[fr.record_number]));
            memcpy(file_buff[fr.record_number], downloaded_fr.buf, sizeof(downloaded_fr.buf));
            num_records_downloaded++;
        }
    }
    CDEBUG_PRINT(("*****************************************************************************\n"));
    CDEBUG_PRINT(("About to write a file of size %d with %d records of %d that should be there\n", filesize, num_records_downloaded, num_records_file));
    CDEBUG_PRINT(("*****************************************************************************\n"));
    
    if (num_records_downloaded == (num_records_file)) {
        char filename[256];
        sprintf(filename, "%d-%s", client->node_id, client->taskname[0]);
        
        for (char* p = filename	; (p = strchr(p, '/')); ++p) {
            *p = '_';
        }
        CDEBUG_PRINT(("Writing file %s\n", filename));
        FILE* file = fopen( filename, "wb" );
        fwrite( file_buff_ptr, sizeof(file_buff_ptr[0]), downloaded_fr.filesize, file);
        fclose(file);
    }
    
    close(sockfd);
       return;
}




int CProcessTimer(int p)
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
 * client_listener():  Responds to calls from clients                        *
 ******************************************************************************/

void client_listener(int sockfd, struct client *client, char *logfile)
{
    struct timeval tmv;
    int status;
    fd_set rset;
    char   client_buf[256];
    
    
    char log_message[80],  filename[256], errmsg[256];
    
    int maxfdpl, stdineof;
    int bytes_recieved;
    
    struct sockaddr_in client_addr;
    socklen_t client_addr_length;
    int opt = 1;
    int client_socket[MAX_CLIENTS];
    static int elapsed_time =0;
    long file_size =0;
    int  num_records = 0;
    
    unsigned char file_buff[FILE_BUF_RECORDS][FILE_BUF_SIZE];
    unsigned char *file_buff_ptr = *file_buff;
    struct file_record fr;
    
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        client_socket[i] = 0;
    }
    
    //set master socket to allow multiple connections ,
    if( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    
    
    
    
    FILE *fp = fopen(logfile, "a");
    if (!fp) {
        perror("Error appending to  logfile!\n");
        
    }
    strncpy(filename, client->filename, sizeof(filename));
    for (char* p = filename	; (p = strchr(p, '/')); ++p) {
        *p = '_';
    }

    
    FILE *ofp = fopen(filename, "rb");
    if (!ofp) {
        sprintf(errmsg, "Client %d:  Error opening file %s\n", client->node_id, filename);
        perror (errmsg);
        
    } else {
        //file_size = (int)fread(file_buff, MAX_FILE_SIZE, 1, ofp);
        fseek(ofp, 0, SEEK_END);
        file_size = ftell(ofp);
        fseek(ofp, 0, SEEK_SET);  //same as rewind(f);
        fread(file_buff_ptr, file_size, 1, ofp);
        num_records = (int) file_size/FILE_BUF_SIZE;
        if ( (file_size % FILE_BUF_SIZE) != 0)
            num_records += 1;
        CDEBUG_PRINT(("Client %d:  File %s is %ld bytes and %d records \n", client->node_id, filename, file_size, num_records));
    }
    
    fclose(ofp);
    
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
                
                /* Timer expired, Hence process it  */
                Timers_ExecuteNextTimer();
                /* Execute all timers that have expired.*/
                Timers_NextTimerTime(&tmv);
                while(tmv.tv_sec == 0 && tmv.tv_usec == 0){
                    /* Timer at the head of the queue has expired  */
                    Timers_ExecuteNextTimer();
                    Timers_NextTimerTime(&tmv);
                    
                }
                
                
               
                CDEBUG_PRINT(("Client %d: Timer == %d\n",client->node_id, elapsed_time ));
                //CDEBUG_PRINT(("Client %d:  Executing Client-Client Comms\n", client->node_id));
                client_client_comms (  client, logfile,  elapsed_time);
                elapsed_time++;
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
                        perror("Client Listener:  Error in recvfrom\n");
                    } else {
                        CDEBUG_PRINT(("Client Listener %d:   Got datagram of %d bytes from %s port %d\n", client->node_id, bytes_recieved, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port)));
                        deserialize_record((unsigned char *)client_buf, &fr);
                        fr.num_records = num_records;
                        fr.filesize = (int)file_size;
                        
                        memcpy(fr.buf, file_buff[fr.record_number], sizeof(fr.buf));

                        bzero(client_buf, sizeof(client_buf));
                        serialize_record((unsigned char *)client_buf, &fr);
            
                        CDEBUG_PRINT(("Client listener %d:  Sending datagram of %lu bytes to %s port %d\n", client->node_id, sizeof(client_buf), inet_ntoa (client_addr.sin_addr), ntohs(client_addr.sin_port)));
                        sendto(sockfd,client_buf, sizeof(client_buf), 0, (struct sockaddr *) &client_addr, client_addr_length);

                    }
                    
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
    //CDEBUG_PRINT(("Client:  Launching Client: %d & logging to file %02d.out\n", client_id, client_id));


    sprintf(log_file, "%02d.out", client_id);
    create_log(log_file);

    log_entry(log_file, "type Client\n\n");

    sprintf(log_message, "myID %02d\n", client_id);
    log_entry(log_file, log_message);
    sprintf(log_message, "pid %d\n", getpid());
    log_entry(log_file, log_message);



    //CDEBUG_PRINT(("Client %d:  Launching our UDP Server\n", client_id));
    //Create socket for our UDP listener
    socket_desc = socket(AF_INET , SOCK_DGRAM , 0);
    if (socket_desc == -1) {
        perror("Client:  Could not create socket\n");
    } else {
        //CDEBUG_PRINT(("Client %d:  Socket created\n", client_id));
    }

  
    //Prepare the sockaddr_in structure for our UDP listener
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons( 0 );
    int optval = 1;

    //setsockopt: Handy debugging trick that lets us quickly rerun the server
    //Not sure if it's necessary, since we're dynamically allocating ports
    
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
    //CDEBUG_PRINT(("Client %d:  Binding our server\n", client_id));
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
       
        //CDEBUG_PRINT(("Client  %d:  Client is listening on port number %d\n", client_id, client_port));
    }


    //CDEBUG_PRINT(("Client %d:  Connecting to manager\n", client_id));
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
        //CDEBUG_PRINT(("Client %d:  Sending %s to manager at port:  %d\n", client_id, client_helloline, manager_port));
        setsockopt(client_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));
        connect(client_sockfd, (struct sockaddr *)&manager_servaddr, sizeof(manager_servaddr));
        
        write(client_sockfd,client_helloline, strlen(client_helloline));
        //CDEBUG_PRINT(("Client %d:  Waiting to Receive something from manager\n", client_id));
        read_size = recv(client_sockfd , client_message , sizeof(client_message) , 0);
        //CDEBUG_PRINT(("Client %d:  received %s:  %ld bytes\n", client_id, client_message, read_size));
        close(client_sockfd);
        
    } while (read_size <5000);
    
    
    init_client(&tmp_client);
    client_record_deserialization(client_message, &tmp_client);
    tmp_client.node_port=client_port;
    //read_size = recv(client_sockfd , client_message , sizeof(client_message) , 0);
    //tracker_port = (int)strtol(client_message,NULL,10);
    tracker_port = tmp_client.tracker_port;
    //CDEBUG_PRINT(("Client: %d Tracker port is %d\n", client_id, tracker_port));

    //CDEBUG_PRINT(("CLient:  %d -> has_task is %d \n", tmp_client.node_id, tmp_client.has_task ));
    //Mandatory log Message
    sprintf(log_message, "tport %d\n", tracker_port);
    log_entry( log_file, log_message);
    sprintf(log_message, "myPort %d\n", client_port);
    log_entry(log_file, log_message);

    //CDEBUG_PRINT(("Client:%02d has completed Phase 1 logging.\n", client_id));
    
    //CDEBUG_PRINT(("Client %d going into Client_tracker_comms\n", tmp_client.node_id));
    //client_tracker_comms(  tmp_client, log_file);

    
    if (tmp_client.has_task) {
        printf("Adding timer for %d\n", tmp_client.node_id);
        int (*cpt)();   /* Define a pointer to the function */
        cpt = CProcessTimer;  /* Gets the address of ProcessTimer */
        
        Timers_AddTimer(3000 ,cpt,(int*)tmp_client.node_id);
        client_listener(socket_desc, &tmp_client, log_file);
        
    } else {
        close(socket_desc);
    }
    CDEBUG_PRINT(("Client %d:   Launch client complete\n", tmp_client.node_id));

   // should put a close for our server udp, but we'll never get here
   // close(socket_desc);

}
