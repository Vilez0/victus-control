#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/stat.h>

#include "handler.hpp"

#define SOCKET_DIR "/var/run/victus-control"
#define SOCKET_PATH SOCKET_DIR "/victus_backend.sock"


int main()
{
	int server_socket, client_socket;
	struct sockaddr_un server_addr;

	unlink(SOCKET_PATH);

	if (mkdir(SOCKET_DIR, 0755) < 0 && errno != EEXIST)
	{
		std::cerr << "Failed to create socket directory" << std::endl;
		return 1;
	}

	server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_socket < 0)
	{
		std::cerr << "Error creating socket" << std::endl;
		return 1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, SOCKET_PATH);

	if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		std::cerr << "Bind failed" << std::endl;
		return 1;
	}

	if (chmod(SOCKET_PATH, 0666) < 0)
	{
		std::cerr << "Failed to set socket permissions" << std::endl;
		return 1;
	}

	if (listen(server_socket, 5) < 0)
	{
		std::cerr << "Listen failed" << std::endl;
		return 1;
	}

	std::cout << "Server is listening..." << std::endl;

	handle_load_config();

	while (true)
	{
		client_socket = accept(server_socket, nullptr, nullptr);
		if (client_socket < 0)
		{
			perror("accept");
			continue;
		}

		std::cout << "Client connected" << std::endl;

		while (true)
		{
			char buffer[256] = {0};
			ssize_t bytes_received = read(client_socket, buffer, sizeof(buffer) - 1);
			if (bytes_received <= 0)
			{
				std::cerr << "Client disconnected or error occurred.\n";
				break;
			}

			handle_command(buffer, client_socket);
		}

		close(client_socket);
		std::cout << "Client disconnected" << std::endl;
	}

	close(server_socket);
	return 0;
}