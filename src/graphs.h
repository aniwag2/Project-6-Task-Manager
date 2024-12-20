#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <gtk/gtk.h>

#define NUM_POINTS 100

typedef struct {
    double cpu_usage;
    double memory_usage;
    long memory_used;
    long memory_total;
    double swap_usage;
    long swap_used;
    long swap_total;
    double network_in;
    double network_out;
    long total_received;
    long total_sent;

    GtkWidget *cpu_drawing_area;
    GtkWidget *mem_swap_drawing_area;
    GtkWidget *network_drawing_area;
} Metrics;

typedef struct {
    double usages[NUM_POINTS];
} GraphData;

GtkWidget *create_graph_tab(Metrics *metrics);
void draw_cpu_graph(GtkWidget *widget, cairo_t *cr);
void draw_mem_swap_graph(GtkWidget *widget, cairo_t *cr);
void draw_network_graph(GtkWidget *widget, cairo_t *cr);

double get_cpu_usage(void);
void get_memory_usage(double *memory_usage, long *mem_used, long *mem_total);
void get_swap_usage(double *swap_usage, long *swap_used, long *swap_total);
void get_network_usage(double *net_in, double *net_out, long *total_received, long *total_sent);

gboolean update_metrics(gpointer data);

#endif // TASK_MANAGER_H