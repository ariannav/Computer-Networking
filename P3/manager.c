//Manager.c
//Author: Arianna Vacca
//CS457 P3

//Includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include "manager.h"

//Functions
void parse_file(char* filename);
void bad_file(FILE *fptr);
void spawn();
void mlog(char* usr_message);
void serve();
void sighandler(int sig);
void create_router(int router_number, int tcp_port, int udp_port);
int tcp_to_router(int tcp_socket);

//Class Variables
struct links router_link[100];
struct router_packet router_info[100];
int num_routers;
struct packet_src_dest packet[1000];
int num_packets;
FILE* log_file;
int sockit;

int main(int argc, char* argv[]){
    //Check number of arguments.
    if(argc != 2){
        printf("Usage: $./manager <input file>\n");
        exit(1);
    }

    //Opening logging file.
    log_file = fopen("manager.out", "wb");
    signal(SIGINT, sighandler);

    //Parse file. CP 1 & 2
    parse_file(argv[1]);
    printf("Checkpoint 1 complete.\nCheckpoint 2 complete.\n");
    char line[200];
    sprintf(line, "File processed. Number of routers: %d, number of packets: %d.", num_routers, num_packets);
    mlog(line);

    //Spawn N unix processes.
    spawn(); //TODO: TIME TO DEBUG
}

//-------------------------- CREATING ROUTERS ----------------------------------

void spawn(){
    //Spawn number of routers
    int process_id[num_routers];
    for(int i = 0; i < num_routers; i++){
        int id = fork();

        if(id < 0){
            printf("fork() was not successful. Exiting program.\n");
            exit(1);
        }
        else if(id == 0){ //Router process.
            char st_fd[8];
            execlp("./router", st_fd, (char *)NULL);
        }
        else{
            char line[100];
            sprintf(line, "Created router process #%d", i);
            mlog(line);
            process_id[i] = id;
        }
    }

    //======== Checkpoint 3 ========
    serve();

    //Wait for routers to exit.
    for(int i = 0; i < num_routers; i++){
        int status;
        waitpid(process_id[i], &status, 0);
    }

}

void serve(){
    //Create socket.
    struct sockaddr_in manager;
    if((sockit = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        printf("Error creating socket. Exiting program.\n");
        exit(1);
    }
    mlog("Created manager socket.");

    //Make socket reusable.
    setsockopt(sockit, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    char ip[50];
    get_ip(ip);
    manager.sin_addr.s_addr = inet_addr(ip);
    manager.sin_family = AF_INET;
    manager.sin_port = htons(7014);

    //Connecting to a remove server.
    if(bind(sockit, (struct sockaddr*) &manager, sizeof(manager)) < 0){
      printf("Binding error. Exiting ss program.\n");
      close(sockit);
      exit(1);
    }

    char line[100];
    sprintf(line, "Broadcasting on IP: %s", ip);
    mlog(line);

    //Sharing initial information with routers.
    fd_set readfds;
    struct sockaddr_in router;
    socklen_t size = sizeof(router);
    int router_socket[num_routers], new_socket, activity, sd, max_sd, num_ready = 0;
    memset(router_socket, '0', 100);

    while(num_ready < num_routers){
        FD_ZERO(&readfds);
        FD_SET(sockit, &readfds);
        max_sd = sockit;

        //Adding existing routers to readfds
        for(int i = 0; i < num_routers; i++){
            sd = router_socket[i];
            if(sd > 0){
                FD_SET(sd, &readfds);
            }
            if(sd > max_sd){
                max_sd = sd;
            }
        }

        if((activity = select(max_sd+1, &readfds, NULL, NULL, NULL))<0){
         printf("Select failed. Exiting program.");
         close(sockit);
         exit(1);
        }

        //Incoming connection? New router contacting me <3 so cute ^.^
        if(FD_ISSET(sockit, &readfds)){
            if((new_socket = accept(sockit, (struct sockaddr*) &router, &size)) < 0){
                printf("Last stepping stone not accepted. Exiting program.\n");
                close(sockit);
                exit(1);
            }

            //Receiving UDP port number.
            int udp_port;
            if(recv(new_socket, &udp_port, sizeof(int), MSG_WAITALL) < 0){
              printf("Could not receive packet from last ss. Exiting program.\n");
              close(new_socket);
              exit(1);
            }

            int router_number;
            for(int j = 0; j < num_routers; j++){
                if(router_socket[j] == 0){
                    router_socket[j] = new_socket;
                    router_number = j;
                    break;
                }
            }
            create_router(router_number, new_socket, udp_port);
        }

        //Existing connection? Receiving "Ready!" signal.
        for(int i = 0; i < num_routers; i++){
            sd = router_socket[i];
            if(FD_ISSET(sd, &readfds)){
                int status;
                if(recv(sd, &status, sizeof(status), MSG_WAITALL) < 0){
                  printf("Could not receive packet from last ss. Exiting program.\n");
                  close(new_socket);
                  exit(1);
                }
                if(status){
                    num_ready++;
                    int which_router_number = tcp_to_router(sd);
                    sprintf(line, "Router %d is ready.", which_router_number);
                    mlog(line);
                }
                break;
            }
        }
    }
}

//Takes router number and udp port and creates a router packet.
//Then it passes that packet to the router.
void create_router(int router_number, int tcp_port, int udp_port){
    //Creating router packet
    router_info[router_number].node_num = router_number;
    router_info[router_number].udp_port = udp_port;
    router_info[router_number].tcp_port = tcp_port;
    router_info[router_number].num_routers = num_routers;
    router_info[router_number].link = router_link[router_number];

    if(send(tcp_port, &router_info[router_number], sizeof(router_info[router_number]), 0)<0){
        printf("Send failed.");
        close(tcp_port);
        exit(1);
    }
}

//Takes a tcp socket number and generates a router number.
int tcp_to_router(int tcp_socket){
    for(int i = 0; i < num_routers; i++){
        if(router_info[i].tcp_port == tcp_socket){
            return router_info[i].node_num;
        }
    }
    return -1;
}

//---------------------------- FILE PARSING ------------------------------------
//Parses input file.
void parse_file(char* filename){
    FILE *fptr;
    if((fptr = fopen(filename,"r")) == NULL){
        printf("Problem opening file %s. Exiting program.\n", filename);
    }

    //Get number of routers.
    if(fscanf(fptr, "%d", &num_routers) == 0){ bad_file(fptr); }
    for(int i = 0; i < num_routers; i++){
        router_link[i].num_edges = 0;
    }

    //Parsing the list of links.
    int num, curr;
    while(num != -1){
        if(fscanf(fptr, "%d", &num) == 0){  //No number present.
            bad_file(fptr);
        }
        else if(num == -1){  //Reached end of link list.
            break;
        }
        else{  //Link information.
            int edge = router_link[num].num_edges;

            fscanf(fptr, "%d", &curr);
            router_link[num].dest[edge] = curr;
            fscanf(fptr, "%d", &curr);
            router_link[num].cost[edge] = curr;
            router_link[num].num_edges++;
        }
    }

    //======== Checkpoint 1 ========

    //Parse packet source/destination pairs.
    if(fscanf(fptr, "%d", &curr) == 0){ bad_file(fptr); }
    int num_packets = 0;

    while(curr != -1){
        packet[num_packets].src = curr;
        fscanf(fptr, "%d", &curr);
        packet[num_packets].dest = curr;
        fscanf(fptr, "%d", &curr);
        num_packets++;
    }

    //======== Checkpoint 2 ========
    fclose(fptr);
}

//Produces error message when file is incorrectly formatted.
void bad_file(FILE *fptr){
    printf("Improperly formatted file! Exiting program.\n");
    fclose(fptr);
    exit(1);
}

//-------------------------- FILE LOGGING --------------------------------------
void mlog(char* usr_message){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    char line[200];
    sprintf(line, "%d:%d:%d \t| %s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, usr_message);
    fwrite(line, sizeof(char), strlen(line), log_file);
}

//----------------------------- SIGNAL -----------------------------------------
void sighandler(int sig){
    printf(" Exiting program.\n");
    fclose(log_file);
    exit(1);
}
