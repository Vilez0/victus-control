#include <gtk/gtk.h>

#include "about.hpp"
#include "fan.hpp"
#include "keyboard.hpp"

class VictusControl
{
public:
	GtkWidget *window;
	GtkWidget *notebook;
	GtkWidget *menu_button;
	GtkWidget *menu;

	VictusSocketClient *socket_client;
	VictusFanControl fan_control;
	VictusKeyboardControl keyboard_control;
	VictusAbout about;

	VictusControl();
	~VictusControl();

	void add_tabs();
	void add_menu();
	void run();

private:
	static void on_about_clicked(GtkButton *button, gpointer user_data);
};
