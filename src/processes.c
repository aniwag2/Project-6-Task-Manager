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
#include <sys/sysinfo.h>
#include <time.h>
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
    process_data->treeview = GTK_TREE_VIEW(treeview); // Set the treeview in ProcessData

    // Connect signals
    g_signal_connect(refresh_button, "clicked", G_CALLBACK(on_refresh_button_clicked), process_data);
    g_signal_connect(user_processes_toggle, "toggled", G_CALLBACK(on_refresh_button_clicked), process_data);

    // Add right-click context menu
    g_signal_connect(treeview, "button-press-event", G_CALLBACK(on_treeview_right_click), treeview);

    // Add double-click for detailed view
    g_signal_connect(treeview, "row-activated", G_CALLBACK(on_row_activated), NULL);

    // Update the process list initially
    update_process_list(process_data);

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
    GtkTreeView *treeview = process_data->treeview;

    gboolean show_user_only = gtk_toggle_button_get_active(toggle_button);
    uid_t current_uid = getuid(); // Current user's UID

    // Record expanded nodes
    GtkTreeModel *model = GTK_TREE_MODEL(store);
    GList *expanded_pids = NULL;

    // Function to recursively collect expanded nodes
    void collect_expanded_nodes(GtkTreeModel *model, GtkTreeIter *iter) {
        do {
            GtkTreePath *path = gtk_tree_model_get_path(model, iter);
            if (gtk_tree_view_row_expanded(treeview, path)) {
                guint pid;
                gtk_tree_model_get(model, iter, 0, &pid, -1);
                expanded_pids = g_list_prepend(expanded_pids, GUINT_TO_POINTER(pid));

                // Recurse into child nodes
                if (gtk_tree_model_iter_has_child(model, iter)) {
                    GtkTreeIter child_iter;
                    if (gtk_tree_model_iter_children(model, &child_iter, iter)) {
                        collect_expanded_nodes(model, &child_iter);
                    }
                }
            }
            gtk_tree_path_free(path);
        } while (gtk_tree_model_iter_next(model, iter));
    }

    // Start from the root
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(model, &iter)) {
        collect_expanded_nodes(model, &iter);
    }

    // Clear the existing list
    gtk_tree_store_clear(store);

    // Hash table to store PID to iter mapping
    GHashTable *pid_to_iter = g_hash_table_new(g_direct_hash, g_direct_equal);

    // Open /proc directory
    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) {
        g_warning("Failed to open /proc directory");
        return;
    }

    struct dirent *entry;
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
                if (fscanf(stat_file, "%*u %*s %*c %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %ld %ld", &utime, &stime) < 2) {
                    // Handle error if necessary
                }
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

    // Restore expanded nodes
    // Function to recursively expand nodes if their PID is in the expanded_pids list
    void expand_nodes(GtkTreeModel *model, GtkTreeIter *iter) {
        do {
            guint pid;
            gtk_tree_model_get(model, iter, 0, &pid, -1);
            if (g_list_find(expanded_pids, GUINT_TO_POINTER(pid))) {
                GtkTreePath *path = gtk_tree_model_get_path(model, iter);
                gtk_tree_view_expand_row(treeview, path, FALSE);
                gtk_tree_path_free(path);

                // Recurse into child nodes
                if (gtk_tree_model_iter_has_child(model, iter)) {
                    GtkTreeIter child_iter;
                    if (gtk_tree_model_iter_children(model, &child_iter, iter)) {
                        expand_nodes(model, &child_iter);
                    }
                }
            }
        } while (gtk_tree_model_iter_next(model, iter));
    }

    // Start from the root
    if (gtk_tree_model_get_iter_first(model, &iter)) {
        expand_nodes(model, &iter);
    }

    // Free the expanded_pids list
    g_list_free(expanded_pids);

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
    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    char details[2048];
    get_process_details(pid, details, sizeof(details));

    gtk_text_buffer_set_text(buffer, details, -1);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
    gtk_widget_set_size_request(scrolled_window, 400, 300);

    gtk_container_add(GTK_CONTAINER(content_area), scrolled_window);

    gtk_widget_show_all(dialog);
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
}

void get_process_details(guint pid, char *details, size_t size) {
    char path[512], line[4096];
    char name[256] = "Unknown", state_char = 'U', state[32] = "Unknown", user[64] = "Unknown";
    unsigned long vsize = 0, rss = 0, shared_mem = 0;
    long utime = 0, stime = 0;
    unsigned long long starttime_ticks = 0;
    uid_t uid = 0;

    // Get user ID and other info from /proc/[pid]/status
    snprintf(path, sizeof(path), "/proc/%u/status", pid);
    FILE *status_file = fopen(path, "r");
    if (status_file) {
        while (fgets(line, sizeof(line), status_file)) {
            if (sscanf(line, "Name:\t%255s", name) == 1)
                continue;
            if (sscanf(line, "State:\t%c", &state_char) == 1)
                continue;
            if (sscanf(line, "Uid:\t%u", &uid) == 1)
                continue;
            if (sscanf(line, "VmSize:\t%lu kB", &vsize) == 1)
                continue;
            if (sscanf(line, "VmRSS:\t%lu kB", &rss) == 1)
                continue;
            if (sscanf(line, "RssFile:\t%lu kB", &shared_mem) == 1)
                continue;
            // For systems where 'RssFile' is not available, you can also try 'VmShared'
            if (sscanf(line, "VmShared:\t%lu kB", &shared_mem) == 1)
                continue;
        }
        fclose(status_file);
    }

    // Map state_char to full state description
    switch (state_char) {
        case 'R':
            strcpy(state, "Running");
            break;
        case 'S':
            strcpy(state, "Sleeping");
            break;
        case 'D':
            strcpy(state, "Waiting");
            break;
        case 'Z':
            strcpy(state, "Zombie");
            break;
        case 'T':
        case 't':
            strcpy(state, "Stopped");
            break;
        case 'W':
            strcpy(state, "Paging");
            break;
        case 'X':
            strcpy(state, "Dead");
            break;
        case 'I':
            strcpy(state, "Idle");
            break;
        default:
            strcpy(state, "Unknown");
            break;
    }

    // Get user name
    struct passwd *pwd = getpwuid(uid);
    if (pwd) {
        strncpy(user, pwd->pw_name, sizeof(user) - 1);
        user[sizeof(user) - 1] = '\0';
    } else {
        snprintf(user, sizeof(user), "%u", uid);
    }

    // Get CPU times and start time from /proc/[pid]/stat
    snprintf(path, sizeof(path), "/proc/%u/stat", pid);
    FILE *stat_file = fopen(path, "r");
    if (stat_file) {
        if (fgets(line, sizeof(line), stat_file) != NULL) {
            // Extract the required fields
            // The comm field can contain spaces and is enclosed in parentheses
            char *start = strchr(line, '(');
            char *end = strrchr(line, ')');
            if (start && end && end > start) {
                // Extract comm (process name)
                size_t comm_len = end - start - 1;
                strncpy(name, start + 1, comm_len);
                name[comm_len] = '\0';

                // Move past the comm field to the rest of the data
                char *rest = end + 2; // Skip past ") "
                char *token;
                int field = 3; // Fields start from position 3 after comm

                char *saveptr;
                token = strtok_r(rest, " ", &saveptr);
                while (token != NULL) {
                    if (field == 14) {
                        utime = atol(token);
                    } else if (field == 15) {
                        stime = atol(token);
                    } else if (field == 22) {
                        starttime_ticks = atoll(token);
                        break; // We have all the data we need
                    }
                    token = strtok_r(NULL, " ", &saveptr);
                    field++;
                }
            }
        }
        fclose(stat_file);
    }

    // Convert start time to human-readable format
    struct sysinfo s_info;
    sysinfo(&s_info);

    time_t boot_time = time(NULL) - s_info.uptime;
    time_t start_time = boot_time + (starttime_ticks / sysconf(_SC_CLK_TCK));

    char start_time_str[64];
    strftime(start_time_str, sizeof(start_time_str), "%Y-%m-%d %H:%M:%S", localtime(&start_time));

    // Calculate CPU time
    long total_time = (utime + stime) / sysconf(_SC_CLK_TCK);

    snprintf(details, size,
             "Process Name: %s\n"
             "User: %s\n"
             "State: %s\n"
             "CPU Time: %ld s\n"
             "Started At: %s\n"
             "Memory: %lu kB\n"
             "Virtual Memory: %lu kB\n"
             "Resident Memory: %lu kB\n"
             "Shared Memory: %lu kB\n",
             name,
             user,
             state,
             total_time,
             start_time_str,
             rss,         // Using RSS as "Memory"
             vsize,
             rss,
             shared_mem);
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

