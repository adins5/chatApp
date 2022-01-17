#include "Server.h"
#include "Helper.h"
#include <exception>
#include <string>
#include <thread>

Server::Server()
{

	// this server use TCP. that why SOCK_STREAM & IPPROTO_TCP
	// if the server use UDP we will use: SOCK_DGRAM & IPPROTO_UDP
	_serverSocket = socket(AF_INET,  SOCK_STREAM,  IPPROTO_TCP); 

	if (_serverSocket == INVALID_SOCKET)
		throw std::exception(__FUNCTION__ " - socket");
}

Server::~Server()
{
	try
	{
		// the only use of the destructor should be for freeing 
		// resources that was allocated in the constructor
		closesocket(_serverSocket);
	}
	catch (...) {}
}

void Server::serve(int port)
{
	
	struct sockaddr_in sa = { 0 };
	
	sa.sin_port = htons(port); // port that server will listen for
	sa.sin_family = AF_INET;   // must be AF_INET
	sa.sin_addr.s_addr = INADDR_ANY;    // when there are few ip's for the machine. We will use always "INADDR_ANY"

	// Connects between the socket and the configuration (port and etc..)
	if (bind(_serverSocket, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ " - bind");
	
	// Start listening for incoming requests of clients
	if (listen(_serverSocket, SOMAXCONN) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ " - listen");
	std::cout << "Listening on port " << port << std::endl;

	while (true)
	{
		// the main thread is only accepting clients 
		// and add then to the list of handlers
		std::cout << "Waiting for client connection request" << std::endl;
		acceptClient();
	}
}


void Server::acceptClient()
{

	// this accepts the client and create a specific socket from server to this client
	// the process will not continue until a client connects to the server
	SOCKET client_socket = accept(_serverSocket, NULL, NULL);
	if (client_socket == INVALID_SOCKET)
		throw std::exception(__FUNCTION__);

	std::cout << "Client accepted. Server and client can speak" << std::endl;
	// the function that handle the conversation with the client
	std::thread T (&Server::clientHandler, this, client_socket);
	T.detach();
}


void Server::clientHandler(SOCKET clientSocket)
{
	std::string chat = "";
	char buff[265];
	recv(clientSocket, buff, 265, 0);
	
	int msgType = atoi(buff + 2);
	int nameLen = atoi(buff + 3);

	std::string name;
	for (int i = 0; i < nameLen; i++)
	{
		name += char(buff[5 + i]);
	}
	_names.insert(name);

	std::string ret = "1010000000000" + Helper::getPaddedNumber(nameLen, 2) + name;
	std::cout << ret << std::endl;
	Helper::sendData(clientSocket, ret);
	
	try
	{
		while (true)
		{
			char buff[265];
			recv(clientSocket, buff, 265, 0);
			std::cout << buff << std::endl;
			int msgLen = atoi(buff + 5 + nameLen);
			
			std::string namesString = "";
			for (std::set <std::string>::iterator it = _names.begin(); it != _names.end(); ++it)
			{
				namesString += *it + "&";
			}
			namesString = namesString.substr(0, namesString.length() - 1);

			ret = "101" + Helper::getPaddedNumber(chat.length(), 5) + chat;
			if (_names.size() != 1)
			{
				ret += Helper::getPaddedNumber(nameLen, 2) + name;
			}
			else
			{
				ret += "00";
			}
			ret += Helper::getPaddedNumber(namesString.length(), 5);
			ret += namesString;

			std::cout << ret << std::endl;
			Helper::sendData(clientSocket, ret);
			
			/*_msgMtx.lock();
			_msgQueue.push(buff);
			_msgMtx.unlock();*/
			/*int chatLen = Helper::getIntPartFromSocket(clientSocket, 8) - 10100000;
			std::cout << chatLen;
			
			
			std::string s = "101";

			send(clientSocket, s.c_str(), s.size(), 0);*/

		}
	}
	catch (const std::exception& e)
	{
		closesocket(clientSocket);
	}
}

