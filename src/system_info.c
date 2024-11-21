#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function prototypes
// GtkWidget *create_system_info_tab();
void fetch_system_info(char *os_version, char *kernel_version, char *memory, char *cpu_model, char *disk_storage);

// Function to create the System Information tab
GtkWidget *create_system_info_tabb() {
    // Create a grid for layout
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

    // Buffers to hold system information
    char os_version[128] = "Unknown";
    char kernel_version[128] = "Unknown";
    char memory[64] = "Unknown";
    char cpu_model[128] = "Unknown";
    char disk_storage[64] = "Unknown";

    // Fetch system information
    fetch_system_info(os_version, kernel_version, memory, cpu_model, disk_storage);

    // Add labels and values to the grid
    GtkWidget *os_label = gtk_label_new("OS Release Version:");
    GtkWidget *os_value = gtk_label_new(os_version);
    gtk_grid_attach(GTK_GRID(grid), os_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), os_value, 1, 0, 1, 1);

    GtkWidget *kernel_label = gtk_label_new("Kernel Version:");
    GtkWidget *kernel_value = gtk_label_new(kernel_version);
    gtk_grid_attach(GTK_GRID(grid), kernel_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), kernel_value, 1, 1, 1, 1);

    GtkWidget *memory_label = gtk_label_new("Total Memory:");
    GtkWidget *memory_value = gtk_label_new(memory);
    gtk_grid_attach(GTK_GRID(grid), memory_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), memory_value, 1, 2, 1, 1);

    GtkWidget *cpu_label = gtk_label_new("Processor Model:");
    GtkWidget *cpu_value = gtk_label_new(cpu_model);
    gtk_grid_attach(GTK_GRID(grid), cpu_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cpu_value, 1, 3, 1, 1);

    GtkWidget *disk_label = gtk_label_new("Disk Storage:");
    GtkWidget *disk_value = gtk_label_new(disk_storage);
    gtk_grid_attach(GTK_GRID(grid), disk_label, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), disk_value, 1, 4, 1, 1);

    return grid;
}

// Function to fetch system information
void fetch_system_info(char *os_version, char *kernel_version, char *memory, char *cpu_model, char *disk_storage) {
    FILE *fp;
    char buffer[256];

    // Fetch OS version
    fp = popen("lsb_release -d | cut -f2-", "r");
    if (fp) {
        fgets(os_version, 128, fp);
        os_version[strcspn(os_version, "\n")] = '\0'; // Remove newline
        pclose(fp);
    }

    // Fetch Kernel version
    fp = popen("uname -r", "r");
    if (fp) {
        fgets(kernel_version, 128, fp);
        kernel_version[strcspn(kernel_version, "\n")] = '\0';
        pclose(fp);
    }

    // Fetch memory information
    fp = fopen("/proc/meminfo", "r");
    if (fp) {
        while (fgets(buffer, sizeof(buffer), fp)) {
            if (strncmp(buffer, "MemTotal:", 9) == 0) {
                sscanf(buffer, "MemTotal: %s", memory);
                strcat(memory, " kB");
                break;
            }
        }
        fclose(fp);
    }

    // Fetch CPU model
    fp = fopen("/proc/cpuinfo", "r");
    if (fp) {
        while (fgets(buffer, sizeof(buffer), fp)) {
            if (strncmp(buffer, "model name", 10) == 0) {
                sscanf(buffer, "model name : %[^\n]", cpu_model);
                break;
            }
        }
        fclose(fp);
    }

    // Fetch disk storage
    fp = popen("df -h --total | grep total | awk '{print $2 \" total, \" $4 \" available\"}'", "r");
    if (fp) {
        fgets(disk_storage, 64, fp);
        disk_storage[strcspn(disk_storage, "\n")] = '\0';
        pclose(fp);
    }
}
