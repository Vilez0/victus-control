#include "util.hpp"
#include <dirent.h>

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