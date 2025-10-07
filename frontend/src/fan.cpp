#include "fan.hpp"
#include <iostream>

VictusFanControl::VictusFanControl(std::shared_ptr<VictusSocketClient> client) : socket_client(client)
{
	fan_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_widget_set_margin_top(fan_page, 20);
	gtk_widget_set_margin_bottom(fan_page, 20);
	gtk_widget_set_margin_start(fan_page, 20);
	gtk_widget_set_margin_end(fan_page, 20);

	update_current_state();

	toggle_button = gtk_button_new_with_label(("Set Fan: " + fan_mode_to_str(current_state)).c_str());
	gtk_box_append(GTK_BOX(fan_page), toggle_button);
	g_signal_connect(toggle_button, "clicked", G_CALLBACK(on_toggle_clicked), this);

	apply_button = gtk_button_new_with_label("Apply");
	gtk_box_append(GTK_BOX(fan_page), apply_button);
	g_signal_connect(apply_button, "clicked", G_CALLBACK(on_apply_clicked), this);

	state_label = gtk_label_new(("Current State: " + fan_mode_to_str(current_state)).c_str());
	gtk_widget_set_halign(state_label, GTK_ALIGN_START);
	gtk_box_append(GTK_BOX(fan_page), state_label);

	fan1_speed_label = gtk_label_new("Fan1 Speed: N/A RPM");
	gtk_widget_set_halign(fan1_speed_label, GTK_ALIGN_START);
	gtk_box_append(GTK_BOX(fan_page), fan1_speed_label);

	fan2_speed_label = gtk_label_new("Fan2 Speed: N/A RPM");
	gtk_widget_set_halign(fan2_speed_label, GTK_ALIGN_START);
	gtk_box_append(GTK_BOX(fan_page), fan2_speed_label);

	update_state_label();
	update_fan_speeds();

	g_timeout_add_seconds(2, [](gpointer data) -> gboolean {
		VictusFanControl *self = static_cast<VictusFanControl *>(data);
		self->update_fan_speeds();
		return TRUE;
	}, this);
}

GtkWidget *VictusFanControl::get_page()
{
	return fan_page;
}

void VictusFanControl::update_current_state()
{
	auto response = socket_client->send_command_async(GET_FAN_MODE);
	std::string fan_state = response.get();

	if (fan_state == "AUTO" || fan_state == "MANUAL" ||fan_state == "MAX")
	{
		current_state = str_to_fan_mode(fan_state);
		automatic_mode = (fan_state == "AUTO");
	}
	else
	{
		std::cerr << "Error while getting fan information!" << fan_state << std::endl;
		current_state = FanMode::Auto;
		automatic_mode = true;
	}
}

void VictusFanControl::update_fan_mode()
{
	std::string state = fan_mode_to_str(++current_state);
	gtk_button_set_label(GTK_BUTTON(toggle_button), ("Set Fan: " + state).c_str());
}

void VictusFanControl::update_fan_speeds()
{
	auto response1 = socket_client->send_command_async(GET_FAN_SPEED, "1");
	std::string fan1_speed = response1.get();

	auto response2 = socket_client->send_command_async(GET_FAN_SPEED, "2");
	std::string fan2_speed = response2.get();

	std::string fan1_text = "Fan1 Speed: " + fan1_speed + " RPM";
	std::string fan2_text = "Fan2 Speed: " + fan2_speed + " RPM";

	gtk_label_set_text(GTK_LABEL(fan1_speed_label), fan1_text.c_str());
	gtk_label_set_text(GTK_LABEL(fan2_speed_label), fan2_text.c_str());
}

void VictusFanControl::update_state_label()
{
	std::string text = "Current State: " + fan_mode_to_str(current_state);

	gtk_label_set_text(GTK_LABEL(state_label), text.c_str());
}

void VictusFanControl::on_toggle_clicked(GtkWidget *widget, gpointer data)
{
	VictusFanControl *self = static_cast<VictusFanControl *>(data);
	self->update_fan_mode();
}

void VictusFanControl::on_apply_clicked(GtkWidget *widget, gpointer data)
{
	VictusFanControl *self = static_cast<VictusFanControl *>(data);

	std::string command = fan_mode_to_str(self->current_state);

	auto response = self->socket_client->send_command_async(SET_FAN_MODE, command);
	std::string response2 = response.get();

	if (response2 == "OK")
		self->current_state = self->str_to_fan_mode(command);
	else
		std::cerr << "Failed to change fan mode!" << response2 << std::endl;

	self->update_state_label();
}

FanMode VictusFanControl::str_to_fan_mode(std::string mode)
{
	if (mode == "MAX")
		return FanMode::Max;
	else if (mode == "MANUAL")
		return FanMode::Manual;
	else
		return FanMode::Auto;
}
