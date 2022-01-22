#include "Server.h"
#include "Helper.h"
#include <exception>
#include <string>
#include <thread>
#include <fstream>

#define BUFFLEN 265
#define CODELEN 3
#define LENLEN 5
#define NAMELEN 2

Server::Server()
{
	_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

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
	while (true)
	{
		std::unique_lock<std::mutex> locker(_msgMtx);
		_condMsgQueue.wait(locker);

		std::fstream file;
		std::string fileName, data;
		data = _msgQueue.front();
		_msgQueue.pop();
		locker.unlock();

		int seperator = data.find('$');

		fileName = data.substr(0, seperator);
		std::cout << "seperatot: " << seperator << "file name : " << fileName << std::endl;
		data = data.substr(seperator + 1, data.length() - seperator - 1);

		file.open(fileName, std::fstream::app);

		if (!file.is_open())
		{
			std::cerr << "Could not open file in writing" << std::endl;
			exit(EXIT_FAILURE);
		}
		else
		{
			file << data;
			file.close();
		}
	}
}


void Server::acceptClient()
{
	// accepting
	SOCKET client_socket = accept(_serverSocket, NULL, NULL);
	if (client_socket == INVALID_SOCKET)
		throw std::exception(__FUNCTION__);

	std::cout << "Client accepted. Server and client can speak" << std::endl;
	// the function that handle the conversation with the client
	std::thread T(&Server::clientHandler, this, client_socket);
	T.detach();
}


void Server::clientHandler(SOCKET clientSocket)
{
	try
	{
		std::string name = firstMessage(clientSocket);
		int nameLen = name.length();
		std::string ret;
		int result = 1;
		char buff[BUFFLEN];

		while (true)
		{
			// getting msg from client
			recv(clientSocket, buff, BUFFLEN, 0);
			if (result == 0)
			{
				break;
			}
			//assembeling msg and len

			int name2Len = atoi(buff + CODELEN);
			std::string name2;
			for (int i = 0; i < name2Len; i++)
			{
				name2 += char(buff[LENLEN + i]);
			}

			std::string filePath = getFileName(name, name2);
			
			int msgLen = atoi(buff + LENLEN + name2Len);
			// if message has content
			if (msgLen != 0)
			{
				processMsg(msgLen, name2Len, name, buff, filePath);
			}

			//Sleep(200);
			std::string chat = name2Len != 0 ? getChatFromFile(filePath) : "";
			
			// creating return message
			ret = buildMessage(chat, nameLen, name);
			std::cout << name << ": " << ret << std::endl;
			Helper::sendData(clientSocket, ret);
		}

		_names.erase(name);
		std::cout << name << " has disconected" << std::endl;
	}
	catch (const std::exception& e)
	{
		closesocket(clientSocket);
	}
}

void Server::processMsg(int msgLen, int name2Len, std::string name, char* buff, std::string filePath)
{
	//process it
	std::string msg;
	for (int i = 0; i < msgLen; i++)
	{
		msg += char(buff[2 * LENLEN + name2Len + i]);
	}
	std::cout << msgLen << " : " << msg << std::endl;
	// constructing chat msg			
	msg = filePath + '$' + "&MAGSH_MESSAGE&&Author&" + name + "&DATA&" + msg;

	std::unique_lock<std::mutex> locker(_msgMtx);
	_msgQueue.push(msg);
	locker.unlock();
	_condMsgQueue.notify_one();
}

std::string Server::buildMessage(std::string chat, int nameLen, std::string name)
{
	std::string ret;
	std::string namesString = "";
	for (std::set <std::string>::iterator it = _names.begin(); it != _names.end(); ++it)
	{
		namesString += *it + "&";
	}
	namesString = namesString.substr(0, namesString.length() - 1);

	ret = "101" + Helper::getPaddedNumber(chat.length(), LENLEN) + chat;
	if (_names.size() != 1)
	{
		ret += Helper::getPaddedNumber(nameLen, NAMELEN) + name;
	}
	else
	{
		ret += "00";
	}
	ret += Helper::getPaddedNumber(namesString.length(), LENLEN);
	ret += namesString;
	return ret;
}

std::string Server::firstMessage(SOCKET soc)
{
	char buff[BUFFLEN];
	int result = recv(soc, buff, BUFFLEN, 0);

	// getting name and name len
	int nameLen = atoi(buff + CODELEN);
	std::string name;
	for (int i = 0; i < nameLen; i++)
	{
		name += char(buff[LENLEN + i]);
	}
	_names.insert(name);

	// sending back a message
	std::string ret = "1010000000000" + Helper::getPaddedNumber(nameLen, NAMELEN) + name;
	std::cout << ret << std::endl;
	Helper::sendData(soc, ret);
	
	return name;
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
		std::cout << "file wont open - " << fileName;
		return "";
	}

	return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}
