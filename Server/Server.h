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
	void queueToFileHandler();

private:

	void acceptClient();
	void clientHandler(SOCKET clientSocket);

	void processMsg(int msgLen, int name2Len, std::string name, char* buff, std::string filePath);
	std::string buildMessage(std::string chat, int nameLen, std::string name);
	std::string firstMessage(SOCKET soc);
	SOCKET _serverSocket;
	std::mutex _msgMtx;
	std::mutex _namesMtx;;
	std::condition_variable _condMsgQueue;
	std::queue <std::string> _msgQueue;
	std::set <std::string> _names;

};


std::string getFileName(std::string n1, std::string n2);
std::string getChatFromFile(std::string fileName);
