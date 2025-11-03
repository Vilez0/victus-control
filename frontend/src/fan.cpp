#include "fan.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <sstream>

VictusFanControl::VictusFanControl(std::shared_ptr<VictusSocketClient> client) : socket_client(client)
{
	fan_page_scrollable = gtk_scrolled_window_new();

	fan_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
	gtk_widget_set_margin_top(fan_page, 20);
	gtk_widget_set_margin_bottom(fan_page, 20);
	gtk_widget_set_margin_start(fan_page, 20);
	gtk_widget_set_margin_end(fan_page, 20);

	update_current_state();

	toggle_button = gtk_button_new_with_label(("Set Fan: " + fan_mode_to_str(current_state)).c_str());
	gtk_box_append(GTK_BOX(fan_page), toggle_button);
	g_signal_connect(toggle_button, "clicked", G_CALLBACK(on_toggle_clicked), this);

	switch_curve_sliders_button = gtk_button_new_with_label("Switch to Curve");
	gtk_box_append(GTK_BOX(fan_page), switch_curve_sliders_button);
	gtk_widget_set_visible(switch_curve_sliders_button, (current_state == FanMode::Manual)); // show the button only in manual mode
	g_signal_connect(switch_curve_sliders_button, "clicked", G_CALLBACK(on_switch_curve_sliders_clicked), this);

	fan1_slider_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	auto resp = std::stoi((socket_client->send_command_async(GET_FAN_MAX_SPEED, "1")).get());
	int fan1_max_speed = (resp != 0) ? resp : 4500;
	fan1_speed_slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, fan1_max_speed/100, 1);
	gtk_widget_set_hexpand(fan1_speed_slider, TRUE);
	fan1_label_slider = gtk_label_new("Fan1 Speed: 0 RPM");
	gtk_box_append(GTK_BOX(fan1_slider_box), fan1_speed_slider);
	gtk_box_append(GTK_BOX(fan1_slider_box), fan1_label_slider);
	g_signal_connect(fan1_speed_slider, "value-changed", G_CALLBACK(on_fan1_speed_changed), this);

	fan2_slider_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	resp = std::stoi((socket_client->send_command_async(GET_FAN_MAX_SPEED, "2")).get());
	int fan2_max_speed = (resp != 0) ? resp : fan1_max_speed;
	fan2_speed_slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, fan2_max_speed/100, 1);
	gtk_widget_set_hexpand(fan2_speed_slider, TRUE);
	fan2_label_slider = gtk_label_new("Fan2 Speed: 0 RPM");
	gtk_box_append(GTK_BOX(fan2_slider_box), fan2_speed_slider);
	gtk_box_append(GTK_BOX(fan2_slider_box), fan2_label_slider);
	g_signal_connect(fan2_speed_slider, "value-changed", G_CALLBACK(on_fan2_speed_changed), this);

	fans_slider_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_box_append(GTK_BOX(fans_slider_box), fan1_slider_box);
	gtk_box_append(GTK_BOX(fans_slider_box), fan2_slider_box);
	gtk_widget_set_visible(fans_slider_box, (current_state == FanMode::Manual)); // hide the manual sliders by default
	gtk_box_append(GTK_BOX(fan_page), fans_slider_box);
	g_signal_connect(fans_slider_box, "show", G_CALLBACK(populate_fan_sliders_box), this);

	/*
		Fan Curve UI will be like a table:
		TEMP : FAN1_SPEED : FAN2_SPEED : + (add new point button)
		XX (input) : XX (input) : XX (input) :  - (remove point button)
	*/

	fan_curve_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_widget_set_visible(fan_curve_box, false);
	// populate curve box when visible
	g_signal_connect(fan_curve_box, "show", G_CALLBACK(populate_curve_box), this);
	fan_curve_label_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_set_homogeneous(GTK_BOX(fan_curve_label_box), TRUE);

	fan_curve_temp_label = gtk_label_new("CPU Temperature (°C)");
	fan_curve_fan1_speed_label = gtk_label_new("Fan1 Speed (%)");
	fan_curve_fan2_speed_label = gtk_label_new("Fan2 Speed (%)");
	fan_curve_add_button = gtk_button_new_from_icon_name("list-add-symbolic");
	g_signal_connect(fan_curve_add_button, "clicked", G_CALLBACK(on_fan_curve_add_button_clicked), this);

	gtk_box_append(GTK_BOX(fan_curve_label_box), fan_curve_temp_label);
	gtk_box_append(GTK_BOX(fan_curve_label_box), fan_curve_fan1_speed_label);
	gtk_box_append(GTK_BOX(fan_curve_label_box), fan_curve_fan2_speed_label);
	gtk_box_append(GTK_BOX(fan_curve_label_box), fan_curve_add_button);

	gtk_box_append(GTK_BOX(fan_curve_box), fan_curve_label_box);
	gtk_box_append(GTK_BOX(fan_page), fan_curve_box);

	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(fan_page_scrollable), fan_page);

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
	return fan_page_scrollable;
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

	std::string fans_curve = socket_client->send_command_async(GET_FANS_CURVE).get();
	if (fans_curve != "" && fans_curve != "1/;2/" && manual_mode) {
		gtk_widget_set_visible(fan_curve_box, true);
		gtk_widget_set_visible(fans_slider_box, false);
	} else if (manual_mode) {
		gtk_widget_set_visible(fan_curve_box, false);
		gtk_widget_set_visible(fans_slider_box, true);
	} else {
		gtk_widget_set_visible(fan_curve_box, false);
		gtk_widget_set_visible(fans_slider_box, false);
	}

	gtk_widget_set_visible(switch_curve_sliders_button, manual_mode);
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
			if (gtk_widget_is_visible(GTK_WIDGET(self->fans_slider_box)) != false) {
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
			} else if (gtk_widget_is_visible(GTK_WIDGET(self->fan_curve_box)) != false) {
				std::string curve = self->extract_curve_string(self);
				auto resp = self->socket_client->send_command_async(SET_FANS_CURVE, curve);
				std::string resp_str = resp.get();
				if (resp_str != "OK") {
					std::cerr << "Failed to set fan curve!" << resp_str << std::endl;
				}
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

void VictusFanControl::on_fan_curve_add_button_clicked(GtkWidget *widget, gpointer data)
{
	VictusFanControl *self = static_cast<VictusFanControl *>(data);

	GtkWidget *new_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_set_homogeneous(GTK_BOX(new_row), TRUE);
	GtkWidget *temp_entry = gtk_spin_button_new_with_range(0, 120, 1);
	g_signal_connect(temp_entry, "value-changed", G_CALLBACK(sort_curve_rows), self);

	GtkWidget *fan1_entry = gtk_spin_button_new_with_range(0, 100, 1);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(fan1_entry), true);

	GtkWidget *fan2_entry = gtk_spin_button_new_with_range(0, 100, 1);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(fan2_entry), true);

	GtkWidget *remove_button = gtk_button_new_from_icon_name("list-remove-symbolic");
	g_signal_connect(remove_button, "clicked", G_CALLBACK(on_fan_curve_remove_button_clicked), self);
	gtk_box_append(GTK_BOX(new_row), temp_entry);
	gtk_box_append(GTK_BOX(new_row), fan1_entry);
	gtk_box_append(GTK_BOX(new_row), fan2_entry);
	gtk_box_append(GTK_BOX(new_row), remove_button);
	gtk_box_append(GTK_BOX(self->fan_curve_box), new_row);
}

void VictusFanControl::on_fan_curve_remove_button_clicked(GtkWidget *widget, gpointer data)
{
	VictusFanControl *self = static_cast<VictusFanControl *>(data);
	GtkWidget *row = gtk_widget_get_parent(widget);
	gtk_box_remove(GTK_BOX(self->fan_curve_box), row);
}

void VictusFanControl::sort_curve_rows(GtkWidget *widget, gpointer data)
{
	VictusFanControl *self = static_cast<VictusFanControl *>(data);
	GtkWidget *curve_box = self->fan_curve_box;
	std::vector<std::pair<int, GtkWidget*>> rows;

	for (GtkWidget *child = gtk_widget_get_first_child(curve_box); child != nullptr; ) {
		if (child != self->fan_curve_label_box) {
			GtkWidget *temp_entry = gtk_widget_get_first_child(child);
			int temp_value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(temp_entry));
			rows.emplace_back(temp_value, child);
		}
		child = gtk_widget_get_next_sibling(child);
	}
	std::sort(rows.begin(), rows.end(), [](const auto &a, const auto &b) {
		return a.first > b.first;
	});

	for (const auto &[temp, row] : rows) {
		gtk_box_reorder_child_after(GTK_BOX(curve_box), row, nullptr);
	}

	// move the label row back to the top
	gtk_box_reorder_child_after(GTK_BOX(curve_box), self->fan_curve_label_box, nullptr);
}

std::string VictusFanControl::extract_curve_string(gpointer data)
{
	VictusFanControl *self = static_cast<VictusFanControl *>(data);
	std::string fan1_curve = "1/";
	std::string fan2_curve = "2/";

	for (GtkWidget *child = gtk_widget_get_first_child(self->fan_curve_box); child != nullptr; ) {
		if (child != self->fan_curve_label_box) {
			GtkWidget *temp_entry = gtk_widget_get_first_child(child);
			GtkWidget *fan1_entry = gtk_widget_get_next_sibling(temp_entry);
			GtkWidget *fan2_entry = gtk_widget_get_next_sibling(fan1_entry);

			int temp_value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(temp_entry));
			int fan1_value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(fan1_entry));
			int fan2_value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(fan2_entry));

			fan1_curve += std::to_string(temp_value) + ":" + std::to_string(fan1_value) + ",";
			fan2_curve += std::to_string(temp_value) + ":" + std::to_string(fan2_value) + ",";
		}
		child = gtk_widget_get_next_sibling(child);
	}

	return fan1_curve.substr(0, fan1_curve.size() - 1) + ";" + fan2_curve.substr(0, fan2_curve.size() - 1); // remove trailing commas
}

void VictusFanControl::populate_curve_box(GtkWidget *widget, gpointer data)
{
	VictusFanControl *self = static_cast<VictusFanControl *>(data);

	auto response = self->socket_client->send_command_async(GET_FANS_CURVE);
	std::string curve_str = response.get();

	// Example expected format: "1/30:20,60:50,90:80;2/30:25,60:55,90:85"
	size_t separator_pos = curve_str.find(';');
	if (separator_pos == std::string::npos) {
		std::cerr << "Invalid fan curve format received: " << curve_str << std::endl;
		return;
	}

	std::string fan1_curve = curve_str.substr(0, separator_pos);
	std::string fan2_curve = curve_str.substr(separator_pos + 1);

	auto parse_curve = [](const std::string &curve) {
		std::vector<std::pair<int, int>> points;
		size_t slash_pos = curve.find('/');
		if (slash_pos == std::string::npos) return points;


		std::istringstream ss(curve.substr(slash_pos + 1));
		std::string segment;
		while (std::getline(ss, segment, ',')) {
			size_t colon_pos = segment.find(':');
			if (colon_pos == std::string::npos) continue;

			int temp = std::stoi(segment.substr(0, colon_pos));
			int speed = std::stoi(segment.substr(colon_pos + 1));
			points.emplace_back(temp, speed);
		}
		return points;
	};

	auto fan1_points = parse_curve(fan1_curve);
	auto fan2_points = parse_curve(fan2_curve);

	for (GtkWidget *row = gtk_widget_get_first_child(self->fan_curve_box);
	     row != nullptr;) {
		GtkWidget *next = gtk_widget_get_next_sibling(row); // skip header row
		if (row != self->fan_curve_label_box) {
			gtk_box_remove(GTK_BOX(self->fan_curve_box), row);
		}
		row = next;
	}

	size_t num_points = std::max(fan1_points.size(), fan2_points.size());
	for (size_t i = 0; i < num_points; ++i) {
		self->on_fan_curve_add_button_clicked(nullptr, self);
	}

	std::vector<GtkWidget *> data_rows;
	for (GtkWidget *row = gtk_widget_get_first_child(self->fan_curve_box);
		 row != nullptr;
		 row = gtk_widget_get_next_sibling(row)) {
		if (row != self->fan_curve_label_box) {
			data_rows.push_back(row);
		}
	}

	for (size_t i = 0; i < num_points && i < data_rows.size(); ++i) {
		GtkWidget *row = data_rows[i];
		GtkWidget *temp_entry = gtk_widget_get_first_child(row);
		GtkWidget *fan1_entry = gtk_widget_get_next_sibling(temp_entry);
		GtkWidget *fan2_entry = gtk_widget_get_next_sibling(fan1_entry);
		if (i < fan1_points.size()) {
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(temp_entry), fan1_points[i].first);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(fan1_entry), fan1_points[i].second);
		}
		if (i < fan2_points.size()) {
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(fan2_entry), fan2_points[i].second);
		}
	}
}

void VictusFanControl::populate_fan_sliders_box(GtkWidget *widget, gpointer data) 
{
	VictusFanControl *self = static_cast<VictusFanControl *>(data);

	auto response1 = self->socket_client->send_command_async(GET_FAN_SPEED, "1");
	std::string fan1_speed_str = response1.get();
	int fan1_speed = std::stoi(fan1_speed_str);

	auto response2 = self->socket_client->send_command_async(GET_FAN_SPEED, "2");
	std::string fan2_speed_str = response2.get();
	int fan2_speed = std::stoi(fan2_speed_str);

	gtk_range_set_value(GTK_RANGE(self->fan1_speed_slider), fan1_speed / 100.0);
	gtk_range_set_value(GTK_RANGE(self->fan2_speed_slider), fan2_speed / 100.0);
}

void VictusFanControl::on_switch_curve_sliders_clicked(GtkWidget *widget, gpointer data)
{
	VictusFanControl *self = static_cast<VictusFanControl *>(data);
	bool is_curve_visible = gtk_widget_get_visible(self->fan_curve_box);
	gtk_widget_set_visible(self->fan_curve_box, !is_curve_visible);
	gtk_widget_set_visible(self->fans_slider_box, is_curve_visible && (self->current_state == FanMode::Manual));

	if (is_curve_visible) {
		gtk_button_set_label(GTK_BUTTON(self->switch_curve_sliders_button), "Switch to Curve");
	} else {
		gtk_button_set_label(GTK_BUTTON(self->switch_curve_sliders_button), "Switch to Sliders");
	}
}