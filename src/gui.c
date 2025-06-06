#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include "gui.h"
#include "scheduler.h"
#include "file_loader.h"

static Process procesos[MAX_PROCESOS];
static int cantidad = 0;

// Cargar procesos
static gboolean cargar_procesos_autom()
{
    cantidad = cargar_procesos("data/procesos.txt", procesos, MAX_PROCESOS);
    if (cantidad <= 0)
    {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Error al cargar procesos.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return FALSE;
    }
    return TRUE;
}

// Mostrar ventana de resultados
static void mostrar_resultados(Process *proc, int n, const char *titulo)
{
    GtkWidget *ventana = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(ventana), titulo);
    gtk_window_set_default_size(GTK_WINDOW(ventana), 800, 200);
    gtk_container_set_border_width(GTK_CONTAINER(ventana), 10);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    GtkWidget *caja = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    for (int i = 0; i < n; i++)
    {
        GtkWidget *frame = gtk_frame_new(proc[i].pid);
        char texto[64];
        snprintf(texto, sizeof(texto), "BT:%d\nWT:%d", proc[i].burst_time, proc[i].waiting_time);
        GtkWidget *label = gtk_label_new(texto);
        gtk_container_add(GTK_CONTAINER(frame), label);
        gtk_box_pack_start(GTK_BOX(caja), frame, FALSE, FALSE, 5);
    }

    gtk_container_add(GTK_CONTAINER(scroll), caja);
    gtk_container_add(GTK_CONTAINER(ventana), scroll);
    gtk_widget_show_all(ventana);
}

// Handler para opciones de algoritmo
static void on_seleccion_algoritmo(GtkComboBoxText *combo, gpointer user_data)
{
    const gchar *algoritmo = gtk_combo_box_text_get_active_text(combo);
    if (!cargar_procesos_autom())
        return;

    if (strcmp(algoritmo, "FIFO") == 0)
    {
        fifo(procesos, cantidad);
        mostrar_resultados(procesos, cantidad, "FIFO");
    }
    else if (strcmp(algoritmo, "SJF") == 0)
    {
        sjf(procesos, cantidad);
        mostrar_resultados(procesos, cantidad, "SJF");
    }
    else if (strcmp(algoritmo, "SRT") == 0)
    {
        srt(procesos, cantidad);
        mostrar_resultados(procesos, cantidad, "SRT");
    }
    else if (strcmp(algoritmo, "Round Robin") == 0)
    {
        GtkWidget *dialog = gtk_dialog_new_with_buttons("Quantum",
                                                        NULL, GTK_DIALOG_MODAL,
                                                        "_OK", GTK_RESPONSE_OK, "_Cancel", GTK_RESPONSE_CANCEL, NULL);

        GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        GtkWidget *entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Ingrese quantum");
        gtk_container_add(GTK_CONTAINER(content), entry);
        gtk_widget_show_all(dialog);

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
        {
            const gchar *texto = gtk_entry_get_text(GTK_ENTRY(entry));
            int quantum = atoi(texto);
            if (quantum > 0)
            {
                round_robin(procesos, cantidad, quantum);
                mostrar_resultados(procesos, cantidad, "Round Robin");
            }
        }
        gtk_widget_destroy(dialog);
    }
    else if (strcmp(algoritmo, "Prioridad") == 0)
    {
        priority(procesos, cantidad);
        mostrar_resultados(procesos, cantidad, "Prioridad con envejecimiento");
    }
}

// Ventana principal
void mostrar_ventana_principal()
{
    gtk_init(NULL, NULL);

    GtkWidget *ventana = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(ventana), "Simulador de Planificación");
    gtk_window_set_default_size(GTK_WINDOW(ventana), 400, 150);
    gtk_container_set_border_width(GTK_CONTAINER(ventana), 20);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget *label = gtk_label_new("Seleccione algoritmo de planificación:");
    GtkWidget *combo = gtk_combo_box_text_new();

    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "FIFO");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "SJF");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "SRT");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Round Robin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Prioridad");

    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 0);

    g_signal_connect(combo, "changed", G_CALLBACK(on_seleccion_algoritmo), NULL);
    g_signal_connect(ventana, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_container_add(GTK_CONTAINER(ventana), vbox);
    gtk_widget_show_all(ventana);
    gtk_main();
}
