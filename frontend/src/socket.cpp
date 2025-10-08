#include "socket.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

VictusSocketClient::VictusSocketClient(const std::string &path) : socket_path(path), sockfd(-1)
{
	command_prefix_map = {
		{GET_FAN_SPEED, "GET_FAN_SPEED"},
		{SET_FAN_SPEED, "SET_FAN_SPEED"},
		{SET_FAN_MODE, "SET_FAN_MODE"},
		{GET_FAN_MODE, "GET_FAN_MODE"},
		{GET_KEYBOARD_COLOR, "GET_KEYBOARD_COLOR"},
		{SET_KEYBOARD_COLOR, "SET_KEYBOARD_COLOR"},
		{GET_KBD_BRIGHTNESS, "GET_KBD_BRIGHTNESS"},
		{SET_KBD_BRIGHTNESS, "SET_KBD_BRIGHTNESS"},
	};

	if (!connect_to_server())
		throw std::runtime_error("Failed to connect to server");
}

VictusSocketClient::~VictusSocketClient()
{
	close_socket();
}

bool VictusSocketClient::connect_to_server()
{
	std::cout << "Connecting to server...\n";

	if (sockfd != -1)
		close_socket();

	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		std::cerr << "Cannot create socket!\n";
		return false;
	}

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

	if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		std::cerr << "Failed to connect to the server!\n";
		close(sockfd);
		sockfd = -1;
		return false;
	}

	std::cout << "Connection to server successful.\n";

	return true;
}

void VictusSocketClient::close_socket()
{
	std::cout << "Closing the connection...\n";

	if (sockfd != -1)
		close(sockfd);

	std::cout << "Connection closed.\n";
}

std::string VictusSocketClient::send_command(const std::string &command)
{
	if (sockfd == -1 && !connect_to_server())
		return "ERROR: No server connection?";

	send(sockfd, command.c_str(), command.size(), 0);
	char buffer[256];
	memset(buffer, 0, sizeof(buffer));

	recv(sockfd, buffer, sizeof(buffer) - 1, 0);
	return std::string(buffer);
}

std::future<std::string> VictusSocketClient::send_command_async(ServerCommands type, const std::string &command)
{
	return std::async(std::launch::async, [this, type, command]()
					  {
		auto it = command_prefix_map.find(type);
		if (it != command_prefix_map.end())
		{
			std::string full_command = it->second;
			if (!command.empty())
			{
				if (it->second.back() != ' ')
				{
					full_command += " ";
				}
				full_command += command;
			}
			std::cout << "Sending command: " << full_command << std::endl;
			auto result = send_command(full_command);
			std::cout << "Received response: " << result << std::endl;
			return result;
		}
		return std::string("ERROR: Unknown command type"); });
}