// file_system.h
#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <gtk/gtk.h>

typedef struct file_info
{
    char *device;
    char *mount_point;
    char *type;
    char *total;
    char *free;
    char *available;
    char *used;
} file_info;

// Function prototypes
file_info *get_file_info();
GtkWidget* create_file_system_tab();

#endif // FILE_SYSTEM_H