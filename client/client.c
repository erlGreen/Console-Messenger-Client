#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "55555"
#define IP "192.168.100.8"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

SOCKET ConnectSocket;
HANDLE watki[3];
int forcedBuffer = 0;
int finish = 0;
CRITICAL_SECTION ghMutex;
int priorytety[3] = { THREAD_PRIORITY_BELOW_NORMAL ,
THREAD_PRIORITY_NORMAL , THREAD_PRIORITY_ABOVE_NORMAL
};

void gotoxy(int x, int y)
{
	COORD c = { x, y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

DWORD WINAPI GetMessages(void* argumenty)
{
	char recvbuf[512];

	char tempbuffer[2048] = "";
	int currentLine = 5;
	int next_lines = 0;
	int result;
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	char c;
	DWORD dwTmp;
	INPUT_RECORD ir[2];

	ir[0].EventType = KEY_EVENT;
	ir[0].Event.KeyEvent.bKeyDown = TRUE;
	ir[0].Event.KeyEvent.dwControlKeyState = 0;
	ir[0].Event.KeyEvent.uChar.UnicodeChar = '\0';
	ir[0].Event.KeyEvent.wRepeatCount = 1;
	ir[0].Event.KeyEvent.wVirtualKeyCode = '\0';
	ir[0].Event.KeyEvent.wVirtualScanCode = MapVirtualKey('Q', MAPVK_VK_TO_VSC);

	ir[1].EventType = KEY_EVENT;
	ir[1].Event.KeyEvent.bKeyDown = TRUE;
	ir[1].Event.KeyEvent.dwControlKeyState = 0;
	ir[1].Event.KeyEvent.uChar.UnicodeChar = VK_RETURN;
	ir[1].Event.KeyEvent.wRepeatCount = 1;
	ir[1].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
	ir[1].Event.KeyEvent.wVirtualScanCode = MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC);
	while (1) {
		result = recv(ConnectSocket, recvbuf, 512, 0);
		if (result == 0)
			break;
		if (TryEnterCriticalSection(&ghMutex)) {
			recvbuf[strlen(recvbuf)] = '\0';
			gotoxy(0, currentLine);
			currentLine += next_lines + 1;
			printf("%s", tempbuffer);
			printf("%s", recvbuf);
			printf("\n");
			LeaveCriticalSection(&ghMutex);
			memset(tempbuffer, 0, strlen(tempbuffer));
			next_lines = 0;
		}
		else {
			if (strlen(tempbuffer) + strlen(recvbuf) > 2048) {
				forcedBuffer = 1;
				WriteConsoleInput(hStdin, ir, 2, &dwTmp);
			}
			next_lines++;
			strncpy(tempbuffer + strlen(tempbuffer), recvbuf, strlen(recvbuf) + 1);
		}
	}
	finish = 1;
	return 0;
}
int main(int argc, char** argv)
{
	WSADATA wsaData;
	DWORD id;
	HANDLE dane;
	ConnectSocket = INVALID_SOCKET;
	struct addrinfo* result = NULL, * ptr = NULL, hints;
	char sendbuf[256];
	int infos = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (infos != 0)
		return 1;
	InitializeCriticalSection(&ghMutex);
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	infos = getaddrinfo(IP, DEFAULT_PORT, &hints, &result);
	if (infos != 0)
	{
		printf("Disconnected\n");
		WSACleanup();
		return 1;
	}
	printf("connected");
	ptr = result;
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	if (ConnectSocket == INVALID_SOCKET)
	{
		printf("Disconnected\n");
		WSACleanup();
		return 1;
	}

	infos = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (infos == SOCKET_ERROR)
	{
		printf("Disconnected\n");
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
		return 1;
	}

	dane = CreateThread(NULL, 0, GetMessages, NULL, 0, &id);
	if (dane != INVALID_HANDLE_VALUE)
	{
		SetThreadPriority(dane, THREAD_PRIORITY_ABOVE_NORMAL);
	}

	char recvbuf[512];
	EnterCriticalSection(&ghMutex);
	scanf("%s", &sendbuf);
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
	gotoxy(0, 0);
	printf("Wcisnij enter aby zaczac pisac wiadomosc oraz enter by ja wyslac.");
	LeaveCriticalSection(&ghMutex);
	infos = send(ConnectSocket, sendbuf, (int)strlen(sendbuf) + 1, 0);
	while (1)
	{
		c = getchar();
		if (finish)
			break;
		if (c == '\n')
		{
			EnterCriticalSection(&ghMutex);
			gotoxy(0, 1);
			for (int i = 0; i < strlen(sendbuf); i++)
				printf(" ");
			gotoxy(0, 1);
			fgets(sendbuf, 512, stdin);
			LeaveCriticalSection(&ghMutex);
			if (finish)
				break;
			if (sendbuf[0] != '\n' && forcedBuffer == 0)
				send(ConnectSocket, sendbuf, (int)strlen(sendbuf) + 1, 0);
			forcedBuffer = 0;
		}
	}
	closesocket(ConnectSocket);
	WSACleanup();
	return 0;
}
