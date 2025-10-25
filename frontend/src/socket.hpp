#ifndef VICTUS_SOCKET_HPP
#define VICTUS_SOCKET_HPP

#include <string>
#include <future>
#include <unordered_map>
#include <functional>

enum ServerCommands
{
	GET_FAN_SPEED,
	SET_FAN_SPEED,
	GET_FAN_MODE,
	SET_FAN_MODE,
	GET_FANS_CURVE,
	SET_FANS_CURVE,
	GET_FAN_MAX_SPEED,
	GET_KEYBOARD_COLOR,
	SET_KEYBOARD_COLOR,
	GET_KBD_BRIGHTNESS,
	SET_KBD_BRIGHTNESS,
	GET_CPU_TEMP
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