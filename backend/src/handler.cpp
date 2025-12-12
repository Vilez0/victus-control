#include <string>
#include <iostream>
#include <sys/socket.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <filesystem>

#include "handler.hpp"

#define CONFIG_PATH "/var/lib/victus-control/config"

void handle_load_config() {
	// example config file format:
	// kbd_color=255 255 255
	// kbd_brightness=255
	// fan_mode=AUTO
	// fan_curve_settings=
	// fan1_slider_settings=2500
	// fan2_slider_settings=5400
	std::ifstream config_file(CONFIG_PATH);
	if (!config_file.is_open()) {
		std::cerr << "Config file not found, using defaults." << std::endl;
		return;
	}
    std::string color, brightness, mode, curve, fan1_speed, fan2_speed;

	std::string line;
	while (std::getline(config_file, line)) {
		if (line.find("kbd_color=") == 0) {
            color = line.substr(9);
            set_keyboard_color(color);
		} else if (line.find("kbd_brightness=") == 0) {
			brightness = line.substr(15);
			set_keyboard_brightness(brightness);
		} else if (line.find("fan_mode=") == 0) {
			mode = line.substr(9);
			set_fan_mode(mode);
		} else if (line.find("fan_curve_settings=") == 0) {
            if (mode != "MANUAL") continue;
			curve = line.substr(19);
			set_fans_curve(curve);
        } else if (line.find("fan1_slider_settings=") == 0) {
            if (mode != "MANUAL") continue;
            fan1_speed = line.substr(21);
            set_fan_speed("1", fan1_speed);
		} else if (line.find("fan2_slider_settings=") == 0) {
            if (mode != "MANUAL") continue;
            fan2_speed = line.substr(21);
            set_fan_speed("2", fan2_speed);
		}
	}
}

void handle_write_config(const std::string &key, const std::string &value) {
    std::ifstream config_file_in(CONFIG_PATH);
    std::ostringstream temp_stream;
    bool key_found = false;

    // if config file doesn't exist, create it with root permissions
    if (!std::filesystem::exists(CONFIG_PATH)) {
        mkdir("/var/lib/victus-control", 0700);
    }

    if (config_file_in.is_open()) {
        std::string line;
        while (std::getline(config_file_in, line)) {
            if (line.find(key + "=") == 0) {
                temp_stream << key << "=" << value << "\n";
                key_found = true;
            } else {
                temp_stream << line << "\n";
            }
        }
        config_file_in.close();
    }

    if (!key_found) {
        temp_stream << key << "=" << value << "\n";
    }

    std::ofstream config_file_out(CONFIG_PATH);
    if (config_file_out.is_open()) {
        config_file_out << temp_stream.str();
        config_file_out.close();
    } else {
        std::cerr << "Failed to open config file for writing." << std::endl;
    }
}

void remove_config_key(const std::string &key) {
    std::ifstream config_file_in(CONFIG_PATH);
    std::ostringstream temp_stream;
    bool key_found = false;

    if (!config_file_in.is_open()) {
        std::cerr << "Config file not found." << std::endl;
    }

    std::string line;
    while (std::getline(config_file_in, line)) {
        if (line.find(key + "=") == 0) {
            key_found = true;
            continue; // skip this line
        } else {
            temp_stream << line << "\n";
        }
    }
    config_file_in.close();

    if (!key_found) {
        std::cerr << "Key not found in config." << std::endl;
    }

    std::ofstream config_file_out(CONFIG_PATH);
    if (config_file_out.is_open()) {
        config_file_out << temp_stream.str();
        config_file_out.close();
    } else {
        std::cerr << "Failed to open config file for writing." << std::endl;
    }
}

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
        remove_config_key("fan_curve_settings");
        handle_write_config("fan" + fan_num + "_slider_settings", speed);
	}
	else if (command == "GET_FAN_MODE")
	{
		response = get_fan_mode();
	}
	else if (command.find("SET_FAN_MODE") == 0)
	{
		std::string mode = command.substr(13); // 1 more char for the space
		response = set_fan_mode(mode);
		fan_mode_trigger(mode);
        handle_write_config("fan_mode", mode);
	}
	else if (command == "GET_FANS_CURVE")
	{
		response = get_fans_curve();
	}
	else if (command.find("SET_FANS_CURVE") == 0)
	{
		std::string curve = command.substr(15);
		response = set_fans_curve(curve);
		fan_curve_monitor();
        remove_config_key("fan1_slider_settings");
        remove_config_key("fan2_slider_settings");
        handle_write_config("fan_curve_settings", curve);
	}
	else if (command.find("GET_FAN_MAX_SPEED") == 0) {
		response = get_fan_max_speed(command.substr(18));
	}
	else if (command == "GET_KEYBOARD_COLOR")
	{
		response = get_keyboard_color();
	}
	else if (command.find("SET_KEYBOARD_COLOR") == 0)
	{
		std::string color = command.substr(19);
		response = set_keyboard_color(color);
		handle_write_config("kbd_color", color);
	}
	else if (command == "GET_KBD_BRIGHTNESS")
	{
		response = get_keyboard_brightness();
	}
	else if (command.find("SET_KBD_BRIGHTNESS") == 0)
	{
		std::string value = command.substr(19);
		response = set_keyboard_brightness(value);
        handle_write_config("kbd_brightness", value);
	}
	else if (command.find("GET_CPU_TEMP") == 0)
	{
		response = get_cpu_temperature();
	} else if (command == "GET_DRIVER_SUPPORT_FLAGS") {
		response = get_driver_support_flags();
	} else
		response = "ERROR: Unknown command";

	if (send(client_socket, response.c_str(), response.length(), 0) < 0)
		std::cerr << "Failed to send response" << std::endl;
}
