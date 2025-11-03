#include "util.hpp"

#include <filesystem>
#include <fstream>
#include <dirent.h>

std::string find_hp_hwmon_directory()
{
	const std::string base_path = "/sys/devices/platform/hp-wmi/hwmon";
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

std::string find_cpu_hwmon_directory()
{
	std::string base_path = "/sys/class/hwmon";
	for (const auto &entry : std::filesystem::directory_iterator(base_path))
	{
		std::string name_file = entry.path().string() + "/name";
		std::ifstream file(name_file);
		if (file)
		{
			std::string name;
			std::getline(file, name);
			if (name == "coretemp" || name == "k10temp")
			{
				return entry.path().string();
			}
		}
	}
	return "";
}

std::string get_cpu_temperature()
{
	std::string hwmon_path = find_cpu_hwmon_directory();

	if (!hwmon_path.empty())
	{
		std::ifstream temp_file(hwmon_path + "/temp1_input");
		if (temp_file)
		{
			std::string temp_str;
			std::getline(temp_file, temp_str);
			int temp_millideg = std::stoi(temp_str);
			float temp_deg = temp_millideg / 1000.0;
			return std::to_string(temp_deg).substr(0, 4); // Return first 4 characters for XX.X
		}
		else
		{
			return "ERROR: Unable to read CPU temperature";
		}
	}
	else
	{
		return "ERROR: CPU Hwmon directory not found";
	}
}

std::string get_driver_support_flags()
{
	std::string status_flags;
	std::string hwmon_path = find_hp_hwmon_directory();

	if (std::filesystem::exists(hwmon_path + "/fan1_target"))
	{
		status_flags += "FAN_CONTROL_SUPPORTED";
	}
	
	if (std::filesystem::exists("/sys/class/leds/hp::kbd_backlight/multi_intensity"))
	{
		if (!status_flags.empty()) {
			status_flags += ",";
		}
		status_flags += " KBD_BACKLIGHT_SUPPORTED";
	}

	return status_flags;
}