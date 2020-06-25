# CN2019_project-TCP/UDP File Transmission

This repository contains two file transmitting and video streaming applications, which use the TCP and UDP protocols respectively.

## Requrements

- GCC/G++ Compiler
- OpenCV 3.0 or higher
- linux-like environment with GUI (e.g.: Ubuntu)

## TCP Application

In this application, a client can list the files in the server, download files from the server, upload files to the server, and stream a .mpg video on the server.

### Usage

1. In the command line shell, change to the TCP directory and type `make`. The programs will be compiled with the provided MAKEFILE.
2. Type `./server [port]` in the shell to launch the server with a specific port. Here is an example: `./server 22`
4. Open another shell and change to the directory where the program "client" is. Type `./client [ip:port]` in the shell to launch a client and connect to the server. The `ip` is the IP address of the server and `port` is the same in step two. Here is an example: `./client 127.0.0.1:22`
5. Now, you can use the following command in the client:
    1. `ls` : the files in the server folder will be listed.
    2. `put <filename>` : upload a file in the client folder to the server folder.
    3. `get <filename>` : download a file from the server folder to the client folder.
    4. `play <videofile>` : stream a .mpg video in the server folder.

## UDP Application

In this application, the sender can send a file to the folder of the receiver, or send a .mpg video frame by frame to the receiver. This application uses GO-BACK-N and congestion control to deal with package loss.

### Usage

1. In the command line shell, change to the UDP directory and type `make`. The programs will be compiled with the provided MAKEFILE.
2. Type `./agent <sender ip> <receiver ip> <sender port> <agent port> <receiver port> <lost rate>` in the shell to launch an agent which simulates package loss in the network. Here is an example: `./agent 127.0.0.1 127.0.0.1 5000 5001 5002 0.3`
3. Open another shell and change to the directory where the porgram "receiver" is. Type `./receiver <agent ip> <agent port> <receiver port>` in the shell. Here is an example: `./receiver 127.0.0.1 127.0.0.1 5000 5001 5002`
4. Open another shell and change to the  directory where the program "sender" is. Type `./sender <agent ip> <sender port> <agent port> <filename>` in the shell. The following are the command options:
    1. `.mpg` : the video specified in `<filename>` which is `.mpg` file will be played at the receiver end.
    2. other file type : the file specified in `<filename>` in the sender folder will be sent from the sender folder to the receiver folder.

    Here are some examples: 
    `./sender 127.0.0.1 127.0.0.1 5000 5001 5002 file.txt`
    `./sender 127.0.0.1 127.0.0.1 5000 5001 5002 video.mpg`

## Note
- The TCP application supports multiple connections from clients to the server. The UDP application only support one sender, one receiver, and one agent at the same time.