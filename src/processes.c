#include "processes.h"
#include <dirent.h>
#include <errno.h>
#include <glib.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

GtkWidget* create_process_tab() {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    // Create a TreeView with a TreeStore
    GtkTreeStore *store = gtk_tree_store_new(5, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

    // Set selection mode to single selection
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

    // Insert columns
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "PID", gtk_cell_renderer_text_new(), "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "Name", gtk_cell_renderer_text_new(), "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "User", gtk_cell_renderer_text_new(), "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "State", gtk_cell_renderer_text_new(), "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "CPU Time", gtk_cell_renderer_text_new(), "text", 4, NULL);

    // Add treeview to the layout
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    // Add buttons and toggle
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    // Toggle to show processes owned by current user
    GtkWidget *user_processes_toggle = gtk_check_button_new_with_label("Show only my processes");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(user_processes_toggle), FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), user_processes_toggle, FALSE, FALSE, 0);

    // Refresh button
    GtkWidget *refresh_button = gtk_button_new_with_label("Refresh");
    gtk_box_pack_start(GTK_BOX(hbox), refresh_button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    // Create the ProcessData struct
    ProcessData *process_data = g_malloc(sizeof(ProcessData));
    process_data->store = store;
    process_data->toggle_button = GTK_TOGGLE_BUTTON(user_processes_toggle);

    // Connect signals
    g_signal_connect(refresh_button, "clicked", G_CALLBACK(on_refresh_button_clicked), process_data);
    g_signal_connect(user_processes_toggle, "toggled", G_CALLBACK(on_refresh_button_clicked), process_data);

    // Add right-click context menu
    g_signal_connect(treeview, "button-press-event", G_CALLBACK(on_treeview_right_click), treeview);

    // Add double-click for detailed view
    g_signal_connect(treeview, "row-activated", G_CALLBACK(on_row_activated), NULL);

    // Update the process list initially
    update_process_list(process_data);

    // Set up periodic refresh every 5 seconds
    g_timeout_add_seconds(5, update_process_list_timeout, process_data);

    return vbox;
}

void on_refresh_button_clicked(GtkButton *button, gpointer user_data) {
    (void)button; // Mark as unused
    ProcessData *process_data = (ProcessData *)user_data;
    update_process_list(process_data);
}

gboolean update_process_list_timeout(gpointer data) {
    ProcessData *process_data = (ProcessData *)data;
    update_process_list(process_data);
    return TRUE; // Continue calling this function
}

void update_process_list(ProcessData *process_data) {
    GtkTreeStore *store = process_data->store;
    GtkToggleButton *toggle_button = process_data->toggle_button;

    gboolean show_user_only = gtk_toggle_button_get_active(toggle_button);
    uid_t current_uid = getuid(); // Current user's UID

    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) return;

    struct dirent *entry;
    gtk_tree_store_clear(store); // Clear the existing list

    // Hash table to store PID to iter mapping
    GHashTable *pid_to_iter = g_hash_table_new(g_direct_hash, g_direct_equal);

    while ((entry = readdir(proc_dir)) != NULL) {
        if (entry->d_type == DT_DIR && atoi(entry->d_name) > 0) {
            unsigned int pid = atoi(entry->d_name);
            char path[512];

            // Get process information from /proc/[pid]/stat
            snprintf(path, sizeof(path), "/proc/%u/stat", pid);
            FILE *stat_file = fopen(path, "r");
            if (!stat_file) continue;

            unsigned int ppid;
            char comm[256], state_char;
            if (fscanf(stat_file, "%u (%255[^)]) %c %u", &pid, comm, &state_char, &ppid) < 4) {
                fclose(stat_file);
                continue;
            }
            fclose(stat_file);

            // Get user ID of the process
            snprintf(path, sizeof(path), "/proc/%u", pid);
            struct stat st;
            if (stat(path, &st) != 0) continue;

            if (show_user_only && st.st_uid != current_uid) {
                continue; // Skip processes not owned by current user
            }

            // Get user name
            struct passwd *pwd = getpwuid(st.st_uid);
            char user[64];
            if (pwd) {
                strcpy(user, pwd->pw_name);
            } else {
                snprintf(user, sizeof(user), "%u", st.st_uid);
            }

            // Get CPU time
            snprintf(path, sizeof(path), "/proc/%u/stat", pid);
            stat_file = fopen(path, "r");
            long utime = 0, stime = 0;
            if (stat_file) {
                //char unused[1024];
                fscanf(stat_file, "%*u %*s %*c %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %ld %ld", &utime, &stime);
                fclose(stat_file);
            }
            long total_time = (utime + stime) / sysconf(_SC_CLK_TCK);
            char cpu_time[32];
            snprintf(cpu_time, sizeof(cpu_time), "%ld s", total_time);

            // Convert state to string
            char state_str[16];
            switch (state_char) {
                case 'R':
                    strcpy(state_str, "Running");
                    break;
                case 'S':
                    strcpy(state_str, "Sleeping");
                    break;
                case 'D':
                    strcpy(state_str, "Disk Sleep");
                    break;
                case 'Z':
                    strcpy(state_str, "Zombie");
                    break;
                case 'T':
                    strcpy(state_str, "Stopped");
                    break;
                case 't':
                    strcpy(state_str, "Tracing Stop");
                    break;
                case 'X':
                    strcpy(state_str, "Dead");
                    break;
                case 'I':
                    strcpy(state_str, "Idle");
                    break;
                default:
                    strcpy(state_str, "Unknown");
                    break;
            }

            // Find parent iter
            GtkTreeIter *parent_iter = g_hash_table_lookup(pid_to_iter, GUINT_TO_POINTER(ppid));

            // Add to the TreeStore
            GtkTreeIter *new_iter = g_new(GtkTreeIter, 1);
            gtk_tree_store_append(store, new_iter, parent_iter);
            gtk_tree_store_set(store, new_iter,
                               0, pid,
                               1, comm,
                               2, user,
                               3, state_str,
                               4, cpu_time,
                               -1);

            // Store this iter for child processes
            g_hash_table_insert(pid_to_iter, GUINT_TO_POINTER(pid), new_iter);
        }
    }

    closedir(proc_dir);

    // Free the hash table
    GHashTableIter hash_iter;
    gpointer key, value;
    g_hash_table_iter_init(&hash_iter, pid_to_iter);
    while (g_hash_table_iter_next(&hash_iter, &key, &value)) {
        g_free(value);
    }
    g_hash_table_destroy(pid_to_iter);
}

void on_treeview_right_click(GtkWidget *treeview, GdkEventButton *event, gpointer data) {
  	(void)data;

    if (event->type == GDK_BUTTON_PRESS && event->button == 3) { // Right-click
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
        GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
        GtkTreeIter iter;

        guint pid = 0;
        if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
            gtk_tree_model_get(model, &iter, 0, &pid, -1); // Fetch the selected PID
        } else {
            return;
        }

        // Create the context menu
        GtkWidget *menu = gtk_menu_new();

        GtkWidget *stop_item = gtk_menu_item_new_with_label("Stop");
        GtkWidget *continue_item = gtk_menu_item_new_with_label("Continue");
        GtkWidget *kill_item = gtk_menu_item_new_with_label("Kill");
        GtkWidget *list_memory_maps_item = gtk_menu_item_new_with_label("List Memory Maps");
        GtkWidget *list_open_files_item = gtk_menu_item_new_with_label("List Open Files");

        // Pass the selected PID to the callbacks
        g_signal_connect_data(stop_item, "activate", G_CALLBACK(stop_process_action), GUINT_TO_POINTER(pid), NULL, 0);
        g_signal_connect_data(continue_item, "activate", G_CALLBACK(continue_process_action), GUINT_TO_POINTER(pid), NULL, 0);
        g_signal_connect_data(kill_item, "activate", G_CALLBACK(kill_process_action), GUINT_TO_POINTER(pid), NULL, 0);
        g_signal_connect_data(list_memory_maps_item, "activate", G_CALLBACK(list_memory_maps_action), GUINT_TO_POINTER(pid), NULL, 0);
        g_signal_connect_data(list_open_files_item, "activate", G_CALLBACK(list_open_files_action), GUINT_TO_POINTER(pid), NULL, 0);

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), stop_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), continue_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), kill_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), list_memory_maps_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), list_open_files_item);

        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
    }
}

void on_row_activated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data) {
    (void)column; // Mark as unused
    (void)data;
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(model, &iter, path)) {
        guint pid;
        gtk_tree_model_get(model, &iter, 0, &pid, -1); // Get PID from the row
        show_detailed_view(pid);
    }
}

void show_detailed_view(guint pid) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Process Details",
        NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *label = gtk_label_new(NULL);

    char details[4096];
    get_process_details(pid, details, sizeof(details));

    gtk_label_set_text(GTK_LABEL(label), details);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_container_add(GTK_CONTAINER(content_area), label);

    gtk_widget_show_all(dialog);
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
}

void get_process_details(guint pid, char *details, size_t size) {
    char path[512];
    snprintf(path, sizeof(path), "/proc/%u/status", pid);

    FILE *file = fopen(path, "r");
    if (file) {
        char line[256];
        snprintf(details, size, "Process Details:\n\n");

        while (fgets(line, sizeof(line), file)) {
            strncat(details, line, size - strlen(details) - 1);
        }
        fclose(file);
    } else {
        snprintf(details, size, "Unable to fetch details for PID %u", pid);
    }
}

void stop_process_action(GtkMenuItem *item, gpointer data) {
    (void)item; // Mark as unused
    guint pid = GPOINTER_TO_UINT(data);

    if (pid > 0) {
        if (kill(pid, SIGSTOP) == 0) {
            g_print("Stopped process with PID %u\n", pid);
        } else {
            g_print("Failed to stop process with PID %u: %s\n", pid, strerror(errno));
        }
    } else {
        g_print("Invalid PID\n");
    }
}

void continue_process_action(GtkMenuItem *item, gpointer data) {
    (void)item; // Mark as unused
    guint pid = GPOINTER_TO_UINT(data);

    if (pid > 0) {
        if (kill(pid, SIGCONT) == 0) {
            g_print("Continued process with PID %u\n", pid);
        } else {
            g_print("Failed to continue process with PID %u: %s\n", pid, strerror(errno));
        }
    } else {
        g_print("Invalid PID\n");
    }
}

void kill_process_action(GtkMenuItem *item, gpointer data) {
    (void)item; // Mark as unused
    guint pid = GPOINTER_TO_UINT(data);

    if (pid > 0) {
        if (kill(pid, SIGKILL) == 0) {
            g_print("Killed process with PID %u\n", pid);
        } else {
            g_print("Failed to kill process with PID %u: %s\n", pid, strerror(errno));
        }
    } else {
        g_print("Invalid PID\n");
    }
}

void list_memory_maps_action(GtkMenuItem *item, gpointer data) {
    (void)item;
    guint pid = GPOINTER_TO_UINT(data);

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Memory Maps",
        NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    char path[512];
    snprintf(path, sizeof(path), "/proc/%u/maps", pid);

    FILE *file = fopen(path, "r");
    if (file) {
        char line[256];
        GtkTextIter iter;
        gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
        while (fgets(line, sizeof(line), file)) {
            gtk_text_buffer_insert(buffer, &iter, line, -1);
        }
        fclose(file);
    } else {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Unable to fetch memory maps for PID %u: %s", pid, strerror(errno));
        gtk_text_buffer_set_text(buffer, error_msg, -1);
    }

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
    gtk_widget_set_size_request(scrolled_window, 600, 400);

    gtk_container_add(GTK_CONTAINER(content_area), scrolled_window);

    gtk_widget_show_all(dialog);
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
}

void list_open_files_action(GtkMenuItem *item, gpointer data) {
    (void)item;
    guint pid = GPOINTER_TO_UINT(data);

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Open Files",
        NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    char path[512];
    snprintf(path, sizeof(path), "/proc/%u/fd", pid);

    DIR *dir = opendir(path);
    if (dir) {
        struct dirent *entry;
        GtkTextIter iter;
        gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue; // Skip . and ..
            char fd_path[512];
            snprintf(fd_path, sizeof(fd_path), "/proc/%u/fd/%s", pid, entry->d_name);
            char link_target[512];
            ssize_t len = readlink(fd_path, link_target, sizeof(link_target) - 1);
            if (len != -1) {
                link_target[len] = '\0';
                char line[1024];
                snprintf(line, sizeof(line), "%s -> %s\n", entry->d_name, link_target);
                gtk_text_buffer_insert(buffer, &iter, line, -1);
            }
        }
        closedir(dir);
    } else {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Unable to fetch open files for PID %u: %s", pid, strerror(errno));
        gtk_text_buffer_set_text(buffer, error_msg, -1);
    }

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
    gtk_widget_set_size_request(scrolled_window, 600, 400);

    gtk_container_add(GTK_CONTAINER(content_area), scrolled_window);

    gtk_widget_show_all(dialog);
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
}

