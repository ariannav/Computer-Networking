//Manager.h
//Author: Arianna Vacca
//CS457 P3

#ifndef MANAGER_H
#define MANAGER_H

#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

//Data Structures
struct links{
    int num_edges;
    int dest[100];
    int cost[100];
};

struct router_packet{
    int node_num;
    int udp_port;
    int tcp_port;
    int num_routers;
    struct links link;
};

struct packet_src_dest{
    int src;
    int dest;
};

struct udp_port{
    int router[100];
}; 

//Functions
void get_ip(char* ip){
    char hostname[255];
    gethostname(hostname, 255);

    //Get IP Address from hostname
    struct addrinfo hints, *info, *rough_ip;
    struct sockaddr_in *decoy_server;
    int zero;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if((zero = getaddrinfo(hostname, "7014", &hints, &info)) != 0){
        printf("Problem converting hostname to IP address.\n");
        exit(1);
    }

    //Here, I am cycling through the IP addresses to find the first available one.
    for(rough_ip = info; rough_ip != NULL; rough_ip = rough_ip->ai_next){
        decoy_server = (struct sockaddr_in*) rough_ip->ai_addr;
        strcpy(ip, inet_ntoa(decoy_server->sin_addr));
    }

    freeaddrinfo(info);
}

#endif
