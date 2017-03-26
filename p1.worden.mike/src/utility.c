
#include "../include/manager.h"

unsigned char * serialize_int(unsigned char *buffer, int value)
{
    /* Write big-endian int value into buffer; assumes 32-bit int and 8-bit char. */
    buffer[0] = value >> 8;
    buffer[1] = value;
    return buffer + 2;
}

unsigned char * deserialize_int(unsigned char *buffer, int *value) {
    
    *value = buffer[0]<<8;
    *value = *value + buffer[1];
    return buffer + 2;
    
}
unsigned char * serialize_char(unsigned char *buffer, char value)
{
    buffer[0] = value;
    return buffer + 1;
}

unsigned char * deserialize_char(unsigned char *buffer, char *value) {
    *value = buffer[0];
    return buffer+1;
    
}

unsigned char * serialize_unsigned_char(unsigned char *buffer, unsigned char value)
{
    buffer[0] = value;
    return buffer + 1;
}

unsigned char * deserialize_unsigned_char(unsigned char *buffer, unsigned char *value) {
    *value = buffer[0];
    return buffer+1;
    
}

unsigned char * serialize_group_req(unsigned char *buffer, struct group_show_interest *gsi)
{
    buffer=serialize_int(buffer, gsi->msgtype);
    buffer=serialize_int(buffer, gsi->node_id);
    buffer=serialize_int(buffer, gsi->numfiles);
    for (int i=0; i<sizeof(gsi->filename); i++)
        buffer=serialize_char(buffer, gsi->filename[i]);
    buffer=serialize_int(buffer, gsi->type);
    buffer=serialize_int(buffer, gsi->client_port);
    return buffer;
}
int deserialize_group_req(unsigned char *buffer, struct group_show_interest *gsi) {
    buffer=deserialize_int(buffer, &gsi->msgtype);
    buffer=deserialize_int(buffer, &gsi->node_id);
    buffer=deserialize_int(buffer, &gsi->numfiles);
    for (int i=0; i<sizeof(gsi->filename); i++)
        buffer=deserialize_char(buffer, &gsi->filename[i]);
    buffer=deserialize_int(buffer, &gsi->type);
    buffer=deserialize_int(buffer, &gsi->client_port);
    return 0;
    
}

void serialize_group_assign (unsigned char *buffer, struct group_assign *ga) {

    buffer= serialize_int(buffer, ga->msgtype);
    buffer=serialize_int(buffer, ga->num_files);
    for (int i=0;i<sizeof(ga->filename); i++)
        buffer=serialize_char(buffer, ga->filename[i]);
    buffer=serialize_int(buffer, ga->num_neighbors);
    for (int i = 0; i<MAX_CLIENTS; i++) {
        buffer=serialize_int(buffer, ga->neighbor_id[i]);
        for (int j=0; j<sizeof(ga->neighbor_ip[i]); j++)
            buffer = serialize_char(buffer, (char)ga->neighbor_ip[i][j]);
        buffer=serialize_int(buffer, ga->neighbor_port[i]);
    }
    
    
}


void deserialize_group_assign(unsigned char *buffer, struct group_assign *ga, int node_id) {
    int new_node_id;
    unsigned char new_neighbor_ip[4];
    int new_node_port;
    int num_neighbors = 0;
    int index =0;
    
    buffer = deserialize_int(buffer, &ga->msgtype);
    buffer = deserialize_int(buffer, &ga->num_files);
    for (int i=0;i<sizeof(ga->filename); i++)
        buffer=deserialize_char(buffer, &ga->filename[i]);
    buffer=deserialize_int(buffer, &num_neighbors);
    
    for (int i = 0; i<num_neighbors; i++) {
        buffer=deserialize_int(buffer, &new_node_id);
        for (int j=0; j<sizeof(new_neighbor_ip); j++)
            buffer = deserialize_char(buffer, (char *)&new_neighbor_ip[j]);
        buffer=deserialize_int(buffer, &new_node_port);
        if (new_node_id != node_id) {
            ga->neighbor_id[index] = new_node_id;
            for (int j=0; j<sizeof(ga->neighbor_ip[index]); j++)
                strcpy((char *)&ga->neighbor_ip[index][j], (char *) &new_neighbor_ip[j]);
            ga->neighbor_port[i] = new_node_port;
            index++;
        }
    }
    ga->num_neighbors = index;
}

void serialize_record(unsigned char *buffer, struct file_record *fr) {
    buffer=serialize_int(buffer, fr->record_number);
    buffer=serialize_int(buffer, fr->num_records);
    buffer=serialize_int(buffer, fr->filesize);
    
  
    for (int i=0; i<sizeof(fr->buf); i++) {
        //printf("%c", fr->buf[i]);
        buffer=serialize_unsigned_char(buffer, fr->buf[i]);
    }
    for (int i=0; i<sizeof(fr->filename); i++)
        buffer=serialize_char(buffer, fr->filename[i]);
    
}

void deserialize_record(unsigned char *buffer, struct file_record *fr) {
    buffer=deserialize_int(buffer, &fr->record_number);
    buffer=deserialize_int(buffer, &fr->num_records);
    buffer=deserialize_int(buffer, &fr->filesize);

    bzero(fr->buf, sizeof(fr->buf));
    for (int i=0; i<sizeof(fr->buf); i++) {
        buffer=deserialize_unsigned_char(buffer,&fr->buf[i]);
       // printf("%c", fr->buf[i]);
    }
    bzero(fr->filename, sizeof(fr->filename));
    for (int i=0; i<sizeof(fr->filename); i++)
        buffer=deserialize_char(buffer, &fr->filename[i]);
    
}

/******************************************************************************
* create_log:   Initialize our logfile
******************************************************************************/
void create_log(char *file_name) {


    FILE *fp = fopen(file_name, "w");
    if (!fp) {
        perror("Error creating logfile!\n");

    }
    fclose(fp);

}

/******************************************************************************
* log_entry:   Append to our logfiles
******************************************************************************/

void log_entry(char *file_name, char *message ) {

    FILE *fp = fopen(file_name, "a");
    if (!fp) {
        perror("Error appending to  logfile!\n");

    }

    fputs(message, fp);

    fclose(fp);
}


/******************************************************************************
 * log_entry:   Append to our logfiles
 ******************************************************************************/

void log_hex_entry(char *file_name, char hex_message[], int size ) {
    
    
  
    FILE *fp = fopen(file_name, "ab");
    if (!fp) {
        perror("Error appending to  logfile!\n");
        
    }
    
    for (int i=0; i<size; i++) {
        //log_ptr += sprintf(log_message, "%02x ",  hex_message[i]);
        fprintf(fp, "%02x ", hex_message[i]);
    }
    
    fprintf(fp, "\n");
    
    fclose(fp);
}

/******************************************************************************
* debug_print_client:   Utility function to print out client
*
******************************************************************************/
void debug_print_client(struct client *client) {
    DEBUG_PRINT(("\nNode ID:  %d\n", client->node_id));
    DEBUG_PRINT(("Enabled:  %d\n", client->enabled));
    DEBUG_PRINT(("Configured:  %d\n", client->configured));
    DEBUG_PRINT(("Filename:  %s\n", client->filename));
    DEBUG_PRINT(("Has_task:  %d\n", client->has_task));
    DEBUG_PRINT(("Node_id:  %d\n", client->node_id));
    DEBUG_PRINT(("Packet_Delay:  %d\n", client->packet_delay));
    DEBUG_PRINT(("Packet_Drop_Percentage:  %d\n", client->packet_drop_percentage));
    DEBUG_PRINT(("Task_Share:  %d\n", client->task_share));
    DEBUG_PRINT(("Task_Start_Time:  %d\n", client->task_start_time));
    DEBUG_PRINT(("Tracker Port:  %d\n", client->tracker_port));
    DEBUG_PRINT(("Node Port:  %d\n", client->node_port));
}

/******************************************************************************
* copy_client:   "Deep Copy" utility to reliably copy member objects of our
*                client struct
******************************************************************************/
void copy_client(struct client *from_client, struct client *to_client) {


    to_client->enabled = from_client->enabled;
    to_client->configured = from_client->configured;
    
    strncpy(to_client->filename, from_client->filename, sizeof(to_client->filename));
    to_client->has_task = from_client->has_task;
    to_client->node_id =  from_client->node_id;
    to_client->packet_delay = from_client->packet_delay;
    to_client->packet_drop_percentage = from_client->packet_drop_percentage;
    to_client->task_share = from_client->task_share;
    to_client->task_start_time = from_client->task_start_time;
    to_client->tracker_port = from_client->tracker_port;
    to_client->node_port = from_client->node_port;

}

/******************************************************************************
 * check_agent_status:   Dump our struct into a char buf for transport
 ******************************************************************************/
bool terminate_manager_check (struct client_args *clients) {
    int num_clients_configured=0;
    int num_clients_enabled=0;
    
    for (int i =0; i<MAX_CLIENTS; i++) {
        if (clients->client_list[i].enabled == true)
            num_clients_enabled++;
        if (clients->client_list[i].configured == true){
            num_clients_configured++;
        }
        
        
    }
        if (num_clients_configured == (num_clients_enabled)) {
        return true;
    } else {
        return false;
    }
}

/******************************************************************************
* client_record_serialization:   Dump our struct into a char buf for transport
******************************************************************************/

void client_record_serialization(char struct_data[5000], struct client *client_record){

// Hate sprintf!
    
    sprintf (struct_data, "%d|%d|%d|%d|%d|%d|%d|%d|%d|%s|%d|%s|%s", client_record->enabled, client_record->has_task,
    client_record->node_id,client_record->packet_delay, client_record->packet_drop_percentage,client_record->task_share,
    client_record->task_start_time, client_record->tracker_port, client_record->node_port, client_record->filename, client_record->num_tasks, client_record->taskname[0], client_record->taskname[1]);
    //printf("%s\n",struct_data);

}

/******************************************************************************
* client_record_deserialization:   Take a char buf and create a struct
*                                  (needs error handling)
******************************************************************************/
void client_record_deserialization(char struct_data[5000], struct client *client_record){

    char *field[13];
    int index = 0;

    char *token;

    token = strtok (struct_data," ,.|");

    while (token != NULL)
    {
        //printf("%s ", token);
        field[index] = token;
        index++;
        token = strtok (NULL, " |");
    }
    //printf("\n");
    client_record->enabled=strtol(field[0],NULL,10);
    client_record->has_task=strtol(field[1],NULL,10);
    client_record->node_id=(int)strtol(field[2],NULL,10);
    client_record->packet_delay=(int)strtol(field[3],NULL,10);
    client_record->packet_drop_percentage = (int)strtol(field[4],NULL,10);
    client_record->task_share=(int)strtol(field[5],NULL,10);
    client_record->task_start_time=(int)strtol(field[6],NULL,10);
    client_record->tracker_port = (int)strtol(field[7], NULL, 10);
    client_record->node_port = (int)strtol(field[8], NULL, 10);
    strncpy(client_record->filename, field[9], sizeof(client_record->filename));
    client_record->num_tasks = (int)strtol(field[10], NULL, 10);
    strncpy(client_record->taskname[0], field[11], sizeof(client_record->taskname[0]));
    strncpy(client_record->taskname[1], field[12], sizeof(client_record->taskname[1]));

}


