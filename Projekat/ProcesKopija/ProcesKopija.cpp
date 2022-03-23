#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <conio.h>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <combaseapi.h>
#include "..\Common\ListOfData.h"

#define DEFAULT_BUFLEN 512
#define PORT_RK 25002

bool InitWindowsSocketsLibrary();
void RegisterProcess(SOCKET connectSocket, int i);
void SendData(SOCKET connectSocket, char* i);
void SendAllData(SOCKET connectSocket, NODE_PROCESS** head);

DWORD WINAPI handleIncomingData(LPVOID lpParam);

NODE_PROCESS* headProcess;

char messageBuffer[DEFAULT_BUFLEN];
char message[DEFAULT_BUFLEN] = "";
bool restore = false;

int main(int argc, char* argv[])
{
	InitProcessList(&headProcess);

	NODE_PROCESS* head;
	InitProcessList(&head);

	int serverPort = PORT_RK;

	if (argc > 1)
	{
		char* arg = (char*)argv[1];
		if (strcmp("25002", arg) == 0)
		{
			serverPort = PORT_RK;
		}
	}
	
	
	// socket used to communicate with server
	SOCKET connectSocket = INVALID_SOCKET;
	// variable used to store function return value
	int iResult;

	if (InitWindowsSocketsLibrary() == false)
	{
		return 1;
	}

	// create a socket
	connectSocket = socket(AF_INET,
		SOCK_STREAM,
		IPPROTO_TCP);

	if (connectSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	// create and initialize address structure
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_port = htons(serverPort);
	// connect to server specified in serverAddress and socket connectSocket
	if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		printf("Unable to connect to server.\n");
		closesocket(connectSocket);
		WSACleanup();
	}
	else
	{
		printf("Connected to ReplikatorKopija!\n");
	}



	unsigned long mode = 1; //non-blocking mode
	iResult = ioctlsocket(connectSocket, FIONBIO, &mode);
	if (iResult != NO_ERROR)
		printf("ioctlsocket failed with error: %ld\n", iResult);

	fd_set readfds;
	FD_ZERO(&readfds);

	DWORD funId;
	HANDLE handle;

	handle = CreateThread(NULL, 0, &handleIncomingData, &connectSocket, 0, &funId);

	while (true)
	{/*
		if (!restore) {
			puts("__________________________________________________________________________________");
			puts("0. Exit.");
			puts("1. Restore Data.");
		 }
		char line[256];
		int i;
		if (fgets(line, sizeof(line), stdin))
		{
			if (1 == sscanf(line, "%d", &i))
			{
				// Now 'i' can be safely used 
				if (i == 0)
				{
					puts("Client is shuting down...");
					break;
				}
				else if (i == 1)
				{
					//DATA temp;
					SendAllData(connectSocket,&headProcess);
					//message == temp.data;
					//strcpy(&message[1],temp.data);
					restore = true;
				}
				else
				{
					printf("Invalid input.\n");
				}
			}
			
		}*/
		 if(restore)
		 {
			SendAllData(connectSocket,&headProcess);
			restore = false;
		 }
		else 
		{
			
		}
	}

	// cleanup
	closesocket(connectSocket);
	WSACleanup();

	return 0;
}

bool InitWindowsSocketsLibrary()
{
	WSADATA wsaData;
	// Initialize windows sockets library for this process
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

void SendData(SOCKET connectSocket, char* i)
{
	
	int iResult = send(connectSocket, i, (int)strlen(i), 0);

	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return;
	}
}

DWORD WINAPI handleIncomingData(LPVOID lpParam)
{
	SOCKET* connectSocket = (SOCKET*)lpParam;

	char messageBuffer[DEFAULT_BUFLEN];
	int iResult;

	while (true)
	{
		fd_set readfds;
		FD_ZERO(&readfds);

		FD_SET(*connectSocket, &readfds);																																																																																	     
		timeval timeVal;
		timeVal.tv_sec = 2;
		timeVal.tv_usec = 0;
		int result = select(0, &readfds, NULL, NULL, &timeVal);

		if (result == 0)
		{
			// vreme za cekanje je isteklo
		}
		else if (result == SOCKET_ERROR)
		{
			//desila se greska prilikom poziva funkcije
		}
		else if (FD_ISSET(*connectSocket, &readfds))
		{
			// rezultat je jednak broju soketa koji su zadovoljili uslov
			iResult = recv(*connectSocket, messageBuffer, DEFAULT_BUFLEN, 0);
			if (iResult > 0)
			{
				if (messageBuffer[0] == '2')
					printf("Restored unsuccessfully.\n");
				else if (messageBuffer[0] == '3')
					printf("Restored successfully.\n");
				else if (messageBuffer[0] == '4')
				{

					DATA data = InitData(&messageBuffer[1]);
					PushProcess(&headProcess, data);

					PrintAllData(&headProcess);
				}			
				else if (messageBuffer[0] == '5') {
					printf("Your copy has stopped working. Please stop sending messages and close connection.\n");
					break;
				}
				else if (messageBuffer[0] == '8')
				{
					restore = true;
					printf("Restore Data.\n");
				}
			}
			else if (iResult == 0)
			{
				// connection was closed gracefully
				printf("Connection with Replicator closed.\n");
				closesocket(*connectSocket);
			}
			else
			{
				// there was an error during recv
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(*connectSocket);
			}
		}
		FD_CLR(*connectSocket, &readfds);
	}

	return 0;
}


void SendAllData(SOCKET connectSocket,NODE_PROCESS** head)
{
	DATA* temp;

	char message[DEFAULT_BUFLEN];
	NODE_PROCESS* tempNode = *head;
	int i = 1;
	message[0] = '2';
	while (tempNode != NULL)
	{
		temp = &tempNode->data;
		strcpy(&message[i],temp->data);
		tempNode = tempNode->next;
		i+=strlen(temp->data);
		strcpy(&message[i], "\n");
		i += 1;
	}
	SendData(connectSocket, message);
}