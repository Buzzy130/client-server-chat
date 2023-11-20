#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream> 
#include <cstdio> 
#include <cstring> 
#include <winsock2.h> 
#include <vector>
#pragma comment(lib, "WS2_32.lib")
using namespace std;


struct ClientData {
	SOCKET socket; // Сокет клиента
	SOCKADDR_IN address; // Адрес клиента
	string username; // Имя пользователя
};

vector<ClientData> clients;

DWORD WINAPI clientThread(LPVOID lpParam) {
	ClientData* clientData = (ClientData*)lpParam;
	SOCKET clientSocket = clientData->socket;
	SOCKADDR_IN clientAddress = clientData->address;
	char buffer[1024] = { 0 };

	while (true) {
		if (recv(clientSocket, buffer, sizeof(buffer), 0) == SOCKET_ERROR) {
			cout << "recv function failed with error " << WSAGetLastError() << endl;
			break;
		}
		if (strcmp(buffer, "exit\n") == 0) {
			cout << "Client " << inet_ntoa(clientAddress.sin_addr) << ":" << ntohs(clientAddress.sin_port) << " disconnected." << endl;
			break;
		}

		cout << clientData->username << " (" << inet_ntoa(clientAddress.sin_addr) << ":" << ntohs(clientAddress.sin_port) << "): " << buffer << endl;

		// Отправка сообщения клиентам
		string message = clientData->username + ": " + buffer;
		for (int i = 0; i < clients.size(); i++) {
			if (clients[i].socket != clientSocket) {
				if (send(clients[i].socket, message.c_str(), message.length(), 0) == SOCKET_ERROR) {
					cout << "send failed with error " << WSAGetLastError() << endl;
					break;
				}
			}
		}

		memset(buffer, 0, sizeof(buffer));
	}

	// Закрытие сокета клиента
	closesocket(clientSocket);
	delete clientData;
	return 0;
}



int main() {
	WSADATA WSAData;
	SOCKET serverSocket, clientSocket;
	SOCKADDR_IN serverAddress, clientAddress;
	WSAStartup(MAKEWORD(2, 0), &WSAData);
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (serverSocket == INVALID_SOCKET) {
		cout << "Socket creation failed with error: " << WSAGetLastError() << endl;
		return -1;
	}

	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(2007);

	if (bind(serverSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
		cout << "Bind function failed with error: " << WSAGetLastError() << endl;
		return -1;
	}

	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
		cout << "Listen function failed with error: " << WSAGetLastError() << endl;
		return -1;
	}

	cout << "Listening for incoming connections...." << endl;

	while (true) {
		int clientAddressSize = sizeof(clientAddress);
		clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddress, &clientAddressSize);

		if (clientSocket != INVALID_SOCKET) {
			//проверка количества клиентов
			if (clients.size() >= 10) {
				cout << "Connection limit reached. Rejecting incoming connection." << endl;
				closesocket(clientSocket);
				continue;
			}
			ClientData* clientData = new ClientData;
			clientData->socket = clientSocket;
			clientData->address = clientAddress;

			// Получение имени пользователя от клиента
			char usernameBuffer[1024] = { 0 };
			if (recv(clientSocket, usernameBuffer, sizeof(usernameBuffer), 0) == SOCKET_ERROR) {
				cout << "recv function failed with error " << WSAGetLastError() << endl;
				closesocket(clientSocket);
				delete clientData;
				continue;
			}
			clientData->username = usernameBuffer;

			clients.push_back(*clientData);

			cout << "Client " << clientData->username << " connected from " << inet_ntoa(clientAddress.sin_addr) << ":" << ntohs(clientAddress.sin_port) << endl;

			DWORD threadId;
			HANDLE threadHandle = CreateThread(NULL, 0, clientThread, (LPVOID)clientData, 0, &threadId);

			if (threadHandle == NULL) {
				cout << "Thread Creation Error: " << WSAGetLastError() << endl;
			}
		}
	}

	closesocket(serverSocket);
	WSACleanup();
	return 0;
}