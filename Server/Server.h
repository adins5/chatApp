#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <thread>
#include <condition_variable>
#include <queue> 
#include <set>

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
	std::mutex _msgMtx;
	std::condition_variable _condMsgQueue;
	std::queue <std::string> _msgQueue;
	std::set <std::string> _names;

};

