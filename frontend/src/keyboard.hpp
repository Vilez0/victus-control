#ifndef KEYBOARD_HPP
#define KEYBOARD_HPP

#include <gtk/gtk.h>
#include <string>
#include "socket.hpp"

class VictusKeyboardControl
{
public:
	GtkWidget *keyboard_page;

	VictusKeyboardControl(std::shared_ptr<VictusSocketClient> client);

	GtkWidget *get_page();

private:
	GtkLabel *kbd_brightness_label;
	GtkWidget *kbd_brightness_slider;
	GtkWidget *kbd_brightness;
	GtkColorChooser *color_chooser;
	GtkWidget *apply_button;
	GtkWidget *button_box;
	GtkLabel *current_color_label;
	GtkLabel *current_brightness_label;

	void update_keyboard_state(bool enabled);
	void update_keyboard_state_from_device();

	void update_keyboard_brightness(const int brightness);
	void update_keyboard_color(const GdkRGBA &color);

	static void on_brightness_changed(GtkRange *range, gpointer data);
	static void on_color_set(GtkColorChooser *widget, gpointer data);
	static void on_apply_clicked(GtkWidget *widget, gpointer data);
	static void update_current_color_label(gpointer data);

	bool is_keyboard_backlight_supported() const;

	std::shared_ptr<VictusSocketClient> socket_client;
};

#endif