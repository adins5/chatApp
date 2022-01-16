#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <thread>

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

