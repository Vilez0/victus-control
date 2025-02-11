#include "keyboard.hpp"
#include <fstream>
#include <iostream>
#include <gtk/gtk.h>

VictusKeyboardControl::VictusKeyboardControl(std::shared_ptr<VictusSocketClient> client) : socket_client(client)
{
	keyboard_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_widget_set_margin_top(keyboard_page, 20);
	gtk_widget_set_margin_bottom(keyboard_page, 20);
	gtk_widget_set_margin_start(keyboard_page, 20);
	gtk_widget_set_margin_end(keyboard_page, 20);

	toggle_button = gtk_button_new_with_label("Keyboard: OFF");
	color_chooser = GTK_COLOR_CHOOSER(gtk_color_chooser_widget_new());
	apply_button = gtk_button_new_with_label("Apply");
	current_color_label = GTK_LABEL(gtk_label_new("Current Color: #000000"));
	current_state_label = GTK_LABEL(gtk_label_new("Current State: OFF"));

	gtk_box_append(GTK_BOX(keyboard_page), toggle_button);
	gtk_box_append(GTK_BOX(keyboard_page), GTK_WIDGET(color_chooser));
	gtk_box_append(GTK_BOX(keyboard_page), apply_button);
	gtk_box_append(GTK_BOX(keyboard_page), GTK_WIDGET(current_color_label));
	gtk_box_append(GTK_BOX(keyboard_page), GTK_WIDGET(current_state_label));

	g_signal_connect(toggle_button, "clicked", G_CALLBACK(on_toggle_clicked), this);
	g_signal_connect(color_chooser, "color-set", G_CALLBACK(on_color_set), this);
	g_signal_connect(apply_button, "clicked", G_CALLBACK(on_apply_clicked), this);

	update_current_color_label(this);
}

GtkWidget *VictusKeyboardControl::get_page()
{
	return keyboard_page;
}

void VictusKeyboardControl::update_keyboard_state(bool enabled)
{
	std::string command = enabled ? "1" : "0";
	auto result = socket_client->send_command_async(SET_KEYBOARD_ENABLE, command);

	if (result.get() != "OK")
	{
		std::cerr << "Failed to update keyboard state!" << std::endl;
		return;
	}

	auto keyboard_state = socket_client->send_command_async(GET_KEYBOARD_ENABLE);
	std::string szkeyboard_state = keyboard_state.get();

	if (szkeyboard_state.find("ERROR") == std::string::npos)
	{
		bool current_state = (szkeyboard_state == "1");
		gtk_button_set_label(GTK_BUTTON(toggle_button), current_state ? "Keyboard: ON" : "Keyboard: OFF");

		if (current_state_label)
			gtk_label_set_text(current_state_label, ("Current State: " + (current_state ? std::string("ON") : std::string("OFF"))).c_str());
	}
	else
		std::cerr << "Failed to get current keyboard state!" << std::endl;
}

void VictusKeyboardControl::update_keyboard_color(const std::string &color)
{
	auto color_state = socket_client->send_command_async(SET_RGB, color);
	std::string result = color_state.get();

	if (result != "OK")
		std::cerr << "Failed to update keyboard color!: " << result << std::endl;
}

void VictusKeyboardControl::on_toggle_clicked(GtkWidget *widget, gpointer data)
{
	VictusKeyboardControl *self = static_cast<VictusKeyboardControl *>(data);

	const char *current_label = gtk_button_get_label(GTK_BUTTON(widget));

	bool enabled = std::string(current_label).find("OFF") != std::string::npos;
	self->update_keyboard_state(enabled);
}

void VictusKeyboardControl::on_color_set(GtkColorChooser *widget, gpointer data)
{
	VictusKeyboardControl *self = static_cast<VictusKeyboardControl *>(data);

	GdkRGBA color;
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(self->color_chooser), &color);

	char hex_color[7];
	snprintf(hex_color, sizeof(hex_color), "%02X%02X%02X",
			 (int)(color.red * 255), (int)(color.green * 255), (int)(color.blue * 255));

	self->update_keyboard_color(hex_color);
}

void VictusKeyboardControl::on_apply_clicked(GtkWidget *widget, gpointer data)
{
	VictusKeyboardControl *self = static_cast<VictusKeyboardControl *>(data);

	GdkRGBA color;
	gtk_color_chooser_get_rgba(self->color_chooser, &color);

	char hex_color[7];
	snprintf(hex_color, sizeof(hex_color), "%02X%02X%02X",
			 (int)(color.red * 255), (int)(color.green * 255), (int)(color.blue * 255));

	self->update_keyboard_color(hex_color);

	self->update_current_color_label(self);
}

gboolean VictusKeyboardControl::update_current_color_label(gpointer data)
{
	VictusKeyboardControl *self = static_cast<VictusKeyboardControl *>(data);

	auto current_color = self->socket_client->send_command_async(GET_RGB);

	gtk_label_set_text(self->current_color_label, ("Current Color: " + current_color.get()).c_str());

	return FALSE; // AVSARTODO
}