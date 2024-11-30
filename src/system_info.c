#include <gtk/gtk.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

// Function prototypes
GtkWidget *create_system_info_tab();
void fetch_system_info(char *host_name, char *os_version, char *kernel_version, char *memory, char *cpu_model, char *disk_storage, char *system_name);

// Function to create the System Information tab
GtkWidget *create_system_info_tab() {
    // Create a vertical box for layout
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_box_set_homogeneous(GTK_BOX(box), FALSE);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);

    // Add a system information icon
    GtkWidget *icon = gtk_image_new_from_icon_name("face-angry", GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);

    // Create a grid for system information
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

    // Buffers to hold system information
    char host_name[128] = "Unknown";
    char os_version[128] = "Unknown";
    char kernel_version[128] = "Unknown";
    char memory[64] = "Unknown";
    char cpu_model[128] = "Unknown";
    char disk_storage[64] = "Unknown";
    char system_name[128] = "Unknown";

    // Fetch system information
    fetch_system_info(host_name, os_version, kernel_version, memory, cpu_model, disk_storage, system_name);

    GtkWidget *system_section_label = gtk_label_new("System");
    GtkWidget *hardware_section_label = gtk_label_new("Hardware");
    GtkWidget *system_status_label = gtk_label_new("System Status");

    GtkWidget *separator1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    GtkWidget *separator2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    GtkWidget *separator3 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);

    gtk_grid_attach(GTK_GRID(grid), system_section_label, 0, 0, 2, 1);   
    gtk_grid_attach(GTK_GRID(grid), separator1, 0, 0, 2, 1);
    gtk_widget_set_tooltip_text(system_section_label, "System information");
   
    // Add labels and values to the grid
    GtkWidget *system_label = gtk_label_new("System Name:");
    GtkWidget *system_value = gtk_label_new(system_name);
    gtk_grid_attach(GTK_GRID(grid), system_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), system_value, 1, 1, 1, 1);
    gtk_widget_set_tooltip_text(system_label, "System Name");
    gtk_widget_set_tooltip_text(system_value, "The name of the system.");


    GtkWidget *host_label = gtk_label_new("Host Name:");
    GtkWidget *host_value = gtk_label_new(host_name);
    gtk_grid_attach(GTK_GRID(grid), host_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), host_value, 1, 2, 1, 1);
    gtk_widget_set_tooltip_text(host_label, "Host Name");
    gtk_widget_set_tooltip_text(host_value, "The name of the host.");

    GtkWidget *os_label = gtk_label_new("OS Release Version:");
    GtkWidget *os_value = gtk_label_new(os_version);
    gtk_grid_attach(GTK_GRID(grid), os_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), os_value, 1, 3, 1, 1);
    gtk_widget_set_tooltip_text(os_label, "OS Release Version");
    gtk_widget_set_tooltip_text(os_value, "The release version of the operating system.");

    GtkWidget *kernel_label = gtk_label_new("Kernel Version:");
    GtkWidget *kernel_value = gtk_label_new(kernel_version);
    gtk_grid_attach(GTK_GRID(grid), kernel_label, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), kernel_value, 1, 4, 1, 1);
    gtk_widget_set_tooltip_text(kernel_label, "Kernel Version");
    gtk_widget_set_tooltip_text(kernel_value, "The version of the kernel.");

    gtk_grid_attach(GTK_GRID(grid), hardware_section_label, 0, 5, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), separator2, 0, 5, 2, 1);
    gtk_widget_set_tooltip_text(hardware_section_label, "Hardware information");

    GtkWidget *memory_label = gtk_label_new("Total Memory:");
    GtkWidget *memory_value = gtk_label_new(memory);
    gtk_grid_attach(GTK_GRID(grid), memory_label, 0, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), memory_value, 1, 6, 1, 1);
    gtk_widget_set_tooltip_text(memory_label, "Total Memory");
    gtk_widget_set_tooltip_text(memory_value, "The total memory of the system.");

    GtkWidget *cpu_label = gtk_label_new("Processor Model:");
    GtkWidget *cpu_value = gtk_label_new(cpu_model);
    gtk_grid_attach(GTK_GRID(grid), cpu_label, 0, 7, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cpu_value, 1, 7, 1, 1);
    gtk_widget_set_tooltip_text(cpu_label, "Processor Model");
    gtk_widget_set_tooltip_text(cpu_value, "The model name of the processor.");
   
    gtk_grid_attach(GTK_GRID(grid), system_status_label, 0, 8, 2, 1);   
    gtk_grid_attach(GTK_GRID(grid), separator3, 0, 8, 2, 1);
    gtk_widget_set_tooltip_text(system_status_label, "System status information");
    
    GtkWidget *disk_label = gtk_label_new("Disk Storage:");
    GtkWidget *disk_value = gtk_label_new(disk_storage);
    gtk_grid_attach(GTK_GRID(grid), disk_label, 0, 9, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), disk_value, 1, 9, 1, 1);
    gtk_widget_set_tooltip_text(disk_label, "Disk Storage");
    gtk_widget_set_tooltip_text(disk_value, "The storage the disk can hold and has available.");

    // Add the grid to the box
    gtk_box_pack_start(GTK_BOX(box), grid, FALSE, FALSE, 0);

    return box;
}

// Function to fetch system information
void fetch_system_info(char *host_name, char *os_version, char *kernel_version, char *memory, char *cpu_model, char *disk_storage, char *system_name) {
    FILE *fp;
    char buffer[256];
    struct utsname sys_info;

    // Fetch host name
    if (gethostname(host_name, HOST_NAME_MAX) != 0) {
        snprintf(host_name, 128, "Unknown");
    }

    // Fetch system name using uname
    if (uname(&sys_info) == 0) {
        snprintf(system_name, 128, "%s", sys_info.nodename);
    } else {
        snprintf(system_name, 128, "Unknown");
    }

    // Fetch OS version
    fp = popen("lsb_release -d | cut -f2-", "r");
    if (fp) {
        if (fgets(os_version, 128, fp) != NULL) {
            os_version[strcspn(os_version, "\n")] = '\0'; // Remove newline
        } else {
            snprintf(os_version, 128, "Error: Unable to retrieve OS version");
        }
        pclose(fp);
    } else {
        snprintf(os_version, 128, "Error: Unable to execute lsb_release command");
    }

    // Fetch Kernel version
    fp = popen("uname -r", "r");
    if (fp) {
        if (fgets(kernel_version, 128, fp) != NULL) {
            kernel_version[strcspn(kernel_version, "\n")] = '\0';
        } else {
            snprintf(kernel_version, 128, "Error: Unable to retrieve Kernel version");
        }
        pclose(fp);
    } else {
        snprintf(kernel_version, 128, "Error: Unable to execute uname command");
    }

    // Fetch memory information
    fp = fopen("/proc/meminfo", "r");
    if (fp) {
        int found = 0;
        while (fgets(buffer, sizeof(buffer), fp)) {
            if (strncmp(buffer, "MemTotal:", 9) == 0) {
                unsigned long mem_kb;
                if (sscanf(buffer, "MemTotal: %lu", &mem_kb) == 1) {
                    double mem_gib = mem_kb / 1024.0 / 1024.0;
                    snprintf(memory, 64, "%.2f GiB", mem_gib);
                    found = 1;
                    break;
                }
            }
        }
        fclose(fp);
        if (!found) {
            snprintf(memory, 64, "Error: Unable to retrieve memory information");
        }
    } else {
        snprintf(memory, 64, "Error: Unable to open /proc/meminfo");
    }

    // Fetch CPU model
    fp = fopen("/proc/cpuinfo", "r");
    if (fp) {
        int found = 0;
        while (fgets(buffer, sizeof(buffer), fp)) {
            if (strncmp(buffer, "model name", 10) == 0) {
                if (sscanf(buffer, "model name : %[^\n]", cpu_model) == 1) {
                    found = 1;
                    break;
                }
            }
        }
        fclose(fp);
        if (!found) {
            snprintf(cpu_model, 128, "Error: Unable to retrieve CPU model");
        }
    } else {
        snprintf(cpu_model, 128, "Error: Unable to open /proc/cpuinfo");
    }

    // Fetch disk storage
    fp = popen("df -B1 --total | grep total", "r"); // Get raw disk values in bytes
    if (fp) {
        unsigned long total_bytes = 0, available_bytes = 0;
        char temp[256];

        if (fgets(temp, sizeof(temp), fp)) {
            if (sscanf(temp, "%*s %lu %*lu %lu", &total_bytes, &available_bytes) == 2) {
                double total_gib = total_bytes / 1024.0 / 1024.0 / 1024.0;       // Convert total bytes to GiB
                double available_gib = available_bytes / 1024.0 / 1024.0 / 1024.0; // Convert available bytes to GiB

                snprintf(disk_storage, 64, "%.2f GiB total, %.2f GiB available", total_gib, available_gib);
            } else {
                snprintf(disk_storage, 64, "Error: Unable to parse disk storage information");
            }
        } else {
            snprintf(disk_storage, 64, "Error: Unable to read disk storage data");
        }
        pclose(fp);
    } else {
        snprintf(disk_storage, 64, "Error: Unable to execute df command");
    }
}

