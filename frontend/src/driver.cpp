#include "driver.hpp"

VictusDriverSupport::VictusDriverSupport()
{
    driver_support_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(driver_support_page, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(driver_support_page, GTK_ALIGN_CENTER);

    driver_isnt_installed_label = gtk_label_new("Custom hp-wmi driver is not installed or does not support this system.");
    gtk_box_append(GTK_BOX(driver_support_page), driver_isnt_installed_label);

    driver_install_instructions_label = gtk_label_new("Please visit the following link for installation instructions:");
    gtk_box_append(GTK_BOX(driver_support_page), driver_install_instructions_label);

    driver_website_link = gtk_link_button_new("https://github.com/Vilez0/hp-wmi-fan-and-backlight-control");
    gtk_box_append(GTK_BOX(driver_support_page), driver_website_link);

}

GtkWidget *VictusDriverSupport::get_page()
{
	return driver_support_page;
}
