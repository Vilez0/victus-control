#ifndef DRIVER_HPP
#define DRIVER_HPP

#include <gtk/gtk.h>

class VictusDriverSupport
{
public:
    GtkWidget *driver_support_page;
    GtkWidget *driver_isnt_installed_label;
    GtkWidget *driver_install_instructions_label;
    GtkWidget *driver_website_link;

	VictusDriverSupport();

	GtkWidget *get_page();
};

#endif // DRIVER_HPP