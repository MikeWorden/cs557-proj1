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

#include <errno.h>
#include <limits.h>

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

#define TRACKER_DEBUG 1
#ifdef TRACKER_DEBUG
# define TDEBUG_PRINT(x) printf x
#else
# define TDEBUG_PRINT(x) do {} while (0)
#endif



#define CLIENT_DEBUG 1
#ifdef CLIENT_DEBUG
# define CDEBUG_PRINT(x) printf x
#else
# define CDEBUG_PRINT(x) do {} while (0)
#endif


#define max(a,b) \
({ __typeof__ (a) _a = (a); \
__typeof__ (b) _b = (b); \
_a > _b ? _a : _b; })


#define BUFSIZE 1024

#define GROUP_SHOW_INTEREST 1
#define GROUP_ASSIGN  2

#define SA_TYPE_INTEREST_NO_SHARE   1
#define SA_TYPE_INTEREST_SHARE      2
#define SA_TYPE_NO_INTEREST_SHARE   3

#define GROUP_REQ_NO_SHARE 1
#define GROUP_REQ_SHARE    2
#define GROUP_INTEREST_SHARE 3

#define TIMESCALE 10000

#define FILE_BUF_SIZE 32
#define FILE_BUF_RECORDS 625
#define MAX_FILE_SIZE 20000
#define MAX_FILES 1
#define MAX_TASKS 2

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
    
        int  node_port;
        int  num_tasks;
        char taskname[MAX_TASKS][256];
};

// Struct to pass data to the connection manager on the manager process
struct client_args {
        struct client *client_list;
        int           client_sock;
        int           tracker_port;
        int           manager_port;
};


struct file_record {
    int record_number;
    int num_records;
    int filesize;
    char filename[256];
    unsigned char buf[32];
};

struct group_show_interest {
    
    int msgtype;
    int node_id;
    int numfiles;
    int type;
    char filename[32];
    int client_port;
    unsigned char ip_address[4];
    
};

struct group_assign {
    int msgtype;
    int num_files;
    char filename[32];
    int num_neighbors;
    int neighbor_id[MAX_CLIENTS];
    unsigned char neighbor_ip[MAX_CLIENTS][4];
    int neighbor_port[MAX_CLIENTS];
   
    
    
    
};


void init( struct client *clients, int *num_clients, int *timeout);
void init_client(struct client *client);
void copy_client(struct client *from_client, struct client *to_client);
void debug_print_client(struct client *client);

void launch_manager(struct client *clients, int *num_clients, int *timeout, int  *management_port);

void client_record_serialization(char struct_data[5000], struct client *client_record);
void client_record_deserialization(char struct_data[5000], struct client *client_record);

unsigned char * serialize_group_req(unsigned char *buffer, struct group_show_interest *gsi);
int deserialize_group_req(unsigned char *buffer, struct group_show_interest *gsi);

void serialize_group_assign (unsigned char *buffer, struct group_assign *ga);
void deserialize_group_assign(unsigned char *buffer, struct group_assign *ga, int node_id);
unsigned char * serialize_int(unsigned char *buffer, int value);
unsigned char * serialize_char(unsigned char *buffer, char value);
void serialize_record(unsigned char *buffer, struct file_record *fr);
void deserialize_record(unsigned char *buffer, struct file_record *fr);

bool terminate_manager_check (struct client_args *clients);

void launch_tracker( int manager_port);

void launch_client( int manager_port, int client_id);

void create_log(char *file_name);
void log_entry(char *file_name, char *message );
void log_hex_entry(char *file_name, char hex_message[], int size );

int ProcessTimer(int p);


#endif
