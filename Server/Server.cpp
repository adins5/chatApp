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
		std::string namesString = "", chat = "", name2 = "", filePath = "";
		int nameLen = name.length(), result = 1;
		char buff[BUFFLEN];
		std::unique_lock<std::mutex> locker(_namesMtx);
		locker.unlock();
		while (true)
		{
			// getting msg from client
			result = recv(clientSocket, buff, BUFFLEN, 0);
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
				processMsg(msgLen, name2Len, name, buff, filePath);

			Sleep(200);
			std::string chat = name2Len != 0 ? getChatFromFile(filePath) : "";
			std::string namesString = "";
			locker.lock();
			for (std::set <std::string>::iterator it = _names.begin(); it != _names.end(); ++it)
			{
				namesString += *it + "&";
			}
			locker.unlock();
			namesString = namesString.substr(0, namesString.length() - 1);
			Helper::send_update_message_to_client(clientSocket, chat, name2, namesString);
		}
		
		locker.lock();
		_names.erase(name);
		locker.unlock();
		std::cout << name << " has disconected" << std::endl;
	}
	catch (const std::exception& e)
	{
		closesocket(clientSocket);
	}
}

// function will extract the message and build it in the file pattern
void Server::processMsg(int msgLen, int name2Len, std::string name, char* buff, std::string filePath)
{
	//process it
	std::string msg;
	for (int i = 0; i < msgLen; i++)
	{
		msg += char(buff[2 * LENLEN + name2Len + i]);
	}
	// constructing chat msg			
	msg = filePath + '$' + "&MAGSH_MESSAGE&&Author&" + name + "&DATA&" + msg;

	std::unique_lock<std::mutex> locker(_msgMtx);
	_msgQueue.push(msg);
	locker.unlock();
	_condMsgQueue.notify_one();
}

// function will get the first message
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
	std::unique_lock<std::mutex> locker(_namesMtx);
	_names.insert(name);
	locker.unlock();
	// sending back a message
	Helper::send_update_message_to_client(soc, "", "", name);
	
	return name;
}

//function will create the file name
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

// function will retreave chat from file
std::string getChatFromFile(std::string fileName)
{
	std::ifstream file(fileName); //taking file as inputstream
	if (!file.is_open())
	{
		return "";
	}
	return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}
