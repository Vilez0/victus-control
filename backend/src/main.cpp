#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string>

#include "fan.hpp"
#include "keyboard.hpp"

#define SOCKET_DIR "/var/run/victus-control"
#define SOCKET_PATH SOCKET_DIR "/victus_backend.sock"

void handle_command(const std::string &command, int client_socket)
{
	std::string response;

	if (command.find("GET_FAN_SPEED") == 0)
	{
		response = get_fan_speed(command.substr(14));
	}
	else if (command.find("SET_FAN_SPEED") == 0)
	{
		std::string fan_num = command.substr(14, 1);
		std::string speed = command.substr(16);
		response = set_fan_speed(fan_num, speed);
	}
	else if (command.find("SET_FAN_MODE") == 0)
	{
		std::string mode = command.substr(13); // 1 more char for the space
		response = set_fan_mode(mode);
		fan_mode_trigger(mode);
	}
	else if (command == "GET_FAN_MODE")
	{
		response = get_fan_mode();
	}
	else if (command == "GET_KEYBOARD_COLOR")
	{
		response = get_keyboard_color();
	}
	else if (command.find("SET_KEYBOARD_COLOR") == 0)
	{
		std::string color = command.substr(19);
		response = set_keyboard_color(color);
	}
	else if (command == "GET_KBD_BRIGHTNESS")
	{
		response = get_keyboard_brightness();
	}
	else if (command.find("SET_KBD_BRIGHTNESS") == 0)
	{
		std::string value = command.substr(19);
		response = set_keyboard_brightness(value);
	}
	else
		response = "ERROR: Unknown command";

	if (send(client_socket, response.c_str(), response.length(), 0) < 0)
		std::cerr << "Failed to send response" << std::endl;
}

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