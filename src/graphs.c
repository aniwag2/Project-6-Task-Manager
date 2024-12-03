#include <gtk/gtk.h>
#include <cairo.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "graphs.h"

GraphData cpu_data = {{0}};
GraphData memo_data = {{0}};
GraphData swap_data = {{0}};
GraphData netin_data = {{0}};
GraphData netout_data = {{0}};

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
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *graph_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(scrolled_window), graph_tab);

    GtkWidget *cpu_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(cpu_area, 800, 250);
    gtk_box_pack_start(GTK_BOX(graph_tab), cpu_area, FALSE, TRUE, 0);
    g_object_set_data(G_OBJECT(cpu_area), "metrics", metrics);
    g_signal_connect(cpu_area, "draw", G_CALLBACK(draw_cpu_graph), metrics);

    metrics->cpu_drawing_area = cpu_area;

    GtkWidget *mem_swap_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(mem_swap_area, 800, 250);
    gtk_box_pack_start(GTK_BOX(graph_tab), mem_swap_area, FALSE, TRUE, 0);
    g_object_set_data(G_OBJECT(mem_swap_area), "metrics", metrics);
    g_signal_connect(mem_swap_area, "draw", G_CALLBACK(draw_mem_swap_graph), metrics);

    metrics->mem_swap_drawing_area = mem_swap_area;

    GtkWidget *network_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(network_area, 800, 250);
    gtk_box_pack_start(GTK_BOX(graph_tab), network_area, FALSE, TRUE, 0);
    g_object_set_data(G_OBJECT(network_area), "metrics", metrics);
    g_signal_connect(network_area, "draw", G_CALLBACK(draw_network_graph), metrics);

    metrics->network_drawing_area = network_area;

    g_timeout_add(1000, update_metrics, metrics);

    return scrolled_window;
}

gboolean update_metrics(gpointer data) {
    Metrics *metrics = (Metrics *)data;

    if (!metrics) return FALSE;

    // Update the metrics
    metrics->cpu_usage = get_cpu_usage();
    // metrics->memory_usage = get_memory_usage();
    get_memory_usage(&metrics->memory_usage, &metrics->memory_used, &metrics->memory_total);
    // metrics->swap_usage = get_swap_usage();
    get_swap_usage(&metrics->swap_usage, &metrics->swap_used, &metrics->swap_total);
    get_network_usage(&metrics->network_in, &metrics->network_out, &metrics->total_received, &metrics->total_sent);

    // Update the historical data
    memmove(cpu_data.usages, cpu_data.usages + 1, (NUM_POINTS - 1) * sizeof(double));
    cpu_data.usages[NUM_POINTS - 1] = metrics->cpu_usage;

    memmove(memo_data.usages, memo_data.usages + 1, (NUM_POINTS - 1) * sizeof(double));
    memo_data.usages[NUM_POINTS - 1] = metrics->memory_usage;

    memmove(swap_data.usages, swap_data.usages + 1, (NUM_POINTS - 1) * sizeof(double));
    swap_data.usages[NUM_POINTS - 1] = metrics->swap_usage;

    memmove(netin_data.usages, netin_data.usages + 1, (NUM_POINTS - 1) * sizeof(double));
    netin_data.usages[NUM_POINTS - 1] = metrics->network_in;

    memmove(netout_data.usages, netout_data.usages + 1, (NUM_POINTS - 1) * sizeof(double));
    netout_data.usages[NUM_POINTS - 1] = metrics->network_out;

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

    // Draw the graph rectangle
    cairo_set_line_width(cr, 2); 
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_rectangle(cr, 50, 50, 500, 150);
    cairo_stroke(cr);

    // Set line width for the graph line
    cairo_set_line_width(cr, 2); 

    // Set the color for the CPU usage line (orange)
    cairo_set_source_rgb(cr, 0.949, 0.522, 0);

    // Plot the line graph based on historical data
    int width = 500;
    int height = 150;
    double x_step = (double)width / NUM_POINTS;

    // Start the path at the first point in the history 
    cairo_move_to(cr, 50, 50 + (height - cpu_data.usages[0] * height / 100));

    // Loop through the data points and create the line graph
    for (int i = 1; i < NUM_POINTS; i++) {
        int x = 50 + (i * x_step);
        int y = 50 + (height - cpu_data.usages[i] * height / 100);
        cairo_line_to(cr, x, y);
    }

    // Draw the line graph
    cairo_stroke(cr);

    // Display CPU usage text with color-coded dot
    char buffer[64];

    // Draw the colored dot
    cairo_set_source_rgb(cr, 0.949, 0.522, 0); // Same color as the line
    cairo_arc(cr, 50, 40, 5, 0, 2 * M_PI); // Position (50, 40), radius 5
    cairo_fill(cr);

    // Set text color to black
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, 60, 45); // Adjust position to be next to the dot
    snprintf(buffer, sizeof(buffer), "CPU1 %.2f%%", metrics->cpu_usage);
    cairo_show_text(cr, buffer);

    // Draw the graph title
    cairo_move_to(cr, 50, 20);
    cairo_show_text(cr, "CPU History");
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

    cairo_set_source_rgb(cr, .435, .118, .659);

    // Plot the line graph based on historical data
    int width = 500;
    int height = 150;
    double x_step = (double)width / NUM_POINTS;

    // Start the path at the first point in the history 
    cairo_move_to(cr, 50, 50 + (height - memo_data.usages[0] * height / 100));

    // Loop through the data points and create the line graph
    for (int i = 1; i < NUM_POINTS; i++) {
        int x = 50 + (i * x_step);
        int y = 50 + (height - memo_data.usages[i] * height / 100);
        cairo_line_to(cr, x, y);
    }

    cairo_stroke(cr);

    cairo_set_source_rgb(cr, .113, .545, .113);

    // Start the path at the first point in the history 
    cairo_move_to(cr, 50, 50 + (height - swap_data.usages[0] * height / 100));

    // Loop through the data points and create the line graph
    for (int i = 1; i < NUM_POINTS; i++) {
        int x = 50 + (i * x_step);
        int y = 50 + (height - swap_data.usages[i] * height / 100);
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
    // Memory and Swap usage text with color-coded dots
    char buffer[128];
    char mem_space_used[50], mem_space_total[50];
    char swap_space_used[50], swap_space_total[50];

    format_bytes(metrics->memory_used, mem_space_used);
    format_bytes(metrics->memory_total, mem_space_total);
    format_bytes(metrics->swap_used, swap_space_used);
    format_bytes(metrics->swap_total, swap_space_total);

    // Draw graph title
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, 50, 20);
    cairo_show_text(cr, "Memory and Swap History");

    // Draw Memory label with colored dot
    cairo_set_source_rgb(cr, 0.435, 0.118, 0.659);
    cairo_arc(cr, 50, 40, 5, 0, 2 * M_PI);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, 60, 45);
    snprintf(buffer, sizeof(buffer), "Memory: %s (%.2f%%) of %s", mem_space_used, metrics->memory_usage, mem_space_total);
    cairo_show_text(cr, buffer);

    // Draw Swap label with colored dot
    cairo_set_source_rgb(cr, 0.113, 0.545, 0.113);
    cairo_arc(cr, 300, 40, 5, 0, 2 * M_PI);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, 310, 45);
    snprintf(buffer, sizeof(buffer), "Swap: %s (%.2f%%) of %s", swap_space_used, metrics->swap_usage, swap_space_total);
    cairo_show_text(cr, buffer);

}

    // // Memory usage bar graph
    // cairo_set_source_rgb(cr, 0, 0, 1);
    // cairo_rectangle(cr, 50, 180 - metrics->memory_usage * 1.8, 50, metrics->memory_usage * 1.8);
    // cairo_fill(cr);

    // // Swap usage bar graph
    // cairo_set_source_rgb(cr, 1, 0, 0);
    // cairo_rectangle(cr, 150, 180 - metrics->swap_usage * 1.8, 50, metrics->swap_usage * 1.8);
    // cairo_fill(cr);

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

    cairo_set_source_rgb(cr, .529, .808, 1);

    // Plot the line graph based on historical data
    int width = 500;
    int height = 150;
    double x_step = (double)width / NUM_POINTS;

    // Start the path at the first point in the history 
    cairo_move_to(cr, 50, 50 + (height - netin_data.usages[0] * height / 200000 / 1024));

    // Loop through the data points and create the line graph
    for (int i = 1; i < NUM_POINTS; i++) {
        int x = 50 + (i * x_step);
        int y = 50 + (height - netin_data.usages[i] * height / 200000 / 1024);
        if (y < 50) y = 50; //probably not the best way to do it
        // printf("Current netin y: %d\n", y);
        cairo_line_to(cr, x, y);
    }

    // Draw the line graph
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, .863, .078, .235);

    cairo_move_to(cr, 50, 50 + (height - netout_data.usages[0] * height / 200000 / 1024));

    // Loop through the data points and create the line graph
    for (int i = 1; i < NUM_POINTS; i++) {
        int x = 50 + (i * x_step);
        int y = 50 + (height - netout_data.usages[i] * height / 200000 / 1024);
        if (y < 50) y = 50; //probably not the best way to do it
        // printf("Current netin y: %d\n", y);
        cairo_line_to(cr, x, y);
    }

    // Draw the line graph
    cairo_stroke(cr);

    // Network usage text with color-coded dots
    char netin[50], netout[50], totalrec[50], totalsent[50];
    char buffer[128];

    format_bytes(metrics->network_in, netin);
    format_bytes(metrics->network_out, netout);
    format_bytes(metrics->total_received, totalrec);
    format_bytes(metrics->total_sent, totalsent);

    // Draw graph title
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, 50, 20);
    cairo_show_text(cr, "Network Usage");

    // Draw Receiving label with colored dot
    cairo_set_source_rgb(cr, 0.529, 0.808, 1);
    cairo_arc(cr, 50, 40, 5, 0, 2 * M_PI);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, 60, 45);
    snprintf(buffer, sizeof(buffer), "Receiving: %s/s (Total: %s)", netin, totalrec);
    cairo_show_text(cr, buffer);

    // Draw Sending label with colored dot
    cairo_set_source_rgb(cr, 0.863, 0.078, 0.235);
    cairo_arc(cr, 300, 40, 5, 0, 2 * M_PI);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, 310, 45);
    snprintf(buffer, sizeof(buffer), "Sending: %s/s (Total: %s)", netout, totalsent);
    cairo_show_text(cr, buffer);
}

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

void get_memory_usage(double *memory_usage, long *mem_used, long *mem_total) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        perror("Failed to open /proc/meminfo");
        *memory_usage = -1;
        *mem_used = 0;
        *mem_total = 0;
        return;
    }

    char key[64];
    long value, mem_free = 0, buffers = 0, cached = 0;
    *mem_used = 0;
    *mem_total = 0;

    while (fscanf(fp, "%s %ld", key, &value) != EOF) {
        if (strcmp(key, "MemTotal:") == 0)
            *mem_total = value;
        if (strcmp(key, "MemFree:") == 0)
            mem_free = value;
        if (strcmp(key, "Buffers:") == 0)
            buffers = value;
        if (strcmp(key, "Cached:") == 0)
            cached = value;
    }
    fclose(fp);

    if (*mem_total > 0) {
        *mem_used = *mem_total - (mem_free + buffers + cached);
        *memory_usage = 100.0 * (double)*mem_used / *mem_total;
    } else {
        *memory_usage = -1;
    }
}

void get_swap_usage(double *swap_usage, long *swap_used, long *swap_total) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        perror("Failed to open /proc/meminfo");
        *swap_usage = -1;
        *swap_used = 0;
        *swap_total = 0;
        return;
    }

    char key[64];
    long value, swap_free = 0;
    *swap_used = 0;
    *swap_total = 0;

    while (fscanf(fp, "%s %ld", key, &value) != EOF) {
        if (strcmp(key, "SwapTotal:") == 0)
            *swap_total = value;
        if (strcmp(key, "SwapFree:") == 0)
            swap_free = value;
    }
    fclose(fp);

    if (*swap_total > 0) {
        *swap_used = *swap_total - swap_free;
        *swap_usage = 100.0 * (double)*swap_used / *swap_total;
    } else {
        *swap_usage = 0; // No swap available
    }
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
