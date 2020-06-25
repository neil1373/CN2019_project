#include <iostream>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h> 
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include "opencv2/opencv.hpp"

#define BUFF_SIZE 32
#define DATA_SIZE 60000
#define MAXIMGSIZE 6600000

using namespace std;
using namespace cv;

uchar imgbuf[MAXIMGSIZE];

typedef struct {
	int length;
	int seqNumber;
	int ackNumber;
	int fin;
	int syn;
	int ack;
} header;

typedef struct{
	header head;
	char data[DATA_SIZE];
} segment;

void setIP(char *dst, char *src) {
    if(strcmp(src, "0.0.0.0") == 0 || strcmp(src, "local") == 0
            || strcmp(src, "localhost")) {
        sscanf("127.0.0.1", "%s", dst);
    } else {
        sscanf(src, "%s", dst);
    }
}

int main(int argc, char* argv[]){
	int recvsocket, portNum, nBytes;
	segment recv_data, check_msg;
	struct sockaddr_in agent, receiver;
	socklen_t agent_size;
	char ip[2][50];
    int port[2], i;

	if(argc != 4){
        fprintf(stderr,"用法: %s <agent IP> <agent port> <recv port>\n", argv[0]);
        fprintf(stderr, "例如: ./receiver local 8888 8889\n");
        exit(1);
    } 
    else {
        setIP(ip[0], argv[1]);
        setIP(ip[1], "local");

        sscanf(argv[2], "%d", &port[0]);
        sscanf(argv[3], "%d", &port[1]);
    }

    /*Create UDP socket*/
    recvsocket = socket(PF_INET, SOCK_DGRAM, 0);

    /*Configure settings in agent struct*/
    agent.sin_family = AF_INET;
    agent.sin_port = htons(port[0]);
    agent.sin_addr.s_addr = inet_addr(ip[0]);
    memset(agent.sin_zero, '\0', sizeof(agent.sin_zero));    

    /*Configure settings in receiver struct*/
    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(port[1]);
    receiver.sin_addr.s_addr = inet_addr(ip[1]);
    memset(receiver.sin_zero, '\0', sizeof(receiver.sin_zero));   

    /*bind socket*/
    bind(recvsocket,(struct sockaddr *)&receiver,sizeof(receiver));

	/*Initialize size variable to be used later on*/
    agent_size = sizeof(agent);

    int height = 0, width = 0;
    memset(&recv_data, 0, sizeof(recv_data));
    memset(&check_msg, 0, sizeof(check_msg));
	recvfrom(recvsocket, &recv_data, sizeof(recv_data), 0, (struct sockaddr *)&agent, &agent_size);
	sscanf(recv_data.data, "%d %d", &width, &height);
	check_msg.head.fin = 0;
	check_msg.head.syn = 0;
	check_msg.head.ack = 1;
	sendto(recvsocket, &check_msg, sizeof(check_msg), 0, (struct sockaddr *)&agent, agent_size);
	printf("%dX%d\n", width, height);

    Mat imgClient = Mat::zeros(height, width, CV_8UC3);
    if(!imgClient.isContinuous()){
         imgClient = imgClient.clone();
    }

    int imgSize = height * width * 3;
    int tmpSize = 0;
    uchar *imgptr = imgClient.data;

    int ack_seq = 0;
    bool finish = false;
    while(!finish){
    	segment flush_buf[BUFF_SIZE];	//wait to flush
    	segment recv_data;	//from sender
    	segment check_msg;	//send back
    	memset(flush_buf, 0, sizeof(flush_buf));
    	memset(&recv_data, 0, sizeof(recv_data));
    	memset(&check_msg, 0, sizeof(check_msg));

    	bool to_flush = false;

    	while (!to_flush){
    		int recv_size = recvfrom(recvsocket, &recv_data, sizeof(recv_data), 0, (struct sockaddr *)&agent, &agent_size);
    		if (recv_size > 0){
    			if (recv_data.head.fin == 1){
    				printf("recv\tfin\n");
    				check_msg.head.fin = 1;
    				check_msg.head.syn = 0;
    				check_msg.head.ack = 1;
    				to_flush = true;
    				finish = true;
    				sendto(recvsocket, &check_msg, sizeof(check_msg), 0, (struct sockaddr *)&agent, agent_size);
    				printf("send\tfinack\n");
    			}
    			else{
    				int tmp_seqnum = recv_data.head.seqNumber;
    				if (tmp_seqnum - 1 != ack_seq){	//drop data
    					printf("drop\tdata\t#%d\n", tmp_seqnum);
    					check_msg.head.ackNumber = ack_seq;
    					check_msg.head.fin = 0;
    					check_msg.head.syn = 0;
    					check_msg.head.ack = 1;
    					sendto(recvsocket, &check_msg, sizeof(check_msg), 0, (struct sockaddr *)&agent, agent_size);
    					printf("send\tack \t#%d\n", ack_seq);
    				}
    				else{	//recv data
    					ack_seq += 1;
    					flush_buf[(ack_seq - 1) % BUFF_SIZE] = recv_data;
    					printf("recv\tdata\t#%d\n", tmp_seqnum);
    					check_msg.head.ackNumber = ack_seq;
    					check_msg.head.fin = 0;
    					check_msg.head.syn = 0;
    					check_msg.head.ack = 1;
    					sendto(recvsocket, &check_msg, sizeof(check_msg), 0, (struct sockaddr *)&agent, agent_size);
    					printf("send\tack \t#%d\n", ack_seq);
    					if (ack_seq % BUFF_SIZE == 0){	//flush
    						to_flush = true;
    					}
    				}
    			}
    		}
    	}
        if (finish == true){
            break;
        }
    	printf("%s\n", "flush");
    	if (tmpSize < imgSize){
    		for (int i = 0; i < BUFF_SIZE; i++){
    			//fprintf(stderr, "%d\n", i);
    			memcpy(imgbuf + tmpSize, flush_buf[i].data, flush_buf[i].head.length);
    			tmpSize += flush_buf[i].head.length;
    			//fprintf(stderr, "%d\n", tmpSize);
    			if (tmpSize == imgSize){
    				memcpy(imgptr, imgbuf, imgSize);
    				imshow("Receiver", imgClient);
					char c = (char)waitKey(33.3333);
					if(c == 27){
						finish = true;
						break;
					}
					tmpSize = 0;
					memset(imgbuf, 0, sizeof(imgbuf));
    			}
                int recv_size = recvfrom(recvsocket, &recv_data, sizeof(recv_data), 0, (struct sockaddr *)&agent, &agent_size);
                if (recv_size > 0){
                    int tmp_seqnum = recv_data.head.seqNumber;
                    printf("drop\tdata\t#%d\n", tmp_seqnum);
                    check_msg.head.ackNumber = ack_seq;
                    check_msg.head.fin = 0;
                    check_msg.head.syn = 0;
                    check_msg.head.ack = 1;
                    sendto(recvsocket, &check_msg, sizeof(check_msg), 0, (struct sockaddr *)&agent, agent_size);
                    printf("send\tack \t#%d\n", ack_seq);
                }
    		}
    	}
    }
    destroyAllWindows();
    return 0;
}