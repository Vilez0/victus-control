#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string>

#define SOCKET_PATH "/tmp/victus_backend.sock"

std::string find_hwmon_directory(const std::string &base_path)
{
	DIR *dir;
	struct dirent *ent;
	std::string hwmon_path;

	if ((dir = opendir(base_path.c_str())) != nullptr)
	{
		while ((ent = readdir(dir)) != nullptr)
		{
			if (std::string(ent->d_name).find("hwmon") != std::string::npos)
			{
				hwmon_path = base_path + "/" + ent->d_name;
				break;
			}
		}
		closedir(dir);
	}
	return hwmon_path;
}

void handle_command(const std::string &command, int client_socket)
{
	std::string response;

	if (command == "GET_FAN_SPEED")
	{
		std::string hwmon_path = find_hwmon_directory("/sys/devices/platform/hp-wmi/hwmon");

		if (!hwmon_path.empty())
		{
			std::ifstream fan_file(hwmon_path + "/fan1_input");

			if (fan_file)
			{
				std::stringstream buffer;
				buffer << fan_file.rdbuf();

				std::string fan_speed1 = buffer.str();

				fan_speed1.erase(fan_speed1.find_last_not_of(" \n\r\t") + 1);

				response = fan_speed1;
			}
			else
			{
				std::cerr << "Failed to open fan speed file. Error: " << strerror(errno) << std::endl;
				response = "ERROR: Unable to read fan speed";
			}
		}
		else
		{
			std::cerr << "Hwmon directory not found" << std::endl;
			response = "ERROR: Hwmon directory not found";
		}
	}
	else if (command == "GET_FAN_SPEED2")
	{
		std::string hwmon_path = find_hwmon_directory("/sys/devices/platform/hp-wmi/hwmon");

		if (!hwmon_path.empty())
		{
			std::ifstream fan_file(hwmon_path + "/fan2_input");
			if (fan_file)
			{
				std::stringstream buffer;
				buffer << fan_file.rdbuf();

				std::string fan_speed2 = buffer.str();

				fan_speed2.erase(fan_speed2.find_last_not_of(" \n\r\t") + 1);

				response = fan_speed2;
			}
			else
			{
				std::cerr << "Failed to open fan speed file. Error: " << strerror(errno) << std::endl;
				response = "ERROR: Unable to read fan speed";
			}
		}
		else
		{
			std::cerr << "Hwmon directory not found" << std::endl;
			response = "ERROR: Hwmon directory not found";
		}
	}
	else if (command.find("SET_FAN") == 0)
	{
		std::string mode = command.substr(8);
		std::string hwmon_path = find_hwmon_directory("/sys/devices/platform/hp-wmi/hwmon");

		if (!hwmon_path.empty())
		{
			std::ofstream fan_ctrl(hwmon_path + "/pwm1_enable");

			if (fan_ctrl)
			{
				if (mode == "AUTO")
					fan_ctrl << "2";
				else if (mode == "MAX")
					fan_ctrl << "0";

				response = "OK";
			}
			else
				response = "ERROR: Unable to set fan mode";
		}
		else
		{
			std::cerr << "Hwmon directory not found" << std::endl;
			response = "ERROR: Hwmon directory not found";
		}
	}
	else if (command == "GET_FAN")
	{
		std::string hwmon_path = find_hwmon_directory("/sys/devices/platform/hp-wmi/hwmon");

		if (!hwmon_path.empty())
		{
			std::string pwm_path = hwmon_path + "/pwm1_enable";

			std::ifstream fan_ctrl(pwm_path);
			if (fan_ctrl)
			{
				std::stringstream buffer;
				buffer << fan_ctrl.rdbuf();
				std::string fan_mode = buffer.str();

				fan_mode.erase(fan_mode.find_last_not_of(" \n\r\t") + 1);

				//std::cout << "Fan mode: " << fan_mode << std::endl;

				if (fan_mode == "2")
					response = "AUTO";
				else if (fan_mode == "0")
					response = "MAX";
				else
					response = "ERROR: Unknown fan mode " + fan_mode;
			}
			else
			{
				std::cerr << "Failed to open fan control file. Error: " << strerror(errno) << std::endl;
				response = "ERROR: Unable to read fan mode";
			}
		}
		else
		{
			std::cerr << "Hwmon directory not found" << std::endl;
			response = "ERROR: Hwmon directory not found";
		}

		//std::cout << "Response: " << response << std::endl;
	}
	else if (command == "GET_RGB")
	{
		std::ifstream rgb("/sys/devices/platform/hp-wmi/keyboard_color");

		if (rgb)
		{
			std::stringstream buffer;
			buffer << rgb.rdbuf();

			std::string rgb_mode = buffer.str();
			rgb_mode.erase(rgb_mode.find_last_not_of(" \n\r\t") + 1);

			response = rgb_mode;
		}
		else
			response = "ERROR: RGB File not found";
	}
	else if (command.find("SET_RGB") == 0)
	{
		std::string color = command.substr(8);

		std::ofstream rgb("/sys/devices/platform/hp-wmi/keyboard_color");
		if (rgb && color.size() == 6)
		{
			// I hate my life
			int r_val = std::stoi(color.substr(0, 2), nullptr, 16);
			int g_val = std::stoi(color.substr(2, 2), nullptr, 16);
			int b_val = std::stoi(color.substr(4, 2), nullptr, 16);

			rgb << std::setw(2) << std::setfill('0') << std::hex << r_val
				<< std::setw(2) << std::setfill('0') << std::hex << g_val
				<< std::setw(2) << std::setfill('0') << std::hex << b_val;
			rgb.close();

			response = "OK";
		}
		else
			response = "ERROR: Invalid RGB command";
	}
	else if (command == "GET_KEYBOARD_ENABLE")
	{
		std::ifstream keyboard_enable("/sys/devices/platform/hp-wmi/keyboard_enable");

		if (keyboard_enable)
		{
			std::stringstream buffer;
			buffer << keyboard_enable.rdbuf();

			std::string keyboard_enable_mode = buffer.str();
			keyboard_enable_mode.erase(keyboard_enable_mode.find_last_not_of(" \n\r\t") + 1);

			response = keyboard_enable_mode;
		}
		else
			response = "ERROR: Keyboard Enable File not found";
	}
	else if (command.find("SET_KEYBOARD_ENABLE") == 0)
	{
		std::string value = command.substr(20);
		std::ofstream keyboard_enable("/sys/devices/platform/hp-wmi/keyboard_enable");
		if (keyboard_enable)
		{
			keyboard_enable << value;
			response = "OK";
		}
		else
			response = "ERROR: Invalid Keyboard Enable command";
	}
	else
		response = "ERROR: Unknown command";

	if (send(client_socket, response.c_str(), response.length(), 0) < 0)
	//{
		std::cerr << "Failed to send response" << std::endl;
	//}
	//else
	//	std::cout << "Server command: " << response << std::endl;
}

int main()
{
	int server_socket, client_socket;
	struct sockaddr_un server_addr;

	unlink(SOCKET_PATH);

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

			//std::cout << "Received command: " << buffer << std::endl;
			handle_command(buffer, client_socket);
		}

		close(client_socket);
		std::cout << "Client disconnected" << std::endl;
	}

	close(server_socket);
	return 0;
}