#include <iostream>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h> 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFF_SIZE 1024

using namespace std;
int main(int argc , char *argv[])
{

    int localSocket, recved;
    localSocket = socket(AF_INET , SOCK_STREAM , 0);

    if (localSocket == -1){
        printf("Fail to create a socket.\n");
        return 0;
    }

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    const char *delimiter = ":";
    char svr_address[32];
    strcpy(svr_address, argv[1]);
    char *ipaddress = strtok(svr_address, delimiter);
    char *port = strtok(NULL, delimiter);
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr(ipaddress);
    info.sin_port = htons(atoi(port));


    int err = connect(localSocket,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        printf("Connection error\n");
        return 0;
    }
    char receiveMessage[BUFF_SIZE] = {};
    while(1){
        bzero(receiveMessage,sizeof(char)*BUFF_SIZE);
        if ((recved = recv(localSocket,receiveMessage,sizeof(char)*BUFF_SIZE,0)) < 0){
            cout << "recv failed, with received bytes = " << recved << endl;
            break;
        }
        else if (recved == 0){
            cout << "<end>\n";
            break;
        }
        printf("%d:%s\n",recved,receiveMessage);

    }
    printf("close Socket\n");
    close(localSocket);
    return 0;
}

