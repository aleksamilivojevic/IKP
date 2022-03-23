#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <string.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <winsock.h>
#include <tchar.h>
#include "..\Common\ListOfProces.h"
#include "..\Common\ListOfData.h"

#define DEFAULT_BUFLEN 512
#define PORT_RO "25001"
#define PORT_RK 25002
#define GUID_FORMAT "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX"
#define GUID_ARG(guid) guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]

bool InitWindowsSocketsLibrary();

DWORD WINAPI handleSocket(LPVOID lpParam);
DWORD WINAPI handleConnectSocket(LPVOID lpParam);
DWORD WINAPI handleData(LPVOID lpParam);

char* guidToString(const GUID* id, char* out);
GUID stringToGUID(const std::string& guid);

NODE_REPLICATOR* head;
NODE_PROCESS* headProcessReceive;
NODE_PROCESS* headProcessSend;
SOCKET replicatorSocket = INVALID_SOCKET;

int main()
{
	InitReplicatorList(&head);
	InitProcessList(&headProcessReceive);
	InitProcessList(&headProcessSend);

	// Socket used for listening for new clients 
	SOCKET listenSocket = INVALID_SOCKET;
	// Socket used for communication with client
	SOCKET acceptedSocket[10];
	for (int i = 0; i < 10; i++)
	{
		acceptedSocket[i] = INVALID_SOCKET;
	}

	// variable used to store function return value
	int iResult;
	// Buffer used for storing incoming data
	char recvbuf[DEFAULT_BUFLEN];
	char Id[3];

	if (InitWindowsSocketsLibrary() == false)
	{
		return 1;
	}

	// Prepare address information structures
	addrinfo* resultingAddress = NULL;
	addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;       // IPv4 address
	hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
	hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
	hints.ai_flags = AI_PASSIVE;     // 

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, PORT_RO, &hints, &resultingAddress);
	if (iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	listenSocket = socket(AF_INET,      // IPv4 address famly
		SOCK_STREAM,  // stream socket
		IPPROTO_TCP); // TCP

	if (listenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket - bind port number and local address 
	// to socket
	iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	// Since we don't need resultingAddress any more, free it
	freeaddrinfo(resultingAddress);

	// Set listenSocket in listening mode
	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}



	printf("Waiting connection with ReplikatorKopija...\n");
	int numberOfClients = 0;

	replicatorSocket = accept(listenSocket, NULL, NULL);

	if (replicatorSocket == INVALID_SOCKET)
	{
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	else
	{

		// socket used to communicate with server
		SOCKET connectSocket = INVALID_SOCKET;

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
		serverAddress.sin_port = htons(PORT_RK);
		// connect to server specified in serverAddress and socket connectSocket
		if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
		{
			printf("Unable to connect to server.\n");
			closesocket(connectSocket);
		}
		DWORD funId;

		CreateThread(NULL, 0, &handleConnectSocket, &connectSocket, 0, &funId);

		printf("Connection with ReplikatorKopija established.\n");
		printf("Server initialized, waiting for clients.\n");
		puts("__________________________________________________________________________________");
	}

	do
	{
		acceptedSocket[numberOfClients] = accept(listenSocket, NULL, NULL);

		if (acceptedSocket[numberOfClients] == INVALID_SOCKET)
		{
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}
		
		DWORD funId[10];
		HANDLE handle[10];

		PROCESS processAdd[10];
		GUID Id;
		CoCreateGuid(&Id);
		processAdd[numberOfClients] = InitProcess(Id, acceptedSocket[numberOfClients]);

		handle[numberOfClients] = CreateThread(NULL, 0, &handleSocket, &processAdd[numberOfClients], 0, &funId[numberOfClients]);
		CloseHandle(handle[numberOfClients]);

		numberOfClients++;

	} while (1);

	closesocket(listenSocket);

	for (int i = 0; i < 10; i++)
	{
		iResult = shutdown(acceptedSocket[i], SD_SEND);
		if (iResult == SOCKET_ERROR)
		{
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(acceptedSocket[i]);
			WSACleanup();
			return 1;
		}

		closesocket(acceptedSocket[i]);
	}
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



DWORD WINAPI handleSocket(LPVOID lpParam)
{
	PROCESS* process = (PROCESS*)lpParam;
	SOCKET acceptedSocket = process->acceptedSocket;
	GUID Id = process->processId;
	int iResult;
	char recvbuf[512];
	
	/* int id
	char idd[512];
	int id;
	iResult = recv(acceptedSocket, recvbuf, strlen(recvbuf), 0);
	if (iResult > 0) 
	{
		idd[0] = recvbuf[0];
	}
	id = atoi(&idd[0]);
	*/

	if (IsSocketNull(&head))
	{
		*process = InitProcess(Id, acceptedSocket);
		AddSocketToID(&head, &process);
	}

	unsigned long mode = 1; //non-blocking mode
	iResult = ioctlsocket(acceptedSocket, FIONBIO, &mode);
	if (iResult != NO_ERROR)
		printf("ioctlsocket failed with error: %ld\n", iResult);

	fd_set readfds;
	FD_ZERO(&readfds);

	do
	{
		fd_set readfds;
		FD_ZERO(&readfds);
		// Receive data until the client shuts down the connection
		FD_SET(acceptedSocket, &readfds);
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
		else if (FD_ISSET(acceptedSocket, &readfds))
		{
			// rezultat je jednak broju soketa koji su zadovoljili uslov
			iResult = recv(acceptedSocket, recvbuf, DEFAULT_BUFLEN, 0);
			if (iResult > 0)
			{
				if (recvbuf[0] == 1)    
				{
					if (PushBack(&head, *process))
					{
						puts("__________________________________________________________________________________");
						printf("New process added! ID: {" GUID_FORMAT "}\n", GUID_ARG(process->processId));
						strcpy(recvbuf, "1");

						char output[DEFAULT_BUFLEN];
						guidToString(&process->processId, output);
						iResult = send(replicatorSocket, output, strlen(output) + 1, 0);

						if (iResult == SOCKET_ERROR)
						{
							printf("send failed with error: %d\n", WSAGetLastError());
							closesocket(replicatorSocket);
							WSACleanup();
							return 1;
						}
					}
					else
					{
						puts("__________________________________________________________________________________");
						printf("Process: ID: {" GUID_FORMAT "} is already registered.\n", GUID_ARG(process->processId));
						strcpy(recvbuf, "0");
					}

					iResult = send(acceptedSocket, recvbuf, strlen(recvbuf) + 1, 0);

					if (iResult == SOCKET_ERROR)
					{
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(acceptedSocket);
						WSACleanup();
						return 1;
					}

					PrintAllProcesses(&head);
				}
				else if (recvbuf[0] == '2')  
				{
					if (ContainsProcess(&head, *process))
					{
						char temp[DEFAULT_BUFLEN];
						strcpy(temp, &recvbuf[1]);
						temp[iResult - 1] = '\0';

						guidToString(&process->processId, &recvbuf[1]);
						strcpy(&recvbuf[37], temp);

						DATA data = InitData(temp);

						PushProcess(&headProcessReceive, data);

						puts("__________________________________________________________________________________");
						printf("Data saved successfully for process: ID: {" GUID_FORMAT "}\n", GUID_ARG(process->processId));

						recvbuf[0] = '+';
						iResult = send(replicatorSocket, recvbuf, strlen(recvbuf) + 1, 0);

						if (iResult == SOCKET_ERROR)
						{
							printf("send failed with error: %d\n", WSAGetLastError());
							closesocket(replicatorSocket);
							WSACleanup();
							return 1;
						}

						strcpy(recvbuf, "3");
					}
					else
					{
						puts("__________________________________________________________________________________");
						printf("Process: ID: {" GUID_FORMAT "} is not registered!\n", GUID_ARG(process->processId));
						strcpy(recvbuf, "2");
					}

					iResult = send(acceptedSocket, recvbuf, strlen(recvbuf) + 1, 0);

					if (iResult == SOCKET_ERROR)
					{
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(acceptedSocket);
						WSACleanup();
						return 1;
					}
				}
				else if (recvbuf[0] == '*')
				{
					if (ContainsProcess(&head, *process))
					{
						guidToString(&process->processId, &recvbuf[1]);			

						puts("__________________________________________________________________________________");
						printf("Restore data request sent successfully for process: ID: {" GUID_FORMAT "}\n", GUID_ARG(process->processId));

						iResult = send(replicatorSocket, recvbuf, strlen(recvbuf) + 1, 0);

						if (iResult == SOCKET_ERROR)
						{
							printf("send failed with error: %d\n", WSAGetLastError());
							closesocket(replicatorSocket);
							WSACleanup();
							return 1;
						}

					}
				}
			}
			else if (iResult == 0)
			{
				guidToString(&process->processId, &recvbuf[1]);

				// connection was closed gracefully
				puts("__________________________________________________________________________________");
				printf("Connection with process(ID: {" GUID_FORMAT "}) closed.\n", GUID_ARG(process->processId));
				closesocket(acceptedSocket);

				//Potrebno je javiti drugoj strani da je doslo do prekida konekcije
				recvbuf[0] = 'x';
				iResult = send(replicatorSocket, recvbuf, strlen(recvbuf) + 1, 0);

				if (iResult == SOCKET_ERROR)
				{
					printf("send failed with error: %d\n", WSAGetLastError());
					closesocket(replicatorSocket);
					WSACleanup();
					return 1;
				}
				break;
			}
			else
			{
				// there was an error during recv
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(acceptedSocket);
			}
		}
		FD_CLR(acceptedSocket, &readfds);

	} while (true);

	return 0;
}

DWORD WINAPI handleConnectSocket(LPVOID lpParam)
{
	int iResult;
	char recvbuf[DEFAULT_BUFLEN];
	SOCKET* acceptedSocket = (SOCKET*)lpParam;

	fd_set readfds;
	FD_ZERO(&readfds);

	do
	{
		FD_SET(*acceptedSocket, &readfds);
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
		else if (FD_ISSET(*acceptedSocket, &readfds))
		{
			// rezultat je jednak broju soketa koji su zadovoljili uslov
			iResult = recv(*acceptedSocket, recvbuf, DEFAULT_BUFLEN, 0);
			if (iResult > 0)
			{
				if (recvbuf[0] == '+')
				{
					GUID guid = stringToGUID(&recvbuf[1]);

					PROCESS processInfo = InitProcess(guid, NULL); // lose resenje
					PROCESS* process = &processInfo;

					DATA data;                           // lose resenje
					strcpy(data.data, &recvbuf[37]);
					FindProcess(&head, &process, guid);

					PushProcess(&headProcessSend, data);

					DWORD funId;
					HANDLE handle;

					CreateThread(NULL, 0, &handleData, &processInfo, 0, &funId);

					puts("__________________________________________________________________________________");
					printf("Messages received from ReplikatorKopija: %s.\n", &recvbuf[37]);
				}
				
				else if (recvbuf[0] == 'x')
				{
					puts("__________________________________________________________________________________");
					printf("ReplikatorKopija closed connection with process: %s.\n", &recvbuf[1]);
				}
				
				
			}
			else if (iResult == 0)
			{
				// connection was closed gracefully
				puts("__________________________________________________________________________________");
				printf("Connection with ReplikatorKopija closed.\n");
				closesocket(*acceptedSocket);
			}
			else
			{
				// there was an error during recv
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(*acceptedSocket);
			}
		}
		FD_CLR(*acceptedSocket, &readfds);

	} while (true);

	return 0;
}

DWORD WINAPI handleData(LPVOID lpParam)
{
	PROCESS* process = (PROCESS*)lpParam;
	SOCKET acceptedSocket = process->acceptedSocket;
	GUID Id = process->processId;

	int iResult;
	char recvbuf[DEFAULT_BUFLEN];

	DATA returnData = PopFront(&headProcessSend);

	if (returnData.data != NULL)
	{
		strcpy(&recvbuf[0], "4");
		strcpy(&recvbuf[1], returnData.data);

		iResult = send(acceptedSocket, recvbuf, strlen(recvbuf) + 1, 0);

		if (iResult == SOCKET_ERROR)
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(acceptedSocket);
			WSACleanup();
			return 1;
		}
	}
	return 0;
}

char* guidToString(const GUID* id, char* out) {
	int i;
	char* ret = out;
	out += sprintf(out, "%.8lX-%.4hX-%.4hX-", id->Data1, id->Data2, id->Data3);
	for (i = 0; i < sizeof(id->Data4); ++i) {
		out += sprintf(out, "%.2hhX", id->Data4[i]);
		if (i == 1) *(out++) = '-';
	}
	return ret;
}

GUID stringToGUID(const std::string& guid) {
	GUID output;
	const auto ret = sscanf(guid.c_str(), "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX", &output.Data1, &output.Data2, &output.Data3, &output.Data4[0], &output.Data4[1], &output.Data4[2], &output.Data4[3], &output.Data4[4], &output.Data4[5], &output.Data4[6], &output.Data4[7]);
	if (ret != 11)
		throw std::logic_error("Invalid GUID, format should be {00000000-0000-0000-0000-000000000000}");
	return output;
}
