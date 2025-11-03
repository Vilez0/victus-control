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

	kbd_brightness_slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 255, 1);
	gtk_widget_set_hexpand(kbd_brightness_slider, true);
	kbd_brightness_label = GTK_LABEL(gtk_label_new("Keyboard Brightness: 0%"));
	kbd_brightness = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append(GTK_BOX(kbd_brightness), kbd_brightness_slider);
	gtk_box_append(GTK_BOX(kbd_brightness), GTK_WIDGET(kbd_brightness_label));


	color_chooser = GTK_COLOR_CHOOSER(gtk_color_chooser_widget_new());
	apply_button = gtk_button_new_with_label("Apply");
	gtk_widget_set_hexpand(apply_button, true);
	current_color_label = GTK_LABEL(gtk_label_new("Current Color: #000000"));
	current_brightness_label = GTK_LABEL(gtk_label_new("Current Brightness: 0%"));

	gtk_box_append(GTK_BOX(keyboard_page), GTK_WIDGET(kbd_brightness));
	gtk_box_append(GTK_BOX(keyboard_page), GTK_WIDGET(color_chooser));
	gtk_box_append(GTK_BOX(keyboard_page), GTK_WIDGET(apply_button));
	gtk_box_append(GTK_BOX(keyboard_page), GTK_WIDGET(current_color_label));
	gtk_box_append(GTK_BOX(keyboard_page), GTK_WIDGET(current_brightness_label));

	g_signal_connect(kbd_brightness_slider, "value-changed", G_CALLBACK(on_brightness_changed), this);
	g_signal_connect(color_chooser, "color-set", G_CALLBACK(on_color_set), this);
	g_signal_connect(apply_button, "clicked", G_CALLBACK(on_apply_clicked), this);

	update_keyboard_state_from_device();
	update_current_color_label(this);
}

bool VictusKeyboardControl::is_keyboard_backlight_supported() const
{
	auto response = socket_client->send_command_async(GET_DRIVER_SUPPORT_FLAGS);
	std::string flags = response.get();
	return (flags.find("KBD_BACKLIGHT_SUPPORTED") != std::string::npos);
}

GtkWidget *VictusKeyboardControl::get_page()
{
	if (!is_keyboard_backlight_supported())
	{
		return nullptr;
	}
	return keyboard_page;
}

void VictusKeyboardControl::update_keyboard_state_from_device()
{
	auto keyboard_state = socket_client->send_command_async(GET_KBD_BRIGHTNESS);
	std::string szkeyboard_state = keyboard_state.get();

	if (szkeyboard_state.find("ERROR") == std::string::npos)
	{
		int brightness = std::stoi(szkeyboard_state);
		if (kbd_brightness_slider) {
			gtk_range_set_value(GTK_RANGE(kbd_brightness_slider), brightness);
			gtk_label_set_text(kbd_brightness_label, ("Keyboard Brightness: " + std::to_string((brightness * 100) / 255) + "%").c_str());
		}

		if (current_brightness_label)
			gtk_label_set_text(current_brightness_label, ("Current Brightness: " + std::to_string((brightness * 100) / 255) + "%").c_str());

	}
	else
		std::cerr << "Failed to get current keyboard state!" << std::endl;
}

void VictusKeyboardControl::update_keyboard_brightness(const int brightness) {
	printf("Setting brightness to %d\n", brightness);
	auto brightness_state = socket_client->send_command_async(SET_KBD_BRIGHTNESS, std::to_string(brightness));
	std::string result = brightness_state.get();

	if (result != "OK")
		std::cerr << "Failed to update keyboard brightness!: " << result << std::endl;
}

void VictusKeyboardControl::update_keyboard_color(const GdkRGBA &color)
{
	// auto color_string = gdk_rgba_to_string(&color);
	auto value = std::to_string((int)(color.red * 255)) + " " +
				  std::to_string((int)(color.green * 255)) + " " +
				  std::to_string((int)(color.blue * 255));
	auto color_state = socket_client->send_command_async(SET_KEYBOARD_COLOR, value);
	std::string result = color_state.get();

	if (result != "OK")
		std::cerr << "Failed to update keyboard color!: " << result << std::endl;
}

void VictusKeyboardControl::on_brightness_changed(GtkRange *range, gpointer data)
{
	VictusKeyboardControl *self = static_cast<VictusKeyboardControl *>(data);
	int brightness = static_cast<int>(gtk_range_get_value(range));
	gtk_label_set_text(self->kbd_brightness_label, ("Keyboard Brightness: " + std::to_string((brightness * 100) / 255) + "%").c_str());
}

void VictusKeyboardControl::on_color_set(GtkColorChooser *widget, gpointer data)
{
	VictusKeyboardControl *self = static_cast<VictusKeyboardControl *>(data);

	GdkRGBA color;
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(self->color_chooser), &color);

	self->update_keyboard_color(color);
}

void VictusKeyboardControl::on_apply_clicked(GtkWidget *widget, gpointer data)
{
	VictusKeyboardControl *self = static_cast<VictusKeyboardControl *>(data);

	GdkRGBA color;
	gtk_color_chooser_get_rgba(self->color_chooser, &color);

	self->update_keyboard_color(color);

	int brightness = static_cast<int>(gtk_range_get_value(GTK_RANGE(self->kbd_brightness_slider)));
	self->update_keyboard_brightness(brightness);

	self->update_keyboard_state_from_device();

	self->update_current_color_label(self);
}

void VictusKeyboardControl::update_current_color_label(gpointer data)
{
	VictusKeyboardControl *self = static_cast<VictusKeyboardControl *>(data);

	auto current_color = self->socket_client->send_command_async(GET_KEYBOARD_COLOR);
	std::string szcurrent_color = current_color.get();

	gtk_label_set_text(self->current_color_label, ("Current Color: " + szcurrent_color).c_str());
}