#define _CRT_SECURE_NO_WARNINGS

#include "ListOfData.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>

CRITICAL_SECTION csProcess;

void InitProcessList(NODE_PROCESS** head)
{
	InitializeCriticalSectionAndSpinCount(&csProcess, 0x80000400);

	EnterCriticalSection(&csProcess);
	*head = NULL;
	LeaveCriticalSection(&csProcess);
}

void PushProcess(NODE_PROCESS** head, DATA data)
{
	NODE_PROCESS* tempNode = *head;
	NODE_PROCESS* newNode = (NODE_PROCESS*)malloc(sizeof(NODE_PROCESS));
	newNode->data = data;
	newNode->next = NULL;

	if (tempNode == NULL) // dodajemo prvi element
	{
		EnterCriticalSection(&csProcess);
		*head = newNode;
		LeaveCriticalSection(&csProcess);
		return;
	}
	while (tempNode->next != NULL)	// dodajemo na kraj
	{
		tempNode = tempNode->next;
	}
	EnterCriticalSection(&csProcess);
	tempNode->next = newNode;
	LeaveCriticalSection(&csProcess);
}

DATA PopFront(NODE_PROCESS** head)
{
	NODE_PROCESS* tempNode = *head;
	DATA returnData = { {0} };

	if (tempNode == NULL) {
		printf("\nList is empty!\n");
		return returnData;
	}
	returnData = InitData(tempNode->data.data);

	EnterCriticalSection(&csProcess);
	*head = tempNode->next;
	free(tempNode);
	LeaveCriticalSection(&csProcess);

	return returnData;
}

void PrintAllData(NODE_PROCESS** head)
{
	NODE_PROCESS* tempNode = *head;

	printf("\nAll data:\n");
	while (tempNode != NULL)
	{
		printf("%s\n", tempNode->data);

		tempNode = tempNode->next;
	}
	printf("\n");
}

/*DATA SendAllData(NODE_PROCESS** head, char* messageBuff) 
{
	DATA temp = InitData(messageBuff);
	NODE_PROCESS* tempNode = *head;
	while (tempNode != NULL)
	{
		int i = 0;
		//temp->data = tempNode->data;
		i++;
		tempNode = tempNode->next;
	}
	return temp;
}
*/
DATA InitData(char* data)
{
	DATA d;
	strcpy(d.data, data);

	return d;
}