// file_system.c
#include "file_system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/statfs.h>
#include <errno.h>
#endif

int g_num_devices = 0;
file_info *g_file_info = NULL;

file_info *get_file_info() {
    // Reset device count
    g_num_devices = 0;

    // Free existing file_info if any
    if (g_file_info != NULL) {
        for(int i = 0; i < g_num_devices; i++) {
            free(g_file_info[i].device);
            free(g_file_info[i].mount_point);
            free(g_file_info[i].type);
            free(g_file_info[i].total);
            free(g_file_info[i].free);
            free(g_file_info[i].available);
            free(g_file_info[i].used);
        }
        free(g_file_info);
        g_file_info = NULL;
    }

#ifdef _WIN32
    DWORD drive_mask = GetLogicalDrives();
    for(int i = 0; i < 26; i++) {
        if (drive_mask & (1 << i)) {
            char drive_letter[4] = { 'A' + i, ':', '\\', '\0' };
            UINT drive_type = GetDriveTypeA(drive_letter);
            const char *type;
            switch(drive_type) {
                case DRIVE_REMOVABLE: type = "Removable"; break;
                case DRIVE_FIXED: type = "Fixed"; break;
                case DRIVE_REMOTE: type = "Network"; break;
                case DRIVE_CDROM: type = "CD-ROM"; break;
                case DRIVE_RAMDISK: type = "RAM Disk"; break;
                default: type = "Unknown"; break;
            }

            ULARGE_INTEGER free_bytes, total_bytes, available_bytes;
            if (GetDiskFreeSpaceExA(drive_letter, &available_bytes, &total_bytes, &free_bytes)) {
                unsigned long long total = total_bytes.QuadPart;
                unsigned long long free = free_bytes.QuadPart;
                unsigned long long available = available_bytes.QuadPart;
                unsigned long long used = total - free;

                char total_str[32], free_str[32], available_str[32], used_str[32];
                snprintf(total_str, sizeof(total_str), "%llu", total);
                snprintf(free_str, sizeof(free_str), "%llu", free);
                snprintf(available_str, sizeof(available_str), "%llu", available);
                snprintf(used_str, sizeof(used_str), "%llu", used);

                file_info new_file = {0};
                new_file.device = strdup(drive_letter);
                new_file.mount_point = strdup(drive_letter);
                new_file.type = strdup(type);
                new_file.total = strdup(total_str);
                new_file.free = strdup(free_str);
                new_file.available = strdup(available_str);
                new_file.used = strdup(used_str);

                // Add new device to the list
                file_info *temp = realloc(g_file_info, sizeof(file_info) * (g_num_devices + 1));
                if (temp == NULL) {
                    perror("realloc");
                    exit(EXIT_FAILURE);
                }
                g_file_info = temp;
                g_file_info[g_num_devices] = new_file;
                g_num_devices++;
            }
        }
    }
#else
    FILE *mounts_file = fopen("/proc/mounts", "r");
    if (mounts_file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, mounts_file) != -1) {
        file_info new_file = {0};

        // device name
        char *field = strtok(line, " ");
        if (field == NULL) continue;
        new_file.device = strdup(field);

        // mount point
        field = strtok(NULL, " ");
        if (field == NULL) { free(new_file.device); continue; }
        new_file.mount_point = strdup(field);

        // filesystem type
        field = strtok(NULL, " ");
        if (field == NULL) { 
            free(new_file.device); 
            free(new_file.mount_point); 
            continue; 
        }
        new_file.type = strdup(field);

        // Get filesystem statistics
        struct statfs fs_stats;
        if (statfs(new_file.mount_point, &fs_stats) == -1) {
            new_file.total = strdup("0");
            new_file.free = strdup("0");
            new_file.available = strdup("0");
            new_file.used = strdup("0");
        } else {
            unsigned long total = fs_stats.f_blocks * fs_stats.f_frsize;
            unsigned long free = fs_stats.f_bfree * fs_stats.f_frsize;
            unsigned long available = fs_stats.f_bavail * fs_stats.f_frsize;
            unsigned long used = total - free;

            char buffer[32];

            // Total
            snprintf(buffer, sizeof(buffer), "%lu", total);
            new_file.total = strdup(buffer);

            // Free
            snprintf(buffer, sizeof(buffer), "%lu", free);
            new_file.free = strdup(buffer);

            // Available
            snprintf(buffer, sizeof(buffer), "%lu", available);
            new_file.available = strdup(buffer);

            // Used
            snprintf(buffer, sizeof(buffer), "%lu", used);
            new_file.used = strdup(buffer);
        }

        // Add new device to the list
        file_info *temp = realloc(g_file_info, sizeof(file_info) * (g_num_devices + 1));
        if (temp == NULL) {
            perror("realloc");
            fclose(mounts_file);
            free(line);
            exit(EXIT_FAILURE);
        }
        g_file_info = temp;
        g_file_info[g_num_devices] = new_file;
        g_num_devices++;
    }

    fclose(mounts_file);
    if (line) free(line);
#endif

    return g_file_info;
}

GtkWidget* create_file_system_tab() {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    // Create a TreeView with a TreeStore
    GtkTreeStore *store = gtk_tree_store_new(7, 
        G_TYPE_STRING, // Device
        G_TYPE_STRING, // Mount Point
        G_TYPE_STRING, // Type
        G_TYPE_STRING, // Total
        G_TYPE_STRING, // Free
        G_TYPE_STRING, // Available
        G_TYPE_STRING  // Used
    );
    GtkWidget *treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

    // Insert columns
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "Device", gtk_cell_renderer_text_new(), "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "Directory", gtk_cell_renderer_text_new(), "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "Type", gtk_cell_renderer_text_new(), "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "Total", gtk_cell_renderer_text_new(), "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "Free", gtk_cell_renderer_text_new(), "text", 4, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "Available", gtk_cell_renderer_text_new(), "text", 5, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "Used", gtk_cell_renderer_text_new(), "text", 6, NULL);

    // Populate the TreeStore with file system information
    file_info *files = get_file_info();
    for(int i = 0; i < g_num_devices; i++) {
        GtkTreeIter iter;
        gtk_tree_store_append(store, &iter, NULL);
        gtk_tree_store_set(store, &iter,
            0, files[i].device,
            1, files[i].mount_point,
            2, files[i].type,
            3, files[i].total,
            4, files[i].free,
            5, files[i].available,
            6, files[i].used,
            -1);
    }

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    return vbox;
}