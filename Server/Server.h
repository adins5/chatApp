#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <thread>
#include <condition_variable>
#include <queue> 

std::mutex msgMtx;
std::condition_variable condMsgQueue;
std::queue <std::string> msgQueue;

class Server
{
public:
	Server();
	~Server();
	void serve(int port);


private:

	void acceptClient();
	void clientHandler(SOCKET clientSocket);

	SOCKET _serverSocket;
};

