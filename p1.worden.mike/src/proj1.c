#include "../include/manager.h"


/******************************************************************************
* Main:  Just the bare minimum
*
******************************************************************************/
int main(int argc, char *argv[]) {
    struct client clients[MAX_CLIENTS];
    int num_clients = 0;
    int timeout = 0;
    int management_port = 0;


    init( clients, &num_clients, &timeout);


    launch_manager(clients, &num_clients, &timeout, &management_port);


	return (0);
}

