#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <gtk/gtk.h>

#define NUM_POINTS 100

typedef struct {
    double cpu_usage;       // Percentage of CPU usage
    double memory_usage;    // Percentage of memory usage
    double swap_usage;      // Percentage of swap usage
    double network_in;      // Network received KB/s
    double network_out;     // Network transmitted KB/s
    long total_received;
    long total_sent;

    GtkWidget *cpu_drawing_area;
    GtkWidget *mem_swap_drawing_area;
    GtkWidget *network_drawing_area;
} Metrics;

typedef struct {
    double usages[NUM_POINTS];  // Array to store the CPU usage history
} GraphData;

GtkWidget *create_graph_tab(Metrics *metrics);
void draw_cpu_graph(GtkWidget *widget, cairo_t *cr, gpointer data);
void draw_mem_swap_graph(GtkWidget *widget, cairo_t *cr, gpointer data);
void draw_network_graph(GtkWidget *widget, cairo_t *cr, gpointer data);

double get_cpu_usage(void);
double get_memory_usage(void);
double get_swap_usage(void);
void get_network_usage(double *net_in, double *net_out, long *total_received, long *total_sent);
// void draw_graph(GtkWidget *widget, cairo_t *cr, gpointer data);
gboolean update_metrics(gpointer data);

#endif // TASK_MANAGER_H