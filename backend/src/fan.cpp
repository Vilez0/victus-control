#include <array>
#include <iostream>
#include <fstream>
#include <stop_token>
#include <sstream>
#include <sys/un.h>
#include <thread>
#include <chrono>

#include "fan.hpp"
#include "util.hpp"

static std::array<std::jthread, 2> fan_threads;
static std::jthread fan_curves_thread;
static std::array<std::string, 2> fan_curves;

// call set_fan_mode every 100 seconds so that the mode doesn't revert back (weird hp behaviour)
void fan_mode_trigger(const std::string mode) {
	if (mode == "AUTO") return;
    std::thread([mode]() {
        while (true) {
            if (get_fan_mode() != mode) {
                break;
            }

            set_fan_mode(mode);
            std::this_thread::sleep_for(std::chrono::seconds(100));
        }
    }).detach();
}

void manual_fan_speed_maintainer(const std::string fan_num, const std::string speed) {
    int num = std::stoi(fan_num);
    if (num < 1 || num > 2) return;

    auto &worker = fan_threads[num - 1];
    if (worker.joinable()) {
        worker.request_stop();
        worker.join();
    }

    worker = std::jthread([fan_num, speed](std::stop_token stop_token) {
        while (!stop_token.stop_requested() && get_fan_mode() == "MANUAL") {
            set_fan_speed(fan_num, speed);

            for (int i = 0; i < 100 && !stop_token.stop_requested(); ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    });
}

std::string get_fan_mode()
{
	std::string hwmon_path = find_hp_hwmon_directory("/sys/devices/platform/hp-wmi/hwmon");

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

			if (fan_mode == "2")
				return "AUTO";
			else if (fan_mode == "1")
				return "MANUAL";
			else if (fan_mode == "0")
				return "MAX";
			else
				return "ERROR: Unknown fan mode " + fan_mode;
		}
		else
		{
			std::cerr << "Failed to open fan control file. Error: " << strerror(errno) << std::endl;
			return "ERROR: Unable to read fan mode";
		}
	}
	else
	{
		std::cerr << "Hwmon directory not found" << std::endl;
		return "ERROR: Hwmon directory not found";
	}
}

std::string set_fan_mode(const std::string &mode)
{
	std::string hwmon_path = find_hp_hwmon_directory("/sys/devices/platform/hp-wmi/hwmon");

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

			return "OK";
		}
		else
			return "ERROR: Unable to set fan mode";
	}
	else
		return "ERROR: Hwmon directory not found";
}

std::string get_fan_speed(const std::string &fan_num)
{
	std::string hwmon_path = find_hp_hwmon_directory("/sys/devices/platform/hp-wmi/hwmon");

	if (!hwmon_path.empty())
	{
		std::ifstream fan_file(hwmon_path + "/fan" + fan_num + "_input");

		if (fan_file)
		{
			std::stringstream buffer;
			buffer << fan_file.rdbuf();

			std::string fan_speed = buffer.str();

			fan_speed.erase(fan_speed.find_last_not_of(" \n\r\t") + 1);

			return fan_speed;
		}
		else
		{
			std::cerr << "Failed to open fan speed file. Error: " << strerror(errno) << std::endl;
			return "ERROR: Unable to read fan speed";
		}
	}
	else
	{
		std::cerr << "Hwmon directory not found" << std::endl;
		return "ERROR: Hwmon directory not found";
	}
}

std::string get_fan_max_speed(const std::string &fan_num)
{
	std::string hwmon_path = find_hp_hwmon_directory("/sys/devices/platform/hp-wmi/hwmon");

	if (!hwmon_path.empty())
	{
		std::ifstream fan_file(hwmon_path + "/fan" + fan_num + "_max");

		if (fan_file)
		{
			std::stringstream buffer;
			buffer << fan_file.rdbuf();

			std::string fan_speed = buffer.str();

			fan_speed.erase(fan_speed.find_last_not_of(" \n\r\t") + 1);

			if (fan_speed == "0")
				fan_speed = "4500"; // default max speed

			return fan_speed;
		}
		else
		{
			std::cerr << "Failed to open fan max speed file. Error: " << strerror(errno) << std::endl;
			return "ERROR: Unable to read fan max speed";
		}
	}
	else
	{
		std::cerr << "Hwmon directory not found" << std::endl;
		return "ERROR: Hwmon directory not found";
	}
}

std::string set_fan_speed(const std::string &fan_num, const std::string &speed)
{
	std::cout << "Setting fan " << fan_num << " speed to " << speed << std::endl;
	std::string hwmon_path = find_hp_hwmon_directory("/sys/devices/platform/hp-wmi/hwmon");

	if (!hwmon_path.empty())
	{
		std::ofstream fan_file(hwmon_path + "/fan" + fan_num + "_target");

		if (fan_file)
		{
			fan_file << speed;
			return "OK";
		}
		else
			return "ERROR: Unable to set fan speed";
	}
	else
		return "ERROR: Hwmon directory not found";
}

void fan_curve_monitor()
{
	auto &worker = fan_curves_thread;
	if (worker.joinable()) {
		worker.request_stop();
		worker.join();
	}

	worker = std::jthread([](std::stop_token stop_token) {
		while (!stop_token.stop_requested()) {
			std::string cpu_temp_str = get_cpu_temperature();
			if (cpu_temp_str.find("ERROR") != std::string::npos) {
				std::this_thread::sleep_for(std::chrono::seconds(10));
				continue;
			}
			float cpu_temp = std::stof(cpu_temp_str);

			std::string current_mode = get_fan_mode();
			if (current_mode != "MANUAL") {
				break;
			}

			for (size_t i = 0; i < fan_curves.size(); ++i) {
				if (fan_curves[i].empty()) {
					continue;
				}

				std::istringstream ss(fan_curves[i]);
				std::string segment;
				int target_speed = -1;

				while (std::getline(ss, segment, ',')) {
					size_t colon_pos = segment.find(':');
					if (colon_pos == std::string::npos) {
						continue;
					}

					float temp_threshold = std::stof(segment.substr(0, colon_pos));
					int speed_percent = std::stoi(segment.substr(colon_pos + 1));

					if (cpu_temp <= temp_threshold) {
						std::string fan_max_speed = get_fan_max_speed(std::to_string(i + 1));
						int speed_value = (std::stoi(fan_max_speed) * speed_percent) / 100;
						target_speed = speed_value;
						break;
					}
				}

				if (target_speed != -1) {
					set_fan_speed(std::to_string(i + 1), std::to_string(target_speed));
				}
			}
			std::this_thread::sleep_for(std::chrono::seconds(5));
		}
	});
}

std::string set_fans_curve(const std::string &curve)
{
	// example curve format: "1/40:30,60:70,80:100;2/40:20,60:60,80:90"
	std::istringstream ss(curve);
	std::string fan_curve;
	while (std::getline(ss, fan_curve, ';')) {
		size_t slash_pos = fan_curve.find('/');
		if (slash_pos == std::string::npos) {
			return "ERROR: Invalid curve format";
		}

		int fan_num = std::stoi(fan_curve.substr(0, slash_pos));
		if (fan_num < 1 || fan_num > 2) {
			return "ERROR: Invalid fan number";
		}

		fan_curves[fan_num - 1] = fan_curve.substr(slash_pos + 1);
	}

	return "OK";
}

std::string get_fans_curve()
{
	std::ostringstream ss;
	for (size_t i = 0; i < fan_curves.size(); ++i) {
		ss << (i + 1) << "/" << fan_curves[i];
		if (i < fan_curves.size() - 1) {
			ss << ";";
		}
	}
	return ss.str();
}
