#ifndef PROCESSES_H
#define PROCESSES_H

#include <gtk/gtk.h>

// Function prototypes
GtkWidget* create_process_tab();
void update_process_list(GtkTreeStore *store);

#endif // PROCESSES_H
