#include "Server.h"
#include "Helper.h"
#include <exception>
#include <string>
#include <thread>
#include <fstream>

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

void Server::queueToFileHandler()
{
	std::unique_lock<std::mutex> locker(_msgMtx);
	_condMsgQueue.wait(locker);

	std::fstream file;
	std::string fileName, data;
	data = _msgQueue.front();
	_msgQueue.pop();
	
	int seperator = data.find('$');

	fileName = data.substr(seperator + 1, data.length() - seperator - 1);
	data = data.substr(0, seperator - 1);

	file.open(fileName, std::fstream::app);

	if (file.is_open())
	{
		file << data;
		file.close();
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
	try
	{
		// getting msg from client
		char buff[265];
		recv(clientSocket, buff, 265, 0);
		
		int msgType = atoi(buff + 2);
		// getting name and name len
		int nameLen = atoi(buff + 3);
		std::string name;
		for (int i = 0; i < nameLen; i++)
		{
			name += char(buff[5 + i]);
		}
		_names.insert(name);

		// sending back a message
		std::string ret = "1010000000000" + Helper::getPaddedNumber(nameLen, 2) + name;
		std::cout << ret << std::endl;
		Helper::sendData(clientSocket, ret);

		while (true)
		{
			// getting msg from client
			char buff[265];
			recv(clientSocket, buff, 265, 0);
			//assembeling msg and len
			
			int name2Len = atoi(buff + 3);
			std::string name2;
			for (int i = 0; i < name2Len; i++)
			{
				name2 += char(buff[5 + i]);
			}

			std::string chat = "";

			int msgLen = atoi(buff + 5 + name2Len);
			std::string filePath = getFileName(name, name2);
			if (name2Len == 0)
			{
				ret = "1010000000000" + nameLen + name;
			}
			else if (msgLen != 0)
			{
				std::string msg;
				for (int i = 0; i < msgLen; i++)
				{
					msg += char(buff[5 + name2Len + i]);
				}

				// constructing chat msg			
				msg = '$' + filePath + "&MAGSH_MESSAGE&&Author&" + name + "&DATA&" + msg;
				
				std::unique_lock<std::mutex> locker(_msgMtx);
				_msgQueue.push(msg);
				locker.unlock();
				_condMsgQueue.notify_one();

				chat = getChatFromFile(filePath);

			}
			Sleep(200);

			
			// creating return message
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
		}
		std::cout << ret << std::endl;
		Helper::sendData(clientSocket, ret);
		
	}
	catch (const std::exception& e)
	{
		closesocket(clientSocket);
	}
}

std::string getFileName(std::string n1, std::string n2)
{
	int compare = n1.compare(n2);
	std::string ret = "";
	if (compare < 0) {
		ret = n1 + '&' + n2 + ".txt";
	}
	else {
		ret = n2 + '&' + n1 + ".txt";
	}
	return ret;
}

std::string getChatFromFile(std::string fileName)
{
	std::ifstream file(fileName); //taking file as inputstream
	if (!file.is_open()) 
	{
		std::cerr << "Could not open "<< fileName <<std::endl;
		exit(EXIT_FAILURE);
	}

	return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}
