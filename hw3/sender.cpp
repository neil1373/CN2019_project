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

#define DATA_SIZE 60000

using namespace std;
using namespace cv;

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
	int sendersocket, portNum, nBytes;
	segment send_pkt, recv_pkt;
	struct sockaddr_in sender, agent;
	socklen_t agent_size;
	char ip[2][50];
    int port[2], i;

	if(argc != 5){
        fprintf(stderr,"用法: %s <agent IP> <sender port> <agent port> <file path>\n", argv[0]);
        fprintf(stderr, "例如: ./sender local 8887 8888 tmp.mpg\n");
        exit(1);
    } 
    else {
        setIP(ip[0], "local");
        setIP(ip[1], argv[1]);

        sscanf(argv[2], "%d", &port[0]);
        sscanf(argv[3], "%d", &port[1]);

    }

    /*Create UDP socket*/
    sendersocket = socket(PF_INET, SOCK_DGRAM, 0);

    /*Configure settings in sender struct*/
    sender.sin_family = AF_INET;
    sender.sin_port = htons(port[0]);
    sender.sin_addr.s_addr = inet_addr(ip[0]);
    memset(sender.sin_zero, '\0', sizeof(sender.sin_zero));  

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 20000;
	if (setsockopt(sendersocket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	    perror("Error");
	}

    /*bind socket*/
    bind(sendersocket,(struct sockaddr *)&sender,sizeof(sender));

    /*Configure settings in agent struct*/
    agent.sin_family = AF_INET;
    agent.sin_port = htons(port[1]);
    agent.sin_addr.s_addr = inet_addr(ip[1]);
    memset(agent.sin_zero, '\0', sizeof(agent.sin_zero));    

	/*Initialize size variable to be used later on*/
    agent_size = sizeof(agent);

    int filenamelen = strlen(argv[4]);
    char filenamerev[1000] = {};
    for (int i = 0; i < filenamelen; i++){
        if(argv[4][filenamelen - 1 - i] == '.'){
            filenamerev[i] = '\0';
            break;
        }
        filenamerev[i] = argv[4][filenamelen - 1 - i];
    }
    filenamerev[filenamelen] = argv[4][filenamelen];
    if (strcmp(filenamerev, "gpm") != 0){   //reverse of "mpg"
        fprintf(stderr, "The %s is not a mpg file.\n", argv[4]);
        exit(1);
    }
    FILE *stream = fopen(argv[4], "rb");
    if (stream == NULL){
        fprintf(stderr, "Videofile %s doesn't exist.\n", argv[4]);
        exit(1);
    }

    VideoCapture cap(argv[4]);
    
    // get the resolution of the video
    int width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
    int height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);

    //sendto(sendersocket, &send_pkt, sizeof(send_pkt), 0, (struct sockaddr *)&agent, agent_size);

    int thres = 16, winsize = 1, prev_win = 1;	//critical part

    memset(&send_pkt, 0, sizeof(send_pkt));
    memset(&recv_pkt, 0, sizeof(recv_pkt));

    sprintf(send_pkt.data, "%d %d", width, height);
    send_pkt.head.length = strlen(send_pkt.data);
    sendto(sendersocket, &send_pkt, sizeof(send_pkt), 0, (struct sockaddr *)&agent, agent_size);
    fprintf(stderr, "first sent\n");
    recvfrom(sendersocket, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr *)&agent, &agent_size);
    fprintf(stderr, "first recv\n");
    Mat imgServer = Mat::zeros(height, width, CV_8UC3);
    if(!imgServer.isContinuous()){
         imgServer = imgServer.clone();
    }

    int imgSize = imgServer.total() * imgServer.elemSize();
    uchar imgbuf[imgSize];

    int remain_num = 0;
    int pkt_seq_num = 0;
    int acked_num = 0, prev_acked_num = 0;
    int resnd_num = 0;

    while(1){
    	if (remain_num == 0){	/*Capture a Video frame*/
    		cap >> imgServer;
            imgSize = imgServer.total() * imgServer.elemSize();
        	memcpy(imgbuf, imgServer.data, imgSize);
        	remain_num = imgSize;
    	}
        if (imgSize == 0){
            send_pkt.head.fin = 1;
            sendto(sendersocket, &send_pkt, sizeof(send_pkt), 0, (struct sockaddr *)&agent, agent_size);
            printf("send\tfin\n");
            recvfrom(sendersocket, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr *)&agent, &agent_size); 
            printf("recv\tfinack\n");
            break;
        }
    	segment send_datas[winsize - resnd_num];
    	memset(send_datas, 0, sizeof(send_datas));
    	for (int i = 0; i < winsize - resnd_num; i++){
    		int copy_len = (DATA_SIZE < remain_num) ? DATA_SIZE : remain_num;
    		memcpy(send_datas[i].data, &(imgbuf[imgSize - remain_num]), copy_len);
    		send_datas[i].head.length = copy_len;
    		remain_num -= copy_len;
    		pkt_seq_num++;
    		send_datas[i].head.seqNumber = pkt_seq_num;
    	}
    	for (int i = 0; i < winsize - resnd_num; i++){
    		sendto(sendersocket, &(send_datas[i]), sizeof(send_datas[i]), 0, (struct sockaddr *)&agent, agent_size);
    		printf("send\tdata\t#%d,\twinSize = %d\n", send_datas[i].head.seqNumber, winsize);
    	}
    	bool success = true;
    	prev_acked_num = acked_num;
    	for (int i = 0; i < winsize - resnd_num; i++){
    		memset(&recv_pkt, 0, sizeof(recv_pkt));
    		int recvbytes = recvfrom(sendersocket, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr *)&agent, &agent_size);
    		if (recvbytes > 0){
    			printf("recv\tack \t#%d\n", recv_pkt.head.ackNumber);
    			if (recv_pkt.head.ackNumber > acked_num){
    				acked_num = recv_pkt.head.ackNumber;
    			}
    		}
    		else{
    			thres = winsize / 2;
    			if (thres < 1)	thres = 1;
    			prev_win = winsize;
    			winsize = 1;
    			printf("time\tout,\t\tthreshold = %d\n", thres);
    			success = false;
    			break;
    		}
    	}
    	if (acked_num == pkt_seq_num){
    		winsize = (winsize < thres) ? (winsize * 2) : (winsize + 1);
            resnd_num = 0;
    	}
        else if (success == true){
            thres = winsize / 2;
            if (thres < 1)  thres = 1;
            prev_win = winsize;
            winsize = 1;
            printf("time\tout,\t\tthreshold = %d\n", thres);
        }
        //resend
        int remain_pkt = pkt_seq_num - acked_num;
        while (acked_num != pkt_seq_num){
    		//printf("%d %d %d %d\n", pkt_seq_num, acked_num, prev_acked_num, acked_num - prev_acked_num);
    		int start = acked_num - prev_acked_num;
            int range = winsize;
            if (remain_pkt < range){
                resnd_num = remain_pkt;
                range = remain_pkt;
            }
    		for (int i = 0; i < range; i++){
    			sendto(sendersocket, &(send_datas[start + i]), sizeof(send_datas[start + i]), 0, (struct sockaddr *)&agent, agent_size);
    			printf("resnd\tdata\t#%d,\twinSize = %d\n", send_datas[start + i].head.seqNumber, winsize);
    		}
    		bool resnd_success = true;
            int prev_resnd_acked = acked_num;
            for (int i = 0; i < range; i++){
                memset(&recv_pkt, 0, sizeof(recv_pkt));
                int recvbytes = recvfrom(sendersocket, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr *)&agent, &agent_size);
                if (recvbytes > 0){
                    printf("recv\tack \t#%d\n", recv_pkt.head.ackNumber);
                    /*if (recv_pkt.head.ackNumber % 32 == 0){
                        fprintf(stderr, "Receiver Flushing Notice\n");
                    }*/
                    if (recv_pkt.head.ackNumber > acked_num){
                        acked_num = recv_pkt.head.ackNumber;
                    }
                }
                else{
                    thres = winsize / 2;
                    if (thres < 1)  thres = 1;
                    prev_win = winsize;
                    winsize = 1;
                    printf("time\tout,\t\tthreshold = %d\n", thres);
                    resnd_success = false;
                    break;
                }
            }
            remain_pkt -= (acked_num - prev_resnd_acked);
            if (range == winsize && (acked_num - prev_resnd_acked == range)){
                winsize = (winsize < thres) ? (winsize * 2) : (winsize + 1);
                resnd_num = 0;
            }
            else if (resnd_success == true && range == winsize){
                thres = winsize / 2;
                if (thres < 1)  thres = 1;
                prev_win = winsize;
                winsize = 1;
                printf("time\tout,\t\tthreshold = %d\n", thres);
            }
        }
    }
    return 0;
}