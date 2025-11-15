#include <iostream>
#include <string>
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
	SOCKET clientSocket = INVALID_SOCKET;
	const int bufferSize = 100;

	auto cleanupResources = [&]() {
		if (serverAddress)
			freeaddrinfo(serverAddress);

		if (clientSocket != INVALID_SOCKET)
			closesocket(clientSocket);

		WSACleanup();
	};

	try
	{
		// initialize winsock:
		const bool wsaSuccess = WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
		if (!wsaSuccess)
			throw std::runtime_error{ "Failed to initialize winsock" };
		
		// configure server ip address:
		addrinfo hints{};
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		// notice no hints.ai_flags = AI_PASSIVE, this is not a server

		const bool serverAddressSuccess
			= getaddrinfo("127.0.0.1", "8080", &hints, &serverAddress) == 0; // change ip later
		if (!serverAddressSuccess)
			throw std::runtime_error{ "Failed to configure server's IP address" };

		// create client socket:
		clientSocket = socket(
			serverAddress->ai_family,
			serverAddress->ai_socktype,
			serverAddress->ai_protocol
		);
		if (clientSocket == INVALID_SOCKET)
			throw std::runtime_error{ "Failed to set up client socket" };

		// connect to server:
		const bool connectSuccess = connect(
			clientSocket,
			serverAddress->ai_addr,
			serverAddress->ai_addrlen
		) == 0;
		if (!connectSuccess)
			throw std::runtime_error{ "Failed to connect to server" };

		// send data to server:
		const std::string message{ "[CLIENT]: Hello server!" };
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
					std::format("Error trying to send data: {}", WSAGetLastError())
				};
			else if (bytesSent == 0)
				throw std::runtime_error{ "Lost connection with server" };
			else
				index += bytesSent;
		}

		// receive data from server:
		std::string buffer(bufferSize, '\0');
		const int bytesReceived = recv(clientSocket, buffer.data(), bufferSize, 0);

		if (bytesReceived < 0)
			throw std::runtime_error{
				std::format("Error while receiving data: {}", WSAGetLastError())
			};
		else if (bytesReceived == 0)
			throw std::runtime_error{ "Lost connection with server" };
		else
			std::cout << buffer.substr(0, bytesReceived) << '\n';
	}
	catch (const std::exception& e)
	{
		std::cout << "Error> " << e.what() << '\n';
	}

	cleanupResources();
	std::cin.get();

	return 0;
}
