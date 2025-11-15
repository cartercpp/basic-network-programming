#include <iostream>
#include <string>
#include <format>
#include <stdexcept>
#include <algorithm>
#include <cstddef>

#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>

int main()
{
	WSAData wsa;
	addrinfo* serverAddress = nullptr;
	SOCKET
		serverSocket = INVALID_SOCKET,
		clientSocket = INVALID_SOCKET;
	const int bufferSize = 100;

	auto cleanupResources = [&]() {
		if (serverAddress)
			freeaddrinfo(serverAddress);

		if (clientSocket != INVALID_SOCKET)
			closesocket(clientSocket);

		if (serverSocket != INVALID_SOCKET)
			closesocket(serverSocket);

		WSACleanup();
	};

	try
	{
		// initialize winsock library:
		const bool wsaSuccess = WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
		if (!wsaSuccess)
			throw std::runtime_error{ "Failed to set up winsock" };

		// configure local machine's ip address:
		addrinfo hints{};
		hints.ai_family = AF_INET; // ipv4
		hints.ai_socktype = SOCK_STREAM; // tcp
		hints.ai_flags = AI_PASSIVE; // server

		const bool serverAddressSuccess
			= getaddrinfo(nullptr, "8080", &hints, &serverAddress) == 0;
		if (!serverAddressSuccess)
			throw std::runtime_error{ "Failed to configure server IP address" };

		// create server socket:
		serverSocket = socket(
			serverAddress->ai_family,
			serverAddress->ai_socktype,
			serverAddress->ai_protocol
		);
		if (serverSocket == INVALID_SOCKET)
			throw std::runtime_error{ "Failed to set up server socket" };

		// bind server socket:
		const bool bindSuccess = bind(
			serverSocket,
			serverAddress->ai_addr,
			serverAddress->ai_addrlen
		) == 0;
		if (!bindSuccess)
			throw std::runtime_error{ "Failed to bind server socket to server ip address" };

		// listen for incoming connections:
		const bool listenSuccess = listen(serverSocket, SOMAXCONN) == 0;
		if (!listenSuccess)
			throw std::runtime_error{ "Failed to listen for incoming connections" };

		// create client socket:
		clientSocket = accept(serverSocket, nullptr, nullptr);
		if (clientSocket == INVALID_SOCKET)
			throw std::runtime_error{ "Failed to create a client socket" };

		// receive data from the client:
		std::string buffer(bufferSize, '\0');
		const int bytesReceived = recv(clientSocket, buffer.data(), bufferSize, 0);

		if (bytesReceived < 0)
			throw std::runtime_error{
				std::format("Error while receiving data: {}", WSAGetLastError())
			};
		else if (bytesReceived == 0)
			throw std::runtime_error{ "Client disconnected from server" };
		else
			std::cout << buffer.substr(0, bytesReceived) << '\n';

		// send data to client:
		const std::string message{ "[SERVER]: Hello client!" };
		std::size_t index = 0;

		while (index < message.size())
		{
			const int bytesSent = send(
				clientSocket,
				message.data() + index,
				std::min<int>(static_cast<int>(message.size() - index), bufferSize),
				0
			);

			if (bytesSent < 0)
				throw std::runtime_error{
					std::format("Error while sending data: {}", WSAGetLastError())
				};
			else if (bytesSent == 0)
				throw std::runtime_error{ "Disconnection while trying to send data" };
			else
				index += bytesSent;
		}
	}
	catch (const std::exception& e)
	{
		std::cout << "Error> " << e.what() << '\n';
	}

	cleanupResources();
	std::cin.get();

	return 0;
}
