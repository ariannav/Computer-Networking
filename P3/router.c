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

int main(int argc, char* argv[]){
    //Get UDP port.
    int udp_port;
    do{
        srand(time(NULL));
        udp_port = (rand() + 1015) % 9999;
    }while(udp_port == 7014);

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
        printf("Send failed.");
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
        printf("Send failed.");
        close(sockit);
        exit(1);
    }

    if(recv(sockit, &udp_port_list, sizeof(udp_port_list), MSG_WAITALL) < 0){
        printf("Could not receive packet from manager. Exiting program.\n");
        close(sockit);
        exit(1);
    }
    router_initial_contact();
}

void router_initial_contact(){

}