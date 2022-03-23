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
#define PORT_RO 25001

bool InitWindowsSocketsLibrary();
void RegisterProcess(SOCKET connectSocket, int i);
void SendData(SOCKET connectSocket, char* i);
void ReceiveData(SOCKET connectSocket, char* i);
DWORD WINAPI handleIncomingData(LPVOID lpParam);
NODE_PROCESS* headProcess;

int main(int argc, char* argv[]) 
{
	InitProcessList(&headProcess);

	char messageBuffer[DEFAULT_BUFLEN];
	char message[DEFAULT_BUFLEN];
	//char Id[3];
	bool dataSent = false;
	//bool choosed = false;

	// socket used to communicate with server
	SOCKET connectSocket = INVALID_SOCKET;
	// variable used to store function return value
	int iResult;

	if (InitWindowsSocketsLibrary() == false)
	{
		// we won't log anything since it will be logged
		// by InitializeWindowsSockets() function
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
	serverAddress.sin_port = htons(PORT_RO);
	// connect to server specified in serverAddress and socket connectSocket
	if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		printf("Unable to connect to server.\n");
		closesocket(connectSocket);
		WSACleanup();
	}
	else
	{
		printf("Connected to ReplikatorOriginal!\n");
	}

	/*int id
	while(!choosed)
	{

		printf("Choose your ID: \n");
		scanf("%s", Id);
		iResult = send(connectSocket, Id, strlen(Id), 0);
	 	choosed = true;
		
	}*/
	
	
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
	{
		if (!dataSent) {
			puts("__________________________________________________________________________________\nMAIN MENU: \n0. Exit.\n1. Register.\n2. Backup data.\n3. Restore data.\n__________________________________________________________________________________\n");
		}

		char line[256];
		int i;
		if (fgets(line, sizeof(line), stdin))
		{

			if (1 == sscanf(line, "%d", &i))
			{
				if (i == 0)
				{
					puts("Client is shuting down...");
					break;
				}
				else if (i == 1)
				{
					
					RegisterProcess(connectSocket, i);
								
				}
				else if (i == 2)
				{
					message[0] = '2';
					printf("Input text to store: ");
					scanf("%s", &message[1]);
					SendData(connectSocket, message);
					dataSent = true;
				}
				else if (i == 3)
				{
					message[0] = '*';
					ReceiveData(connectSocket, message);
				}
				else
				{
					printf("Invalid input.\n");
				}
			}
			else
			{
				if (dataSent)
				{
					dataSent = false;
				}
				else
				{
					printf("Invalid input.\n");
				}
			}
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

void RegisterProcess(SOCKET connectSocket, int i)
{
	
	// Send an prepared message with null terminator included
	int iResult = send(connectSocket, (char*)&i, sizeof(i), 0);

	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return;
	}

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

void ReceiveData(SOCKET connectSocket, char* i)
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

	int iResult;
	char messageBuffer[DEFAULT_BUFLEN];

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
				if (messageBuffer[0] == '0')
					printf("This process is registered already.\n");
				else if (messageBuffer[0] == '1')
					printf("Registered successfully.\n");
				else if (messageBuffer[0] == '2')
					printf("Data wasn't stored successfully.\n");
				else if (messageBuffer[0] == '3')
					printf("Data stored successfully.\n");
				else if (messageBuffer[0] == '4')
				{
					//DATA data = InitData(&messageBuffer[1]);
					//PushProcess(&headProcess, data);
					printf("Restored data:\n");
					printf("%s", &messageBuffer[1]);
					//PrintAllData(&headProcess);
				}
				else if (messageBuffer[0] == '5') {
					printf("Your copy has stopped working. Please stop sending messages and close connection.\n");
					break;
				}
			}
			else if (iResult == 0)
			{
				// connection was closed gracefully
				printf("Connection with ReplikatorOriginal closed.\n");
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

