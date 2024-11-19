#include <gtk/gtk.h>

int main(int argc, char *argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);

    // Create the main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "System Monitor");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    // Connect the "destroy" signal to close the app
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Add a basic label to the window
    GtkWidget *label = gtk_label_new("Welcome to the System Monitor!");
    gtk_container_add(GTK_CONTAINER(window), label);

    // Show all widgets
    gtk_widget_show_all(window);

    // Start the GTK main loop
    gtk_main();

    return 0;
}

