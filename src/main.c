#include <gtk/gtk.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h> // Added for struct stat and stat()

#include "processes.h"

// #include "system_info.c"

extern GtkWidget *create_system_info_tab();
extern void fetch_system_info(char *os_version, char *kernel_version, char *memory, char *cpu_model, char *disk_storage);

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Create main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "System Monitor");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Create notebook
    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(window), notebook);

    // Add System tab
    GtkWidget *system_info_tab = create_system_info_tab();
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), system_info_tab, gtk_label_new("System Info"));


    // Add Process tab
    GtkWidget *process_tab = create_process_tab();
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), process_tab, gtk_label_new("Processes"));

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}

