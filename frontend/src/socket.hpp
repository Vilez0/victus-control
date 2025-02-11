#ifndef VICTUS_SOCKET_HPP
#define VICTUS_SOCKET_HPP

#include <string>
#include <future>
#include <unordered_map>
#include <functional>

enum ServerCommands
{
	GET_FAN_SPEED,
	GET_FAN_SPEED2,
	SET_FAN,
	GET_FAN,
	GET_RGB,
	SET_RGB,
	GET_KEYBOARD_ENABLE,
	SET_KEYBOARD_ENABLE
};

class VictusSocketClient
{
public:
	VictusSocketClient(const std::string &socket_path);
	~VictusSocketClient();

	std::future<std::string> send_command_async(ServerCommands type, const std::string &command = "");

private:
	std::string send_command(const std::string &command);
	std::string socket_path;

	bool connect_to_server();
	void close_socket();

	int sockfd;

	std::unordered_map<ServerCommands, std::string> command_prefix_map;
};

#endif // VICTUS_SOCKET_HPP