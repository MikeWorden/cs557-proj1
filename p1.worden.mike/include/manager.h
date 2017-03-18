#ifndef MANAGER_H
#define MANAGER_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>



#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include <pthread.h>


#define FILE_NAME "./manager.conf"
#define MAX_CLIENTS 25

//#define DEBUG 1
#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

//#define MANAGER_DEBUG 1
#ifdef MANAGER_DEBUG
# define MDEBUG_PRINT(x) printf x
#else
# define MDEBUG_PRINT(x) do {} while (0)
#endif

//#define TRACKER_DEBUG 1
#ifdef TRACKER_DEBUG
# define TDEBUG_PRINT(x) printf x
#else
# define TDEBUG_PRINT(x) do {} while (0)
#endif



//#define CLIENT_DEBUG 1
#ifdef CLIENT_DEBUG
# define CDEBUG_PRINT(x) printf x
#else
# define CDEBUG_PRINT(x) do {} while (0)
#endif





#define BUFSIZE 1024


// Basic struct to manage each client
struct client {
		int  node_id;
		bool enabled;
        bool configured;
		int  packet_delay;
		int  packet_drop_percentage;
		char filename[256];
		bool has_task;
		int  task_start_time;
		int  task_share;
        int  tracker_port;
};

// Struct to pass data to the connection manager on the manager process
struct client_args {
        struct client *client_list;
        int           client_sock;
        int           tracker_port;
        int           manager_port;
};


void init( struct client *clients, int *num_clients, int *timeout);
void init_client(struct client *client);
void copy_client(struct client *from_client, struct client *to_client);
void debug_print_client(struct client *client);

void launch_manager(struct client *clients, int *num_clients, int *timeout, int  *management_port);

void client_record_serialization(char struct_data[5000], struct client *client_record);
void client_record_deserialization(char struct_data[5000], struct client *client_record);

bool terminate_manager_check (struct client_args *clients);

void launch_tracker( int manager_port);

void launch_client( int manager_port, int client_id);

void create_log(char *file_name);
void log_entry(char *file_name, char *message );



#endif
