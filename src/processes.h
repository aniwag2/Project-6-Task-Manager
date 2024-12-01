#ifndef PROCESSES_H
#define PROCESSES_H

#include <gtk/gtk.h>

GtkWidget* create_process_tab();
void update_process_list(GtkTreeStore *store);
void on_treeview_right_click(GtkWidget *treeview, GdkEventButton *event, gpointer data);
void on_row_activated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data);
void show_detailed_view(guint pid);
void get_process_details(guint pid, char *details, size_t size);
void stop_process_action(GtkMenuItem *item, gpointer data);
void continue_process_action(GtkMenuItem *item, gpointer data);
void kill_process_action(GtkMenuItem *item, gpointer data);

#endif // PROCESSES_H
