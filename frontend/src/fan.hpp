#ifndef FAN_HPP
#define FAN_HPP

#include <gtk/gtk.h>
#include <string>
#include "socket.hpp"

class VictusFanControl
{
public:
	GtkWidget *fan_page;

	VictusFanControl(std::shared_ptr<VictusSocketClient> client);

	GtkWidget *get_page();

private:
	GtkWidget *toggle_button;
	GtkWidget *apply_button;
	GtkWidget *state_label;
	GtkWidget *fan1_speed_label;
	GtkWidget *fan2_speed_label;

	bool automatic_mode;
	std::string current_state;

	void update_fan_mode(bool automatic);
	void update_fan_speeds();
	void update_state_label();
	void update_current_state();

	static void on_toggle_clicked(GtkWidget *widget, gpointer data);
	static void on_apply_clicked(GtkWidget *widget, gpointer data);

	std::shared_ptr<VictusSocketClient> socket_client;
};

#endif // FAN_HPP
