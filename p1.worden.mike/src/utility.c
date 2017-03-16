
#include "../include/manager.h"


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
* debug_print_client:   Utility function to print out client
*
******************************************************************************/
void debug_print_client(struct client *client) {
    DEBUG_PRINT(("Enabled:  %d\n", client->enabled));
    DEBUG_PRINT(("Configured:  %d\n", client->configured));
    DEBUG_PRINT(("Filename:  %s\n", client->filename));
    DEBUG_PRINT(("Has_task:  %d\n", client->has_task));
    DEBUG_PRINT(("Node_id:  %d\n", client->node_id));
    DEBUG_PRINT(("Packet_Delay:  %d\n", client->packet_delay));
    DEBUG_PRINT(("Packet_Drop_Percentage:  %d\n", client->packet_drop_percentage));
    DEBUG_PRINT(("Task_Share:  %d\n", client->task_share));
    DEBUG_PRINT(("Task_Start_Time:  %d\n", client->task_start_time));
}

/******************************************************************************
* copy_client:   "Deep Copy" utility to reliably copy member objects of our
*                client struct
******************************************************************************/
void copy_client(struct client *from_client, struct client *to_client) {


    to_client->enabled = from_client->enabled;
    to_client->configured = from_client->configured;
    to_client->filename = malloc(strlen(from_client->filename)+1);
    strcpy(to_client->filename, from_client->filename);
    to_client->has_task = from_client->has_task;
    to_client->node_id =  from_client->node_id;
    to_client->packet_delay = from_client->packet_delay;
    to_client->packet_drop_percentage = from_client->packet_drop_percentage;
    to_client->task_share = from_client->task_share;
    to_client->task_start_time = from_client->task_start_time;

}

/******************************************************************************
 * check_agent_status:   Dump our struct into a char buf for transport
 ******************************************************************************/
bool check_agent_status (struct client_args *clients) {
    return false;
}

/******************************************************************************
* client_record_serialization:   Dump our struct into a char buf for transport
******************************************************************************/

void client_record_serialization(char struct_data[5000], struct client *client_record){

// Hate sprintf!
sprintf (struct_data, "%d|%d|%d|%d|%d|%d|%d|%s", client_record->enabled, client_record->has_task,
    client_record->node_id,client_record->packet_delay, client_record->packet_drop_percentage,client_record->task_share,
    client_record->task_start_time, client_record->filename);

}

/******************************************************************************
* client_record_deserialization:   Take a char buf and create a struct
*                                  (needs error handling)
******************************************************************************/
void client_record_deserialization(char struct_data[5000], struct client *client_record){

    char *field[8];
    int index = 0;

    char *token;

    token = strtok (struct_data," ,.|");

    while (token != NULL)
    {
        //printf ("%s\n",token);
        field[index] = token;
        index++;
        token = strtok (NULL, " ,.|");
    }

    client_record->enabled=strtol(field[0],NULL,10);
    client_record->has_task=strtol(field[1],NULL,10);
    client_record->node_id=(int)strtol(field[2],NULL,10);
    client_record->packet_delay=(int)strtol(field[3],NULL,10);
    client_record->packet_drop_percentage = (int)strtol(field[4],NULL,10);
    client_record->task_share=(int)strtol(field[5],NULL,10);
    client_record->task_start_time=(int)strtol(field[6],NULL,10);
    client_record->filename = malloc(strlen(field[7]) +1 );
    client_record->filename = field[7];


}
