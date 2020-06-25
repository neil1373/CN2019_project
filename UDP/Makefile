CC = gcc
PP = g++
OPENCV =  `pkg-config --cflags --libs opencv`

RECEIVER = receiver.cpp
SENDER = sender.cpp
AGENT = agent.c
RECV = receiver
SND = sender
AGT = agent

all: sender receiver agent
  
sender: $(SENDER)
	$(PP) $(SENDER) -o $(SND)  $(OPENCV)
receiver: $(RECEIVER)
	$(PP) $(RECEIVER) -o $(RECV)  $(OPENCV)
agent:	$(AGENT)
	$(CC) $(AGENT) -o $(AGT)

.PHONY: clean

clean:
	rm $(SND) $(RECV) $(AGT)