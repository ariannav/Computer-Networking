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

//Class Variables
struct links router[100];
int num_routers;
struct packet_src_dest packet[1000];
int num_packets = 0;
FILE* log_file;
int sockit;

int main(int argc, char* argv[]){
    //Check number of arguments.
    if(argc != 2){
        printf("Usage: $./manager <input file>");
        exit(1);
    }

    //Opening logging file.
    log_file = fopen("manager.out", "wb");
    signal(SIGINT, sighandler);

    //Parse file. CP 1 & 2
    parse_file(argv[1]);
    char line[100];
    sprintf(line, "File processed. Number of routers: %d, number of packets: %d", num_routers, num_packets);
    mlog(line);

    //Spawn N unix processes.
    spawn();
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

    //TODO: Continue working from here. 

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
        router[i].num_edges = 0;
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
            int edge = router[num].num_edges;

            fscanf(fptr, "%d", &curr);
            router[num].dest[edge] = curr;
            fscanf(fptr, "%d", &curr);
            router[num].cost[edge] = curr;
            router[num].num_edges++;
        }
    }

    //======== Checkpoint 1 ========

    //Parse packet source/destination pairs.
    if(fscanf(fptr, "%d", &curr) == 0){ bad_file(fptr); }
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
