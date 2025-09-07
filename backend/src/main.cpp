#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string>
#include "util.hpp"

#define SOCKET_DIR "/var/run/victus-control"
#define SOCKET_PATH SOCKET_DIR "/victus_backend.sock"

void handle_command(const std::string &command, int client_socket)
{
	std::string response;

	if (command.find("GET_FAN_SPEED") == 0)
	{
		std::string fan_num = command.substr(14);
		std::string hwmon_path = find_hwmon_directory("/sys/devices/platform/hp-wmi/hwmon");

		if (!hwmon_path.empty())
		{
			std::ifstream fan_file(hwmon_path + "/fan" + fan_num + "_input");

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
	else if (command.find("SET_FAN_MODE") == 0)
	{
		std::string mode = command.substr(13); // 1 more char for the space
		std::string hwmon_path = find_hwmon_directory("/sys/devices/platform/hp-wmi/hwmon");

		if (!hwmon_path.empty())
		{
			std::ofstream fan_ctrl(hwmon_path + "/pwm1_enable");

			if (fan_ctrl)
			{
				if (mode == "AUTO")
					fan_ctrl << "2";
				else if (mode == "MANUAL")
					fan_ctrl << "1";
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
	else if (command == "GET_FAN_MODE")
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
				else if (fan_mode == "1")
					response = "MANUAL";
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
	else if (command == "GET_KEYBOARD_COLOR")
	{
		std::ifstream rgb("/sys/class/leds/hp::kbd_backlight/multi_intensity");

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
	else if (command.find("SET_KEYBOARD_COLOR") == 0)
	{
		std::string color = command.substr(19);

		std::ofstream rgb("/sys/class/leds/hp::kbd_backlight/multi_intensity");
		if (rgb && color.size() > 4) // it should be at least 5 chars because "R G B" with spaces
		{
            rgb << color;
			std::cout << "Setting keyboard color to: " << color << std::endl;
            rgb.close();

			response = "OK";
		}
		else
			response = "ERROR: Invalid RGB command";
	}
	else if (command == "GET_KBD_BRIGHTNESS")
	{
		std::ifstream brightness("/sys/class/leds/hp::kbd_backlight/brightness");

		if (brightness)
		{
			std::stringstream buffer;
			buffer << brightness.rdbuf();

			std::string keyboard_brightness = buffer.str();
			keyboard_brightness.erase(keyboard_brightness.find_last_not_of(" \n\r\t") + 1);

			response = keyboard_brightness;
		}
		else
			response = "ERROR: Keyboard Brightness File not found";
	}
	else if (command.find("SET_KBD_BRIGHTNESS") == 0)
	{
		std::string value = command.substr(19);
		std::ofstream brightness("/sys/class/leds/hp::kbd_backlight/brightness");
		if (brightness)
		{
			brightness << value;
			response = "OK";
		}
		else
			response = "ERROR: Invalid Keyboard Brightness command";
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

			//std::cout << "Received command: " << buffer << std::endl;
			handle_command(buffer, client_socket);
		}

		close(client_socket);
		std::cout << "Client disconnected" << std::endl;
	}

	close(server_socket);
	return 0;
}