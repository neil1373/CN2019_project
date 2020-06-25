#include <iostream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h> 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "opencv2/opencv.hpp"

#define BUFF_SIZE 1024

using namespace std;
using namespace cv;

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
    
    if (mkdir("Client", 0777) == -1){
        cerr << "Error :  " << strerror(errno) << endl;
    }
    else{
        cout << "Directory created" << endl;
    }
    //while(1){
        /*bzero(receiveMessage,sizeof(char)*BUFF_SIZE);
        if ((recved = recv(localSocket,receiveMessage,sizeof(char)*BUFF_SIZE,0)) < 0){
            cout << "recv failed, with received bytes = " << recved << endl;
            //break;
        }
        else if (recved == 0){
            cout << "<end>\n";
            break;
        }
        printf("%d:%s\n",recved,receiveMessage);*/
        char sentCommand[1024], cmd1[1000], cmd2[1000], cmd3[1000];
		while (fgets(sentCommand, 1024, stdin) != NULL){
			if (sentCommand[strlen(sentCommand) - 1] == '\n'){
				sentCommand[strlen(sentCommand) - 1] = '\0';
			}
			if (strcmp(sentCommand, "exit") == 0)	break;
			
			//send (localSocket, sentCommand, strlen(sentCommand), 0);	//all valid should send
			if (strcmp(sentCommand, "ls") == 0){
				send (localSocket, sentCommand, strlen(sentCommand), 0);
				//printf("Doing ls ...\n");
				bzero(receiveMessage,sizeof(char)*BUFF_SIZE);
				if ((recved = recv(localSocket,receiveMessage,sizeof(char)*BUFF_SIZE,0)) < 0){
				    cout << "recv failed, with received bytes = " << recved << endl;
				    //break;
				}
				else if (recved == 0){
				    cout << "<end>\n";
				    break;
				}
				printf("%s\n", receiveMessage);		
			}
			else if (sscanf(sentCommand, "%s %s %s", cmd1, cmd2, cmd3) == 2){
				//printf("%s\n%s\n%s\n", sentCommand, cmd1, cmd2);
				if (strcmp(cmd1, "get") == 0){
					send (localSocket, sentCommand, strlen(sentCommand), 0);
					//printf("Doing get...\n");
					//printf("Doing command put %s.\n", cmd2);
					char retstatus[1024];
					recv (localSocket, retstatus, 1024, 0);
					if (strcmp(retstatus, "File doesn't exist.") == 0){
						printf("The %s doesn't exist.\n", cmd2);
					}
					else{
						char filedes[1024];
						sprintf(filedes, "./Client/%s", cmd2);
						FILE *fp = fopen(filedes, "wb");
						if (fp == NULL){
							perror("Can't open file.");
						}
						else{
							int nbytes;
							ssize_t len = 0;
							char recvbuff[BUFF_SIZE];
							while((nbytes = recv(localSocket, recvbuff, BUFF_SIZE, 0)) > 0){
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
				}
				else if (strcmp(cmd1, "put") == 0){
					char filedes[1024];
					sprintf(filedes, "./Client/%s", cmd2);
					FILE *fp = fopen(filedes, "rb");
					if (fp == NULL){
						printf("The %s doesn't exist.\n", cmd2);
					}
					else{
						send (localSocket, sentCommand, strlen(sentCommand), 0);
						sleep(1);
						int nbytes;
						ssize_t len = 0;
						char sendbuff[BUFF_SIZE];
						while((nbytes = fread(sendbuff, sizeof(char), BUFF_SIZE, fp)) > 0){
							len += nbytes;
							if (send(localSocket, sendbuff, nbytes, 0) == -1){
								perror("Can't send file.");
							}
							memset(sendbuff, 0, BUFF_SIZE);
						}
						sleep(1);
						sprintf(sendbuff, "Reach end of file... 73\n");
						send(localSocket, sendbuff, strlen(sendbuff), 0);
						fclose(fp);					
					}
					//printf("Doing put...\n");
				}
				else if (strcmp(cmd1, "play") == 0){
					int cmd2len = strlen(cmd2);
					char cmd2rev[1000] = {};
					for (int i = 0; i < cmd2len; i++){
						if(cmd2[cmd2len - 1 - i] == '.'){
							cmd2rev[i] = '\0';
							break;
						}
						cmd2rev[i] = cmd2[cmd2len - 1 - i];
					}
					cmd2rev[cmd2len] = cmd2[cmd2len];
					if (strcmp(cmd2rev, "gpm") != 0){	//reverse of "mpg"
						printf("The %s is not a mpg file. %s\n", cmd2, cmd2rev);
					}
					else{
						send (localSocket, sentCommand, strlen(sentCommand), 0);
						printf("Doing play...\n");
						char retstatus[BUFF_SIZE] = {};
						sleep(1);
						recv(localSocket, retstatus, BUFF_SIZE, 0);
						if (strcmp(retstatus, "File doesn't exist.") == 0){
							printf("The %s doesn't exist.\n", cmd2);
						}
						else{
							//printf("%s\n", retstatus);
							int height, width;
							sscanf(retstatus, "%d %d", &height, &width);
							printf("%d, %d\n", height, width);
							Mat imgClient = Mat::zeros(height, width, CV_8UC3);
							if(!imgClient.isContinuous()){
								imgClient = imgClient.clone();
							}
							strcpy(retstatus, "Success!");
							send(localSocket, retstatus, strlen(retstatus), 0);
							int halt = 0;
							while(1){
								int imgSize = imgClient.total() * imgClient.elemSize();
								//printf("%d\n", imgSize);
								uchar imgbuffer[imgSize];
								int nbytes = recv(localSocket, imgbuffer, imgSize, 0);
								char tmp[imgSize] = {};
								memcpy(tmp, imgbuffer, nbytes);
								if (strcmp(tmp, "End!") == 0){
									printf("no data received.\n");
									while(1){
										char c = (char)waitKey(33.3333);
										if(c==27){
											char haltmsg[BUFF_SIZE] = {};
											haltmsg[0] = c;
											send(localSocket, haltmsg, strlen(haltmsg), 0);
											//while((recv(localSocket, imgbuffer, imgSize, 0)) > 0){
											//}
											halt = 1;
											//break;
										}
									}
								}
								if (halt == 1){
									break;
								}
								else{
									int remaining = imgSize - nbytes;
									//printf("%d\n", nbytes);
									if (remaining){
										uchar imgbuf2[remaining];
										while (remaining != 0){
											int n2bytes = recv(localSocket, imgbuf2, remaining, 0);
											remaining -= n2bytes;
											memcpy(&(imgbuffer[nbytes]), imgbuf2, n2bytes);
											nbytes += n2bytes;
										}
									}
									//printf("%d\n", nbytes);
									uchar *iptr = imgClient.data;
									memcpy(iptr, imgbuffer, imgSize);
									imshow("Video", imgClient);
								
									char c = (char)waitKey(33.3333);
									if(c==27){
										char haltmsg[BUFF_SIZE] = {};
										haltmsg[0] = c;
										send(localSocket, haltmsg, strlen(haltmsg), 0);
										//while((recv(localSocket, imgbuffer, imgSize, 0)) > 0){
										//}
										break;
									}
									else{
										char msg[BUFF_SIZE] = {};
										strcpy(msg, "P");
										send(localSocket, msg, strlen(msg), 0);
									}
								}
								
							}
							//cap.release();
							destroyAllWindows();
						}
					}					
				}
				else{
					printf("Command not found.\n");
				}
			}
			else{
				printf("Command format error.\n");
			}
			bzero(sentCommand,sizeof(char)*BUFF_SIZE);
		}
    //}
    printf("close Socket\n");
    close(localSocket);
    return 0;
}
