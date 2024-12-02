#ifndef PROCESSES_H
#define PROCESSES_H

#include <gtk/gtk.h>

// Define the ProcessData struct to hold shared data
typedef struct {
    GtkTreeStore *store;
    GtkToggleButton *toggle_button;
    GtkTreeView *treeview;
} ProcessData;

// Function prototypes
GtkWidget* create_process_tab();
void on_refresh_button_clicked(GtkButton *button, gpointer user_data);
gboolean update_process_list_timeout(gpointer data);
void update_process_list(ProcessData *process_data);
void on_treeview_right_click(GtkWidget *treeview, GdkEventButton *event, gpointer data);
void on_row_activated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data);
void show_detailed_view(guint pid);
void get_process_details(guint pid, char *details, size_t size);
void stop_process_action(GtkMenuItem *item, gpointer data);
void continue_process_action(GtkMenuItem *item, gpointer data);
void kill_process_action(GtkMenuItem *item, gpointer data);
void list_memory_maps_action(GtkMenuItem *item, gpointer data);
void list_open_files_action(GtkMenuItem *item, gpointer data);

#endif // PROCESSES_H
