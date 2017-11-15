//Router.c
//Author: Arianna Vacca
//CS457 P3

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include "manager.h"

void client_connect(int udp_port);
void router_initial_contact();

struct router_packet me;
struct udp_port udp_port_list;
char ip[50];

int main(int argc, char* argv[]){
    //Get IP
    get_ip(ip);

    //Get UDP port.
    int udp_port;
    do{
        srand(atoi(argv[0]));
        udp_port = (rand() % 8980) + 1015;
    }while(udp_port == 7014);

    printf("My port: %d.\n", udp_port);

    client_connect(udp_port);
}

void client_connect(int udp_port){
    char ip[50];
    get_ip(ip);
    struct sockaddr_in server;

    int sockit = socket(AF_INET, SOCK_STREAM, 0);
    if(sockit == -1){
        printf("Could not create router socket! Exiting program.\n");
        exit(1);
    }

    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(7014);

    if(inet_pton(AF_INET, ip, &(server.sin_addr))<=0){
            printf("Invalid IP address given.\n");
            close(sockit);
            exit(1);
    }

    if(connect(sockit, (struct sockaddr*)&server, sizeof(server))<0){
        printf("Connection failed. Please make sure you are using a valid IP address and port number.\n");
        close(sockit);
        exit(1);
    }

    //Sending UDP port information.
    if(send(sockit, &udp_port, sizeof(int), 0)<0){
        printf("Send failed.\n");
        close(sockit);
        exit(1);
    }

    if(recv(sockit, &me, sizeof(me), MSG_WAITALL) < 0){
        printf("Could not receive packet from manager. Exiting program.\n");
        close(sockit);
        exit(1);
    }

    if(me.udp_port != udp_port){
        printf("My ports don't match!\n");
    }

    int ready = 1;
    if(send(sockit, &ready, sizeof(int), 0)<0){
        printf("Send 1 failed.");
        close(sockit);
        exit(1);
    }

    if(recv(sockit, &udp_port_list, sizeof(udp_port_list), MSG_WAITALL) < 0){
        printf("Could not receive packet from manager. Exiting program.\n");
        close(sockit);
        exit(1);
    }

    for(int i = 0; i< 2; i++){
        printf("Router %d list: %d\n", me.node_num, udp_port_list.router[i]);
    }

    router_initial_contact();
    if(send(sockit, &ready, sizeof(int), 0)<0){
        printf("Send 2 failed.\n");
        close(sockit);
        exit(1);
    }
}

void router_initial_contact(){
    int id = fork();

    if(id < 0){
        printf("fork() was not successful. Exiting program.\n");
        exit(1);
    }
    else if(id == 0){ //Child process. Should be the client, loops through number of connections. Receives and sends.
        struct sockaddr_in si_other;
        int s;
        socklen_t slen = sizeof(si_other);

        if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
            printf("Can't make socket in child!\n");
            exit(1);
        }

        for(int i = 0; i < me.link.num_edges; i++){

            memset((char *) &si_other, 0, sizeof(si_other));
            si_other.sin_family = AF_INET;
            si_other.sin_addr.s_addr = inet_addr(ip);
            si_other.sin_port = htons(udp_port_list.router[me.link.dest[i]]);
            printf("Router %d is contacting router %d on their port %d.\n", me.node_num, me.link.dest[i], udp_port_list.router[me.link.dest[i]]);

            for(int i = 0; i< 2; i++){
                printf("Router %d list: %d\n", me.node_num, udp_port_list.router[i]);
            }

            int buf;
            if (sendto(s, &buf, sizeof(int), 0, (struct sockaddr *) &si_other, slen) == -1){
                printf("Send from in child failed.\n");
                exit(1);
            }

            if (recvfrom(s, &buf, sizeof(int), 0, (struct sockaddr *) &si_other, &slen) == -1){
                printf("Recv from in child failed.\n");
                exit(1);
            }
        }
        exit(0);
    }


    struct sockaddr_in si_me, si_other;
    int s;
    socklen_t slen = sizeof(si_other);

    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        printf("Parent socket failed.\n");
        exit(1);
    }

    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));

    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(me.udp_port);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind socket to port
    if( bind(s, (struct sockaddr*)&si_me, sizeof(si_me)) == -1){
        printf("Parent bind failed.\n");
        exit(1);
    }

    for(int i = 0; i < me.link.num_edges; i++){
        int request = 1;

        printf("On router %d, waiting to hear something.\n", me.node_num);
        if(recvfrom(s, &request, sizeof(int), 0, (struct sockaddr *) &si_other, &slen) == -1){
            printf("Failed to receive from other router.\n");
            exit(1);
        }

        printf("Received something!\n");
        if(sendto(s, &request, sizeof(int), 0, (struct sockaddr *) &si_other, slen) == -1){
            printf("Failed to send to other router.\n");
            exit(1);
        }
    }

    //Wait for routers to exit.
    int status;
    waitpid(id, &status, 0);

    //Tell manager we are done!

    //contact_others(s);
}

void contact_others(int sockit){

}
