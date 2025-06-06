#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include "gui.h"

void iniciar_gui_fifo(Process *procesos, int n)
{
    gtk_init(NULL, NULL);

    GtkWidget *ventana = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(ventana), "Simulador FIFO");
    gtk_window_set_default_size(GTK_WINDOW(ventana), 800, 200);
    gtk_container_set_border_width(GTK_CONTAINER(ventana), 10);

    GtkWidget *caja = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    for (int i = 0; i < n; i++)
    {
        GtkWidget *frame = gtk_frame_new(procesos[i].pid);

        char texto[64];
        snprintf(texto, sizeof(texto), "BT:%d\nWT:%d", procesos[i].burst_time, procesos[i].waiting_time);

        GtkWidget *label = gtk_label_new(texto);
        gtk_container_add(GTK_CONTAINER(frame), label);

        gtk_box_pack_start(GTK_BOX(caja), frame, TRUE, TRUE, 5);
    }

    gtk_container_add(GTK_CONTAINER(ventana), caja);

    g_signal_connect(ventana, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(ventana);
    gtk_main();
}
