#include "processes.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h> // For sysconf()

GtkWidget* create_process_tab() {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    // Create a TreeView with a TreeStore
    GtkTreeStore *store = gtk_tree_store_new(5, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "PID", gtk_cell_renderer_text_new(), "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "Name", gtk_cell_renderer_text_new(), "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "User", gtk_cell_renderer_text_new(), "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "State", gtk_cell_renderer_text_new(), "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "CPU Time", gtk_cell_renderer_text_new(), "text", 4, NULL);

    // Add treeview to the layout
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    // Add buttons
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *refresh_button = gtk_button_new_with_label("Refresh");
    g_signal_connect(refresh_button, "clicked", G_CALLBACK(update_process_list), store);
    gtk_box_pack_start(GTK_BOX(hbox), refresh_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    // Update the process list initially
    update_process_list(store);

    return vbox;
}

void update_process_list(GtkTreeStore *store) {
    if (!GTK_IS_TREE_STORE(store)) {
        g_critical("Invalid GtkTreeStore passed to update_process_list");
        return;
    }

    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) return;

    struct dirent *entry;
    GtkTreeIter iter;
    gtk_tree_store_clear(store); // Clear the existing list

    while ((entry = readdir(proc_dir)) != NULL) {
        if (entry->d_type == DT_DIR && atoi(entry->d_name) > 0) {
            char path[512];
            snprintf(path, sizeof(path), "/proc/%s/status", entry->d_name);

            FILE *status_file = fopen(path, "r");
            if (!status_file) continue;

            // Extract process information
            char line[256], name[128] = "", state[32] = "", user[64] = "", cpu_time[32] = "";
            unsigned int pid = atoi(entry->d_name);

            while (fgets(line, sizeof(line), status_file)) {
                if (sscanf(line, "Name: %s", name) == 1) continue;
                if (sscanf(line, "State: %s", state) == 1) continue;
            }
            fclose(status_file);

            // Get CPU time from /proc/[pid]/stat
            snprintf(path, sizeof(path), "/proc/%s/stat", entry->d_name);
            FILE *stat_file = fopen(path, "r");
            if (stat_file) {
                long utime = 0, stime = 0;
                fscanf(stat_file, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %ld %ld",
                       &utime, &stime);
                fclose(stat_file);

                // Convert jiffies to seconds
                long total_time = (utime + stime) / sysconf(_SC_CLK_TCK);
                snprintf(cpu_time, sizeof(cpu_time), "%ld s", total_time);
            } else {
                snprintf(cpu_time, sizeof(cpu_time), "N/A");
            }

            // Get the user from UID
            snprintf(path, sizeof(path), "/proc/%s", entry->d_name);
            struct stat st;
            if (stat(path, &st) == 0) {
                struct passwd *pwd = getpwuid(st.st_uid);
                if (pwd) strcpy(user, pwd->pw_name);
            }

            // Add to the TreeStore
            gtk_tree_store_append(store, &iter, NULL);
            gtk_tree_store_set(store, &iter,
                               0, pid,
                               1, name,
                               2, user,
                               3, state,
                               4, cpu_time, // Add CPU time here
                               -1);
        }
    }
    closedir(proc_dir);
}

void stop_process(GtkWidget *widget, gpointer pid) {
    (void)widget; // Mark as unused
    kill((pid_t)(uintptr_t)pid, SIGSTOP);
}

void continue_process(GtkWidget *widget, gpointer pid) {
    (void)widget; // Mark as unused
    kill((pid_t)(uintptr_t)pid, SIGCONT);
}

void kill_process(GtkWidget *widget, gpointer pid) {
    (void)widget; // Mark as unused
    kill((pid_t)(uintptr_t)pid, SIGKILL);
}