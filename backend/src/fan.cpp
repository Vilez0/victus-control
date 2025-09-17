#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/un.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <optional>

#include "fan.hpp"
#include "util.hpp"

static std::atomic<int> fan_thread_generation(0);
static std::atomic<bool> is_reapplying(false);
static std::mutex fan_state_mutex;
static std::optional<std::string> last_fan1_speed;
static std::optional<std::string> last_fan2_speed;

// Function to reapply fan settings without triggering fan_mode_trigger
void reapply_fan_settings() {
    bool expected = false;
    if (!is_reapplying.compare_exchange_strong(expected, true, std::memory_order_acquire)) {
        return; // Another reapply loop is already running
    }

    std::optional<std::string> fan1_speed;
    std::optional<std::string> fan2_speed;
    {
        std::lock_guard<std::mutex> lock(fan_state_mutex);
        fan1_speed = last_fan1_speed;
        fan2_speed = last_fan2_speed;
    }

    std::string current_mode = get_fan_mode();
    if (current_mode == "MANUAL" && (fan1_speed || fan2_speed)) {
        std::ostringstream log_message;
        log_message << "Re-applying manual fan settings";
        bool has_detail = false;

        if (fan1_speed) {
            log_message << (has_detail ? ", " : ": ") << "fan1=" << *fan1_speed;
            has_detail = true;
        }
        if (fan2_speed) {
            log_message << (has_detail ? ", " : ": ") << "fan2=" << *fan2_speed;
        }

        std::cout << log_message.str() << std::endl;

        if (fan1_speed) {
            std::string command = "sudo /usr/bin/set-fan-speed.sh 1 " + *fan1_speed;
            int result = system(command.c_str());
            if (result != 0) {
                std::cerr << "Failed to reapply fan 1 speed" << std::endl;
            }
        }

        if (fan2_speed) {
            if (fan1_speed) {
                std::this_thread::sleep_for(std::chrono::seconds(10));
            }
            std::string command = "sudo /usr/bin/set-fan-speed.sh 2 " + *fan2_speed;
            int result = system(command.c_str());
            if (result != 0) {
                std::cerr << "Failed to reapply fan 2 speed" << std::endl;
            }
        }
    }

    is_reapplying.store(false, std::memory_order_release);
}

// call set_fan_mode every 90 seconds so that the mode doesn't revert back (weird hp behaviour)
// also re-applies manual fan speed
void fan_mode_trigger(const std::string mode) {
    fan_thread_generation++;
	if (mode == "AUTO") return;

    std::thread([mode, gen = fan_thread_generation.load()]() {
        while (fan_thread_generation == gen) {
            // Reapply the fan mode
            set_fan_mode(mode);

            // Reapply fan settings if in manual mode
            if (mode == "MANUAL") {
                reapply_fan_settings();
            }

            // Wait for the interval (90 seconds)
            for (int i = 0; i < 90; ++i) {
                if (fan_thread_generation != gen) return;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }).detach();
}

std::string get_fan_mode()
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
			else
				return "ERROR: Invalid fan mode: " + mode;

			fan_ctrl.flush();
			if (fan_ctrl.fail()) {
				return "ERROR: Failed to write fan mode";
			}
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
	std::string hwmon_path = find_hwmon_directory("/sys/devices/platform/hp-wmi/hwmon");

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

std::string set_fan_speed(const std::string &fan_num, const std::string &speed, bool trigger_mode)
{
    {
        std::lock_guard<std::mutex> lock(fan_state_mutex);
        if (fan_num == "1") {
            last_fan1_speed = speed;
        } else if (fan_num == "2") {
            last_fan2_speed = speed;
        }
    }

    // Construct the command to call the external script with sudo
    // The script must be in a location like /usr/bin
    std::string command = "sudo /usr/bin/set-fan-speed.sh " + fan_num + " " + speed;

    int result = system(command.c_str());

    if (result == 0)
    {
        // Only trigger fan_mode_trigger if requested and not already reapplying
        if (trigger_mode && !is_reapplying.load(std::memory_order_acquire) && get_fan_mode() == "MANUAL") {
            fan_mode_trigger("MANUAL");
        }
        return "OK";
    }
    else
    {
        std::cerr << "Failed to execute set-fan-speed.sh for fan " << fan_num << ". Exit code: " << WEXITSTATUS(result) << std::endl;
        return "ERROR: Failed to set fan speed";
    }
}
