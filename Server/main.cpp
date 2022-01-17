//#include "Server.h"
#include <fstream>
#include <string>
#include <iostream>
#include <stdio.h>
#include "Helper.h"
#include "WSAInitializer.h"
#include "Server.h"


#define CONFIG_PATH "./config.txt"

void getConfiguration();
int connector();


std::string* ip;
int* port;


int main()
{
	getConfiguration();


	std::thread T1(connector);
	T1.detach();



	Sleep(10000000);

}

void getConfiguration()
{
	std::string msg;
	std::string p2;
	std::ifstream myfile(CONFIG_PATH);
	if (myfile.is_open())
	{
		std::getline(myfile, msg);
		int start = msg.find('=');
		int end = msg.find('\n');
		msg = msg.substr(start + 1, end - start - 1);
		ip = new std::string (msg);
		
		std::getline(myfile, p2);
		start = p2.find('=');
		p2 = p2.substr(start + 1, start + 4);
		int num = std::stoi(p2);
		port = new int (num);

		myfile.close();
	}
}

int connector()
{
	try
	{
		WSAInitializer wsaInit;
		Server myServer;
		std::thread T(&Server::queueToFileHandler, &myServer);
		T.detach();
		while (true)
		{
			myServer.serve(*port);
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Error occured: " << e.what() << std::endl;
		return 1;
	}

}