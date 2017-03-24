#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/manager.h"




/******************************************************************************
* init_client:   Default values for our client struct
*
******************************************************************************/
void init_client(struct client *client) {

    client->enabled = false;
    client->configured = false;
    strncpy(client->filename, "----", sizeof(client->filename));
    client->has_task = false;
    client->node_id = -1;
    client->packet_delay = -1;
    client->packet_drop_percentage = 100;
    client->task_share = 0;
    client->task_start_time = 1;

}

/******************************************************************************
* init:   Reads in our files and generates the structs for our clients
*
******************************************************************************/

void init( struct client *clients, int *num_clients, int *timeout)
{
    FILE *in_file;
	char line[120];
	int client_id = -1;
	int packet_delay = -1;
	int packet_drop_percentage = 100;
	int scan_count =0;



    

	*num_clients = 0;
	*timeout = 0;


	char param1[120], param2[120], param3[120], param4[120];

    for (int i = 0; i < MAX_CLIENTS; i++ ) {
        clients[i].enabled = false;
        clients[i].configured = false;
        strncpy(clients[i].filename, "------", sizeof(clients[i].filename));
        clients[i].has_task = false;
        clients[i].node_id = i;
        clients[i].packet_delay = -1;
        clients[i].packet_drop_percentage = 100;
        clients[i].task_share = 0;
        clients[i].task_start_time = -1;
    }

	in_file = fopen(FILE_NAME, "r");
	if (in_file == NULL) {
		fprintf(stderr, "Cannot open a %s\n", FILE_NAME);
		exit (EXIT_FAILURE);
	}

	//Parse File
	while (fgets(line, 120, in_file))  {
		if ((line[0] != '\n') &&  (line[0] !='#')) {
			//printf("%s", line);
			scan_count = sscanf(line, "%s %s %s %s", param1, param2, param3, param4);
			switch(scan_count) {
				case 0:
					MDEBUG_PRINT(("Init: Didn't find any parameter\n"));
					break;
				case 1:
					MDEBUG_PRINT(("Init:  Setting number of clients or timeout to  %s\n", param1));
					if (*num_clients == 0) {
						*num_clients = atoi(param1);
						MDEBUG_PRINT(("Init:  Set num_clients to %d\n", *num_clients));
						for (int i =0; i < *num_clients; i++) {
                            clients[i].enabled = true;
                            clients[i].configured = false;
                        }
					} else {
                        MDEBUG_PRINT(("timeout set to %s", param1));
						*timeout = atoi(param1);
						MDEBUG_PRINT(("Init:  Set timeout to %d\n", *timeout));
					}
					break;
				case 2:
					client_id = atoi(param1);
					MDEBUG_PRINT(("Init:  Setting filename to %s for client %d\n", param2, client_id));
					if ((client_id >=0) && (client_id <MAX_CLIENTS)) {
                            strncpy(clients[client_id].filename, param2, sizeof(clients[client_id].filename));
                    }
           
                    
					break;
				case 3:
					MDEBUG_PRINT(("Init:  Setting Packet Delay and Packet drop\n"));
					client_id = atoi(param1);
					packet_delay = atoi(param2);
					packet_drop_percentage = atoi(param3);
					if ((client_id >=0) && (client_id <MAX_CLIENTS)) {
                            clients[client_id].packet_delay = packet_delay;
                            clients[client_id].packet_drop_percentage = packet_drop_percentage;
                    }

					break;
                case 4:
					MDEBUG_PRINT(("Init:  Setting download tasks\n"));
					client_id = atoi(param1);
					if ((client_id >=0) && (client_id <MAX_CLIENTS)) {
                            //for (char* p = param2	; (p = strchr(p, '/')); ++p) {
                            //    *p = '_';
                            //}
                            strncpy(clients[client_id].filename, param2, sizeof(clients[client_id].filename));
                            clients[client_id].task_start_time = atoi(param3);
                            clients[client_id].task_share = atoi(param4);
                            clients[client_id].has_task = true;
					}
                    MDEBUG_PRINT(("Init:  Setting Client ID %d to share filename %s at startTime %d sharing %d\n",
                    client_id, param2, atoi(param3), atoi(param4)));
                    break;
				default:
					MDEBUG_PRINT(("Init:  Something horrible has happened while parsing the file\n"));


                }
			}
		}
    //*bar = *clients;
    MDEBUG_PRINT(("Init:  Closing file\n"));

    // Close file
	fclose(in_file);
}


