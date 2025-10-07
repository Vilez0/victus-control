#ifndef FAN_HPP
#define FAN_HPP

#include <gtk/gtk.h>
#include <string>
#include "socket.hpp"

enum class FanMode { Max, Manual, Auto, COUNT };

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
	GtkWidget *fan1_speed_slider;
	GtkWidget *fan2_speed_slider;

	bool automatic_mode;
	FanMode current_state;

	FanMode str_to_fan_mode(std::string mode);
	void update_fan_mode();
	void update_fan_speeds();
	void update_state_label();
	void update_current_state();

	static void on_toggle_clicked(GtkWidget *widget, gpointer data);
	static void on_apply_clicked(GtkWidget *widget, gpointer data);

    static std::string fan_mode_to_str(FanMode mode) {
        switch (mode) {
            case FanMode::Max:    return "MAX";
            case FanMode::Manual: return "MANUAL";
            default:              return "AUTO";;
        }
    }

	std::shared_ptr<VictusSocketClient> socket_client;
};


inline FanMode& operator++(FanMode& mode) {
    mode = static_cast<FanMode>(
        (static_cast<int>(mode) + 1) % static_cast<int>(FanMode::COUNT)
    );
    return mode;
}


#endif // FAN_HPP
