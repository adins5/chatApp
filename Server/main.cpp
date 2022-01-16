//#include "Server.h"
#include <queue> 
#include <fstream>
#include <string>
#include <iostream>
#include<stdio.h>
#include "Helper.h"
#include "WSAInitializer.h"
#include "Server.h"

#define CONFIG_PATH "./config.txt"

void getConfiguration();

std::queue <std::string> msgQueue;
std::string* ip;
int* port;


int main()
{
	getConfiguration();


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

void connector()
{
	WSAInitializer wsaInit;
	Server myServer;

	while (true)
	{
		try
		{
			myServer.serve(*port);
		}
		catch (std::exception& e)
		{
			std::cerr << "Error occured: " << e.what() << std::endl;
		}
		system("PAUSE");
	}


}