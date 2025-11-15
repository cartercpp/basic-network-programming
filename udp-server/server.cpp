#include <iostream>
#include <string>
#include <format>
#include <stdexcept>

#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>

int main()
{
	WSAData wsa;
	addrinfo* serverAddress = nullptr;
	SOCKET serverSocket = INVALID_SOCKET;
	const int bufferSize = 100;

	auto cleanupResources = [&]() {
		if (serverAddress)
			freeaddrinfo(serverAddress);

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
		hints.ai_socktype = SOCK_DGRAM; // udp
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

		// receive data from the client:
		sockaddr_storage clientAddress{};
		int clientSize = sizeof(clientAddress);

		std::string buffer(bufferSize, '\0');
		const int bytesReceived
			= recvfrom(serverSocket, buffer.data(), bufferSize, 0,
				(sockaddr*)&clientAddress, &clientSize);

		if (bytesReceived == SOCKET_ERROR)
			throw std::runtime_error{
				std::format("Error while receiving data: {}", WSAGetLastError())
		};
		else
			std::cout << buffer.substr(0, bytesReceived) << '\n';

		// send data to client:
		const std::string message{ "[SERVER]: Hello client!" };

		const int bytesSent = sendto(
			serverSocket,
			message.data(),
			static_cast<int>(message.size()),
			0,
			(sockaddr*)&clientAddress,
			clientSize
		);

		if (bytesSent == SOCKET_ERROR)
			throw std::runtime_error{
				std::format("Error while sending data: {}", WSAGetLastError())
			};
	}
	catch (const std::exception& e)
	{
		std::cout << "Error> " << e.what() << '\n';
	}

	cleanupResources();
	std::cin.get();

	return 0;
}
