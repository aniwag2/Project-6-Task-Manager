
#include <gtk/gtk.h>
#include <cairo.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "graphs.h"

GraphData cpu_data = {{0}};
GraphData memo_data = {{0}};
GraphData netin_data = {{0}};

void format_bytes(long bytes, char *output) {
    const char *units[] = {"Bytes", "KiB", "MiB", "GiB", "TiB"};
    int unit_index = 0;
    double size = bytes;

    while (size >= 1024 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }

    sprintf(output, "%.2f %s", size, units[unit_index]);
}

GtkWidget *create_graph_tab(Metrics *metrics) {
    GtkWidget *graph_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    GtkWidget *cpu_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(cpu_area, 800, 100);
    gtk_box_pack_start(GTK_BOX(graph_tab), cpu_area, TRUE, TRUE, 0);
    g_object_set_data(G_OBJECT(cpu_area), "metrics", metrics);
    g_signal_connect(cpu_area, "draw", G_CALLBACK(draw_cpu_graph), metrics);

    metrics->cpu_drawing_area = cpu_area;

    GtkWidget *mem_swap_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(mem_swap_area, 800, 100);
    gtk_box_pack_start(GTK_BOX(graph_tab), mem_swap_area, TRUE, TRUE, 0);
    g_object_set_data(G_OBJECT(mem_swap_area), "metrics", metrics);
    g_signal_connect(mem_swap_area, "draw", G_CALLBACK(draw_mem_swap_graph), metrics);

    metrics->mem_swap_drawing_area = mem_swap_area;

    GtkWidget *network_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(network_area, 800, 100);
    gtk_box_pack_start(GTK_BOX(graph_tab), network_area, TRUE, TRUE, 0);
    g_object_set_data(G_OBJECT(network_area), "metrics", metrics);
    g_signal_connect(network_area, "draw", G_CALLBACK(draw_network_graph), metrics);

    metrics->network_drawing_area = network_area;

    // GtkWidget *drawing_area = gtk_drawing_area_new();

    // // Set size for the drawing area
    // gtk_widget_set_size_request(drawing_area, 800, 400);
    // gtk_box_pack_start(GTK_BOX(graph_tab), drawing_area, TRUE, TRUE, 0);

    // // Associate Metrics with the drawing area
    // g_object_set_data(G_OBJECT(drawing_area), "metrics", metrics);

    // // Connect the draw signal to render the graphs
    // g_signal_connect(drawing_area, "draw", G_CALLBACK(draw_graph), metrics);

    // Start a timeout to update metrics and refresh the drawing area
    g_timeout_add(1000, update_metrics, metrics);
    // g_timeout_add(1000, update_metrics, mem_swap_area);


    return graph_tab;
}

gboolean update_metrics(gpointer data) {
    Metrics *metrics = (Metrics *)data;

    if (!metrics) return FALSE;

    // Update the metrics
    metrics->cpu_usage = get_cpu_usage();
    metrics->memory_usage = get_memory_usage();
    metrics->swap_usage = get_swap_usage();    
    get_network_usage(&metrics->network_in, &metrics->network_out, &metrics->total_received, &metrics->total_sent);

    // Update the historical data
    memmove(cpu_data.usages, cpu_data.usages + 1, (NUM_POINTS - 1) * sizeof(double));
    cpu_data.usages[NUM_POINTS - 1] = metrics->cpu_usage;

    memmove(memo_data.usages, memo_data.usages + 1, (NUM_POINTS - 1) * sizeof(double));
    memo_data.usages[NUM_POINTS - 1] = metrics->memory_usage;

    memmove(netin_data.usages, netin_data.usages + 1, (NUM_POINTS - 1) * sizeof(double));
    netin_data.usages[NUM_POINTS - 1] = metrics->network_in;

    // Redraw all associated drawing areas
    gtk_widget_queue_draw(metrics->cpu_drawing_area);
    gtk_widget_queue_draw(metrics->mem_swap_drawing_area);
    gtk_widget_queue_draw(metrics->network_drawing_area);

    return TRUE; // Keep the timeout running
}


void draw_cpu_graph(GtkWidget *widget, cairo_t *cr, gpointer data) {
    Metrics *metrics = g_object_get_data(G_OBJECT(widget), "metrics");
    if (!metrics) return;

    // Set background to white
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    cairo_set_line_width(cr, 2); 
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_rectangle(cr, 50, 50, 500, 150);
    cairo_stroke(cr);

    // Set line width for the graph line
    cairo_set_line_width(cr, 2); 

    // Set the color for the CPU usage line (green)
    cairo_set_source_rgb(cr, 0, 1, 0);

    // Plot the line graph based on historical data
    int width = 500;
    int height = 200;
    double x_step = (double)width / NUM_POINTS;

    // Start the path at the first point in the history 
    cairo_move_to(cr, 50, (height - cpu_data.usages[0] * height / 100));

    // Loop through the data points and create the line graph
    for (int i = 1; i < NUM_POINTS; i++) {
        int x = 50 + (i * x_step);
        int y = height - cpu_data.usages[i] * height / 100;
        // printf("Current cpu y: %d\n", y);
        cairo_line_to(cr, x, y);
    }

    // Draw the line graph
    cairo_stroke(cr);

    // Display CPU usage text
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, 50, 20);
    cairo_show_text(cr, "CPU Usage");
    cairo_move_to(cr, 50, 40);
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.2f%%", metrics->cpu_usage);
    cairo_show_text(cr, buffer);
}

void draw_mem_swap_graph(GtkWidget *widget, cairo_t *cr, gpointer data) {
    Metrics *metrics = g_object_get_data(G_OBJECT(widget), "metrics");
    if (!metrics) return;

    cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
    cairo_paint(cr);

    
    cairo_set_line_width(cr, 2); 
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_rectangle(cr, 50, 50, 500, 150);
    cairo_stroke(cr);

    // Set line width for the graph line
    cairo_set_line_width(cr, 2); 

    cairo_set_source_rgb(cr, 0, 0, 1);

    // Plot the line graph based on historical data
    int width = 500;
    int height = 200;
    double x_step = (double)width / NUM_POINTS;

    // Start the path at the first point in the history 
    cairo_move_to(cr, 50, (height - memo_data.usages[0] * height / 100));

    // Loop through the data points and create the line graph
    for (int i = 1; i < NUM_POINTS; i++) {
        int x = 50 + (i * x_step);
        int y = height - memo_data.usages[i] * height / 100;
        cairo_line_to(cr, x, y);
    }

    // Draw the line graph
    cairo_stroke(cr);


    // // Memory usage bar graph
    // cairo_set_source_rgb(cr, 0, 0, 1);
    // cairo_rectangle(cr, 50, 180 - metrics->memory_usage * 1.8, 50, metrics->memory_usage * 1.8);
    // cairo_fill(cr);

    // // Swap usage bar graph
    // cairo_set_source_rgb(cr, 1, 0, 0);
    // cairo_rectangle(cr, 150, 180 - metrics->swap_usage * 1.8, 50, metrics->swap_usage * 1.8);
    // cairo_fill(cr);

    // Memory and Swap usage text
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, 50, 20);
    cairo_show_text(cr, "Memory Usage:");
    cairo_move_to(cr, 50, 40);
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.2f%%", metrics->memory_usage);
    cairo_show_text(cr, buffer);

    // cairo_move_to(cr, 220, 90);
    // cairo_show_text(cr, "Swap Usage:");
    // cairo_move_to(cr, 220, 110);
    // snprintf(buffer, sizeof(buffer), "%.2f%%", metrics->swap_usage);
    // cairo_show_text(cr, buffer);
}

void draw_network_graph(GtkWidget *widget, cairo_t *cr, gpointer data) {
    Metrics *metrics = g_object_get_data(G_OBJECT(widget), "metrics");
    if (!metrics) return;

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    cairo_set_line_width(cr, 2); 
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_rectangle(cr, 50, 50, 500, 150);
    cairo_stroke(cr);

    // Set line width for the graph line
    cairo_set_line_width(cr, 2); 

    cairo_set_source_rgb(cr, 1, 0, 0);

    // Plot the line graph based on historical data
    int width = 500;
    int height = 200;
    double x_step = (double)width / NUM_POINTS;

    // Start the path at the first point in the history 
    cairo_move_to(cr, 50, (height - netin_data.usages[0] * height / 200000 / 1024));

    // Loop through the data points and create the line graph
    for (int i = 1; i < NUM_POINTS; i++) {
        int x = 50 + (i * x_step);
        int y = height - netin_data.usages[i] * height / 200000 / 1024;
        if (y < 50) y = 50; //probably not the best way to do it
        // printf("Current netin y: %d\n", y);
        cairo_line_to(cr, x, y);

    }

    // Draw the line graph
    cairo_stroke(cr);

    // Network usage text
    char netin[50] = "";
    format_bytes(metrics->network_in, netin);
    char netout[50] = "";
    format_bytes(metrics->network_out, netout);
    char totalrec[50] = "";
    format_bytes(metrics->total_received, totalrec);
    char totalsent[50] = "";
    format_bytes(metrics->total_sent, totalsent);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, 50, 20);
    cairo_show_text(cr, "Network Usage:");
    cairo_move_to(cr, 50, 40);
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Receiving: %s/s", netin);
    cairo_show_text(cr, buffer);

    cairo_move_to(cr, 450, 40);
    snprintf(buffer, sizeof(buffer), "Sending: %s/s", netout);
    cairo_show_text(cr, buffer);

    cairo_move_to(cr, 250, 40);
    snprintf(buffer, sizeof(buffer), "Total Received: %s", totalrec);
    cairo_show_text(cr, buffer);

    cairo_move_to(cr, 650, 40);
    snprintf(buffer, sizeof(buffer), "Total Sent: %s", totalsent);
    cairo_show_text(cr, buffer);
}


// void draw_cpu_graph(GtkWidget *widget, cairo_t *cr, gpointer data) {
//     Metrics *metrics = g_object_get_data(G_OBJECT(widget), "metrics");
//     if (!metrics) return;

//     cairo_set_source_rgb(cr, 1, 1, 1);
//     cairo_paint(cr);

//     cairo_set_line_width(cr, 2); // Set border thickness
//     cairo_set_source_rgb(cr, 0, 0, 0); // Border color (black)
//     cairo_rectangle(cr, 50, 50, 50, 180);
//     cairo_stroke(cr); // Apply the border

//     // CPU usage bar graph
//     cairo_set_source_rgb(cr, 0, 1, 0);
//     cairo_rectangle(cr, 50, 230 - metrics->cpu_usage * 1.8, 50, metrics->cpu_usage * 1.8);
//     cairo_fill(cr);

//     // CPU usage text
//     cairo_set_source_rgb(cr, 0, 0, 0);
//     cairo_move_to(cr, 120, 70);
//     cairo_show_text(cr, "CPU Usage:");
//     cairo_move_to(cr, 120, 90);
//     char buffer[64];
//     snprintf(buffer, sizeof(buffer), "%.2f%%", metrics->cpu_usage);
//     cairo_show_text(cr, buffer);
// }


// void draw_graph(GtkWidget *widget, cairo_t *cr, gpointer data) {
//     Metrics *metrics = g_object_get_data(G_OBJECT(widget), "metrics");
//     if (!metrics) return;

//     cairo_set_source_rgb(cr, 1, 1, 1);
//     cairo_paint(cr);   

//     cairo_set_source_rgb(cr, 0, 1, 0);
//     cairo_rectangle(cr, 50, 200 - metrics->cpu_usage * 2, 50, metrics->cpu_usage * 2);
//     cairo_fill(cr);

//     cairo_set_source_rgb(cr, 0, 0, 1);
//     cairo_rectangle(cr, 150, 200 - metrics->memory_usage * 2, 50, metrics->memory_usage * 2);
//     cairo_fill(cr);
// }

double get_cpu_usage() {
    static long prev_idle = 0, prev_total = 0;
    long idle, total, delta_idle, delta_total;
    char buffer[1024];
    FILE *fp = fopen("/proc/stat", "r");
    fgets(buffer, sizeof(buffer), fp);
    fclose(fp);

    long user, nice, system, idle_time, iowait, irq, softirq, steal;
    sscanf(buffer, "cpu %ld %ld %ld %ld %ld %ld %ld %ld",
           &user, &nice, &system, &idle_time, &iowait, &irq, &softirq, &steal);

    idle = idle_time + iowait;
    total = user + nice + system + idle_time + iowait + irq + softirq + steal;

    delta_idle = idle - prev_idle;
    delta_total = total - prev_total;

    prev_idle = idle;
    prev_total = total;

    return 100.0 * (1.0 - (double)delta_idle / (double)delta_total);
}


double get_memory_usage() {
    FILE *fp = fopen("/proc/meminfo", "r");
    char key[64];
    long value, mem_total = 0, mem_free = 0, buffers = 0, cached = 0;

    while (fscanf(fp, "%s %ld", key, &value) != EOF) {
        if (strcmp(key, "MemTotal:") == 0)
            mem_total = value;
        if (strcmp(key, "MemFree:") == 0)
            mem_free = value;
        if (strcmp(key, "Buffers:") == 0)
            buffers = value;
        if (strcmp(key, "Cached:") == 0)
            cached = value;
    }
    fclose(fp);

    return 100.0 * (1.0 - (double)(mem_free + buffers + cached) / mem_total);
}

double get_swap_usage() {
    return 0;
}

void get_network_usage(double *net_in, double *net_out, long *total_received, long *total_sent) {
    static long prev_rx = 0, prev_tx = 0;
    char buffer[1024];
    FILE *fp = fopen("/proc/net/dev", "r");

    // Skip headers
    fgets(buffer, sizeof(buffer), fp);
    fgets(buffer, sizeof(buffer), fp);

    long rx = 0, tx = 0;
    while (fgets(buffer, sizeof(buffer), fp)) {
        char interface[32];
        long r, t;
        sscanf(buffer, "%s %ld %*d %*d %*d %*d %*d %*d %*d %ld", interface, &r, &t);
        rx += r;
        tx += t;
    }
    fclose(fp);

    *net_in = (rx - prev_rx); // 1024.0;  // KiB/s
    *net_out = (tx - prev_tx); // 1024.0; // KiB/s

    *total_received = rx; // 1024.0; // KiB
    *total_sent = tx; // 1024.0; // KiB

    prev_rx = rx;
    prev_tx = tx;
}
