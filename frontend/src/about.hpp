#ifndef ABOUT_HPP
#define ABOUT_HPP

#include <gtk/gtk.h>

class VictusAbout
{
public:
	VictusAbout();
	void show_about_window(GtkWindow *parent);

private:
	GtkWidget *about_dialog;
};

#endif // ABOUT_HPP