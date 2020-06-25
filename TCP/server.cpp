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

#define BUFF_SIZE 1024
#define MAX_COUNT 65535

using namespace std;
using namespace cv;

int main(int argc, char** argv){

    int localSocket, remoteSocket, port = atoi(argv[1]);                               

    struct sockaddr_in localAddr,remoteAddr;
          
    int addrLen = sizeof(struct sockaddr_in);  

    localSocket = socket(AF_INET , SOCK_STREAM , 0);
    
    if (localSocket == -1){
        printf("socket() call failed!!");
        return 0;
    }

    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(port);


        
        if( bind(localSocket,(struct sockaddr *)&localAddr , sizeof(localAddr)) < 0) {
            printf("Can't bind() socket\n");
            return 0;
        }
        
        listen(localSocket , 1024);

	if (mkdir("Server", 0777) == -1){
        cerr << "Error :  " << strerror(errno) << endl;
    }
    else{
        cout << "Directory created" << endl;
    }

    fd_set master;
    fd_set read_fds;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(localSocket, &master);
    int maxfd = localSocket;
    while(1){
    	read_fds = master;
    	select(maxfd + 1, &read_fds, NULL, NULL, NULL);
    	for (int ifd = 3; ifd <= maxfd; ifd++){
    		if (FD_ISSET(ifd, &read_fds)){
    			if (ifd == localSocket){	//server
			    	std::cout <<  "Waiting for connections...\n"
			                <<  "Server Port:" << port << std::endl;

			        remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen);  
			        //printf("%d %d\n", remoteSocket, localSocket);
			        if (remoteSocket < 0) {
			            printf("accept failed!");
			            return 0;
			        }
			                
			        std::cout << "Connection accepted" << std::endl;
			        FD_SET(remoteSocket, &master);
			        maxfd = remoteSocket;
    			}	//server
    			else{
    				remoteSocket = ifd;
    				char Message[BUFF_SIZE] = {}, cmd1[BUFF_SIZE] = {}, cmd2[BUFF_SIZE] = {};
					/*int sent;
			        strcpy(Message,"Hello World!!\n");
			        sent = send(remoteSocket,Message,strlen(Message),0);
			        strcpy(Message,"Computer Networking is interesting!!\n");
			        sent = send(remoteSocket,Message,strlen(Message),0);*/
			        bzero(Message,sizeof(char)*BUFF_SIZE);
			        int nbytes;
					if ((nbytes = recv(remoteSocket,Message, 1024, 0)) > 0){
						printf("%s\n", Message);
						sscanf(Message, "%s %s", cmd1, cmd2);
						if (strcmp(cmd1, "ls") == 0){
							//printf("Doing command ls.\n");
							//do_ls("Server");
							DIR *dir_ptr;
							struct dirent *direntp;
						 	char *flow[MAX_COUNT];
							dir_ptr = opendir("Server");
							if(dir_ptr == NULL){
								fprintf(stderr,"ls: can not open %s","Server");
							}
							else{
								//printf("Open Directory %s Success!\n", "Server");
								char lsmsg[BUFF_SIZE], tmp[128];
								bzero(lsmsg, sizeof(char)*BUFF_SIZE);
								while((direntp = readdir(dir_ptr)) != NULL){
									sprintf(tmp, "%s\n",direntp->d_name);
									//puts(filename);
									//write(remoteSocket, filename, strlen(filename));
									strcat(lsmsg, tmp);
								}
								send(remoteSocket, lsmsg, strlen(lsmsg), 0);
								bzero(lsmsg, sizeof(char)*BUFF_SIZE);
								closedir(dir_ptr);
							}
						}
						else if (strcmp(cmd1, "get") == 0){
							printf("Doing command get %s.\n", cmd2);
							char filedes[1024];
							sprintf(filedes, "./Server/%s", cmd2);
							FILE *fp = fopen(filedes, "rb");
							if (fp == NULL){
								char sendstatus[BUFF_SIZE];
								sprintf(sendstatus, "File doesn't exist.");
								send(remoteSocket, sendstatus, strlen(sendstatus), 0);
							}
							else{
								char sendstatus[BUFF_SIZE];
								sprintf(sendstatus, "File exists.\n");
								send(remoteSocket, sendstatus, strlen(sendstatus), 0);
								sleep(1);
								int nbytes;
								ssize_t len = 0;
								char sendbuff[BUFF_SIZE];
								while((nbytes = fread(sendbuff, sizeof(char), BUFF_SIZE, fp)) > 0){
									len += nbytes;
									if (send(remoteSocket, sendbuff, nbytes, 0) == -1){
										perror("Can't send file.");
									}
									memset(sendbuff, 0, BUFF_SIZE);
								}
								sleep(1);
								sprintf(sendbuff, "Reach end of file... 73\n");
								send(remoteSocket, sendbuff, strlen(sendbuff), 0);
								fclose(fp);					
							}
						}
						else if (strcmp(cmd1, "put") == 0){
							printf("Doing command put %s.\n", cmd2);
							char filedes[1024];
							sprintf(filedes, "./Server/%s", cmd2);
							FILE *fp = fopen(filedes, "wb");
							if (fp == NULL){
								perror("Can't open file.");
							}
							else{
								int nbytes;
								ssize_t len = 0;
								char recvbuff[BUFF_SIZE];
								while((nbytes = recv(remoteSocket, recvbuff, BUFF_SIZE, 0)) > 0){
									//printf("%s\n", recvbuff);
									len += nbytes;
									if (strcmp(recvbuff, "Reach end of file... 73\n") == 0){
										printf("Finish.\n");
										break;
									}
									else if (fwrite(recvbuff, sizeof(char), nbytes, fp) != nbytes){
										perror("Error when writing file!");
									}
									memset(recvbuff, 0, BUFF_SIZE);
								}
								fclose(fp);					
							}
						}
						else if (strcmp(cmd1, "play") == 0){
							printf("Doing command play %s.\n", cmd2);
							char videofile[1024];
							sprintf(videofile, "./Server/%s", cmd2);
							// test if the videofile exists
							FILE *fp = fopen(videofile, "r");
							if (fp == NULL){
								char sendstatus[BUFF_SIZE] = {};
								sprintf(sendstatus, "File doesn't exist.");
								send(remoteSocket, sendstatus, strlen(sendstatus), 0);
							}
							else{
								fclose(fp);
								char sendstatus[BUFF_SIZE] = {};
								VideoCapture cap(videofile);
								int width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
								int height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
								sprintf(sendstatus, "%d %d", height, width);
								send(remoteSocket, sendstatus, strlen(sendstatus), 0);
								sleep(2);
								recv(remoteSocket, sendstatus, BUFF_SIZE, 0);
								if (strcmp(sendstatus, "Success!") == 0){
									printf("Start sending...\n");
									Mat imgServer;
									imgServer = Mat::zeros(height, width, CV_8UC3);  
									if(!imgServer.isContinuous()){
										imgServer = imgServer.clone();
									}
									while(1){
										cap >> imgServer;
										int imgSize = imgServer.total() * imgServer.elemSize();
										//printf("%d\n", imgSize);
										if (imgSize == 0){
											char statusmsg[BUFF_SIZE] = {};
											strcpy(statusmsg, "End!");
											send(remoteSocket, statusmsg, strlen(statusmsg), 0);
											bzero(statusmsg, sizeof(char) * BUFF_SIZE);
											sleep(1);
											recv(remoteSocket, statusmsg, BUFF_SIZE, 0);
											if (statusmsg[0] == 27){
												break;
											}
										}
										else{
											uchar imgbuffer[imgSize];
											memcpy(imgbuffer, imgServer.data, imgSize);
											int nbytes = send(remoteSocket, imgbuffer, imgSize, 0);
											//printf("%d\n", nbytes);
											char statusmsg[BUFF_SIZE] = {};
											recv(remoteSocket, statusmsg, BUFF_SIZE, 0);
											if (statusmsg[0] == 27){
												break;
											}
										}
									}
								}
								cap.release();
								printf("Video play finish.\n");
							}
						}
						else{
							char errorMessage[32];
							sprintf(errorMessage, "Command not found.\n");
							send(remoteSocket, errorMessage, strlen(errorMessage), 0);
						}
						bzero(Message,sizeof(char)*BUFF_SIZE);
					}
			        //close(remoteSocket);
			        //FD_CLR(remoteSocket, &master);
    			}	//clients
    		}	//if
    	}	//for
        

        
    }
    return 0;
}
