#include "fan.hpp"
#include <iostream>
#include <cmath>

VictusFanControl::VictusFanControl(std::shared_ptr<VictusSocketClient> client) : socket_client(client)
{
	fan_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
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

	fan1_slider_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	// connect a signal to fan1 and fan2 sliders to update their values
	fan1_speed_slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 65, 1);
	gtk_widget_set_hexpand(fan1_speed_slider, TRUE);
	fan1_label_slider = gtk_label_new("Fan1 Speed: 0 RPM");
	gtk_box_append(GTK_BOX(fan1_slider_box), fan1_speed_slider);
	gtk_box_append(GTK_BOX(fan1_slider_box), fan1_label_slider);
	gtk_box_append(GTK_BOX(fan_page), fan1_slider_box);
	gtk_widget_set_visible(fan1_slider_box, (current_state == FanMode::Manual)); // hide the manual sliders by default
	g_signal_connect(fan1_speed_slider, "value-changed", G_CALLBACK(on_fan1_speed_changed), this);

	fan2_slider_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	fan2_speed_slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 65, 1);
	gtk_widget_set_hexpand(fan2_speed_slider, TRUE);
	fan2_label_slider = gtk_label_new("Fan2 Speed: 0 RPM");
	gtk_box_append(GTK_BOX(fan2_slider_box), fan2_speed_slider);
	gtk_box_append(GTK_BOX(fan2_slider_box), fan2_label_slider);
	gtk_box_append(GTK_BOX(fan_page), fan2_slider_box);
	gtk_widget_set_visible(fan2_slider_box, (current_state == FanMode::Manual)); // hide the manual sliders by default
	g_signal_connect(fan2_speed_slider, "value-changed", G_CALLBACK(on_fan2_speed_changed), this);

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

void VictusFanControl::on_fan1_speed_changed(GtkRange *range, gpointer data)
{
	VictusFanControl *self = static_cast<VictusFanControl *>(data);
	const long fan1_speed = std::lround(gtk_range_get_value(range)) * 100;
	gtk_label_set_text(GTK_LABEL(self->fan1_label_slider), ("Fan1 Speed: " + std::to_string(fan1_speed) + " RPM").c_str());
}

void VictusFanControl::on_fan2_speed_changed(GtkRange *range, gpointer data)
{
	VictusFanControl *self = static_cast<VictusFanControl *>(data);
	const long fan2_speed = std::lround(gtk_range_get_value(range)) * 100;
	gtk_label_set_text(GTK_LABEL(self->fan2_label_slider), ("Fan2 Speed: " + std::to_string(fan2_speed) + " RPM").c_str());
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
	bool manual_mode = (state == "MANUAL");
	gtk_widget_set_visible(fan1_slider_box, manual_mode);
	gtk_widget_set_visible(fan2_slider_box, manual_mode);
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

	if (response2 == "OK") {
		self->current_state = self->str_to_fan_mode(command);
		if (self->current_state == FanMode::Manual) {
			long fan1_speed = std::lround(gtk_range_get_value(GTK_RANGE(self->fan1_speed_slider))) * 100;
			long fan2_speed = std::lround(gtk_range_get_value(GTK_RANGE(self->fan2_speed_slider))) * 100;

			auto resp1 = self->socket_client->send_command_async(SET_FAN_SPEED, "1 " + std::to_string(fan1_speed));
			std::string resp1_str = resp1.get();
			if (resp1_str != "OK") {
				std::cerr << "Failed to set manual fan speeds!" << resp1_str << std::endl;
			}

			auto resp2 = self->socket_client->send_command_async(SET_FAN_SPEED, "2 " + std::to_string(fan2_speed));
			std::string resp2_str = resp2.get();
			if (resp2_str != "OK") {
				std::cerr << "Failed to set manual fan speeds!" << resp2_str << std::endl;
			}
		}
	} else
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
