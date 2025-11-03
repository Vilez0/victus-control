#include "fan.hpp"
#include "keyboard.hpp"
#include "util.hpp"

void handle_write_config(const std::string &key, const std::string &value);
void handle_load_config();
void remove_config_key(const std::string &key);
void handle_command(const std::string &command, int client_socket);