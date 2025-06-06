#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include "gui.h"
#include "scheduler.h"
#include "file_loader.h"

static GtkWidget *entry_quantum;
static GtkWidget *gantt_area;
static GtkWidget *metrics_box;
static GtkWidget *alg_check[5];
static Process procesos[MAX_PROCESOS];
static int num_procesos = 0;

void mostrar_ventana_inicio()
{
    gtk_init(NULL, NULL);

    GtkWidget *ventana = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(ventana), "Simulador Principal");
    gtk_window_set_default_size(GTK_WINDOW(ventana), 400, 200);
    gtk_container_set_border_width(GTK_CONTAINER(ventana), 20);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);

    GtkWidget *label = gtk_label_new("Seleccione qué simulador desea usar:");
    GtkWidget *boton_a = gtk_button_new_with_label("🧠 A: Algoritmos de Calendarización");
    GtkWidget *boton_b = gtk_button_new_with_label("🔐 B: Mecanismos de Sincronización");

    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), boton_a, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), boton_b, FALSE, FALSE, 0);

    g_signal_connect(boton_a, "clicked", G_CALLBACK(mostrar_ventana_algoritmos), NULL);
    g_signal_connect(boton_b, "clicked", G_CALLBACK(mostrar_ventana_sincronizacion), NULL);
    g_signal_connect(ventana, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_container_add(GTK_CONTAINER(ventana), vbox);
    gtk_widget_show_all(ventana);
    gtk_main();
}

// Función auxiliar para dibujar una línea de bloques Gantt
static GtkWidget *crear_linea_gantt(const char *label, const Process *procesos, int ciclos[], int tam)
{
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);

    GtkWidget *titulo = gtk_label_new(label);
    gtk_widget_set_size_request(titulo, 40, -1);
    gtk_box_pack_start(GTK_BOX(hbox), titulo, FALSE, FALSE, 5);

    for (int i = 0; i < tam; i++)
    {
        GtkWidget *frame = gtk_frame_new(NULL);
        char texto[8];
        snprintf(texto, sizeof(texto), "%s", procesos[ciclos[i]].pid);
        GtkWidget *lbl = gtk_label_new(texto);

        GdkRGBA color;
        gdk_rgba_parse(&color, ciclos[i] % 2 == 0 ? "lightblue" : "lightgreen");
        GtkWidget *event = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(event), lbl);
        gtk_widget_override_background_color(event, GTK_STATE_FLAG_NORMAL, &color);

        gtk_container_add(GTK_CONTAINER(frame), event);
        gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 1);
    }

    return hbox;
}

// Simular todo y mostrar resultados
static void simular_algoritmos(GtkWidget *widget, gpointer data)
{
    gtk_container_foreach(GTK_CONTAINER(gantt_area), (GtkCallback)gtk_widget_destroy, NULL);
    gtk_container_foreach(GTK_CONTAINER(metrics_box), (GtkCallback)gtk_widget_destroy, NULL);

    char quantum_text[10];
    strcpy(quantum_text, gtk_entry_get_text(GTK_ENTRY(entry_quantum)));
    int quantum = atoi(quantum_text);

    TimelineEntry timeline[MAX_CICLOS];
    int ciclos = 0;

    double avg_wt, avg_tt, avg_ct;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(alg_check[0])))
    {
        Process copia[MAX_PROCESOS];
        memcpy(copia, procesos, sizeof(Process) * num_procesos);

        simular_fifo(copia, num_procesos, timeline, &ciclos);
        GtkWidget *linea = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
        gtk_box_pack_start(GTK_BOX(linea), gtk_label_new("FIFO:"), FALSE, FALSE, 2);
        for (int i = 0; i < ciclos; i++)
        {
            GtkWidget *lbl = gtk_label_new(timeline[i].pid);
            GtkWidget *frame = gtk_frame_new(NULL);
            gtk_container_add(GTK_CONTAINER(frame), lbl);
            gtk_box_pack_start(GTK_BOX(linea), frame, FALSE, FALSE, 1);
        }
        gtk_box_pack_start(GTK_BOX(gantt_area), linea, FALSE, FALSE, 2);

        calcular_metricas(copia, num_procesos, &avg_wt, &avg_tt, &avg_ct);
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "FIFO Metrics: WT=%.2f | TT=%.2f | CT=%.2f", avg_wt, avg_tt, avg_ct);
        gtk_box_pack_start(GTK_BOX(metrics_box), gtk_label_new(buffer), FALSE, FALSE, 2);
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(alg_check[1])))
    {
        Process copia[MAX_PROCESOS];
        memcpy(copia, procesos, sizeof(Process) * num_procesos);

        simular_sjf(copia, num_procesos, timeline, &ciclos);
        GtkWidget *linea = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
        gtk_box_pack_start(GTK_BOX(linea), gtk_label_new("SJF:"), FALSE, FALSE, 2);
        for (int i = 0; i < ciclos; i++)
        {
            GtkWidget *lbl = gtk_label_new(timeline[i].pid);
            GtkWidget *frame = gtk_frame_new(NULL);
            gtk_container_add(GTK_CONTAINER(frame), lbl);
            gtk_box_pack_start(GTK_BOX(linea), frame, FALSE, FALSE, 1);
        }
        gtk_box_pack_start(GTK_BOX(gantt_area), linea, FALSE, FALSE, 2);

        calcular_metricas(copia, num_procesos, &avg_wt, &avg_tt, &avg_ct);
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "SJF Metrics: WT=%.2f | TT=%.2f | CT=%.2f", avg_wt, avg_tt, avg_ct);
        gtk_box_pack_start(GTK_BOX(metrics_box), gtk_label_new(buffer), FALSE, FALSE, 2);
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(alg_check[2])))
    {
        Process copia[MAX_PROCESOS];
        memcpy(copia, procesos, sizeof(Process) * num_procesos);

        simular_srt(copia, num_procesos, timeline, &ciclos);
        GtkWidget *linea = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
        gtk_box_pack_start(GTK_BOX(linea), gtk_label_new("SRT:"), FALSE, FALSE, 2);
        for (int i = 0; i < ciclos; i++)
        {
            GtkWidget *lbl = gtk_label_new(timeline[i].pid);
            GtkWidget *frame = gtk_frame_new(NULL);
            gtk_container_add(GTK_CONTAINER(frame), lbl);
            gtk_box_pack_start(GTK_BOX(linea), frame, FALSE, FALSE, 1);
        }
        gtk_box_pack_start(GTK_BOX(gantt_area), linea, FALSE, FALSE, 2);

        calcular_metricas(copia, num_procesos, &avg_wt, &avg_tt, &avg_ct);
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "SRT Metrics: WT=%.2f | TT=%.2f | CT=%.2f", avg_wt, avg_tt, avg_ct);
        gtk_box_pack_start(GTK_BOX(metrics_box), gtk_label_new(buffer), FALSE, FALSE, 2);
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(alg_check[3])))
    {
        Process copia[MAX_PROCESOS];
        memcpy(copia, procesos, sizeof(Process) * num_procesos);

        simular_rr(copia, num_procesos, quantum, timeline, &ciclos);
        GtkWidget *linea = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
        gtk_box_pack_start(GTK_BOX(linea), gtk_label_new("RR:"), FALSE, FALSE, 2);
        for (int i = 0; i < ciclos; i++)
        {
            GtkWidget *lbl = gtk_label_new(timeline[i].pid);
            GtkWidget *frame = gtk_frame_new(NULL);
            gtk_container_add(GTK_CONTAINER(frame), lbl);
            gtk_box_pack_start(GTK_BOX(linea), frame, FALSE, FALSE, 1);
        }
        gtk_box_pack_start(GTK_BOX(gantt_area), linea, FALSE, FALSE, 2);

        calcular_metricas(copia, num_procesos, &avg_wt, &avg_tt, &avg_ct);
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "RR Metrics: WT=%.2f | TT=%.2f | CT=%.2f", avg_wt, avg_tt, avg_ct);
        gtk_box_pack_start(GTK_BOX(metrics_box), gtk_label_new(buffer), FALSE, FALSE, 2);
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(alg_check[4])))
    {
        Process copia[MAX_PROCESOS];
        memcpy(copia, procesos, sizeof(Process) * num_procesos);

        simular_priority(copia, num_procesos, timeline, &ciclos);
        GtkWidget *linea = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
        gtk_box_pack_start(GTK_BOX(linea), gtk_label_new("Priority:"), FALSE, FALSE, 2);
        for (int i = 0; i < ciclos; i++)
        {
            GtkWidget *lbl = gtk_label_new(timeline[i].pid);
            GtkWidget *frame = gtk_frame_new(NULL);
            gtk_container_add(GTK_CONTAINER(frame), lbl);
            gtk_box_pack_start(GTK_BOX(linea), frame, FALSE, FALSE, 1);
        }
        gtk_box_pack_start(GTK_BOX(gantt_area), linea, FALSE, FALSE, 2);

        calcular_metricas(copia, num_procesos, &avg_wt, &avg_tt, &avg_ct);
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "Priority Metrics: WT=%.2f | TT=%.2f | CT=%.2f", avg_wt, avg_tt, avg_ct);
        gtk_box_pack_start(GTK_BOX(metrics_box), gtk_label_new(buffer), FALSE, FALSE, 2);
    }

    gtk_widget_show_all(gantt_area);
    gtk_widget_show_all(metrics_box);
}

// Cargar archivo procesos.txt
static void cargar_archivo(GtkWidget *widget, gpointer data)
{
    num_procesos = cargar_procesos("data/procesos.txt", procesos, MAX_PROCESOS);
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK, "Procesos cargados: %d", num_procesos);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// GUI principal para simulador A
void mostrar_ventana_algoritmos()
{
    GtkWidget *ventana = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(ventana), "Simulador A: Algoritmos");
    gtk_window_set_default_size(GTK_WINDOW(ventana), 1000, 600);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    // Barra superior
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    const char *nombres[] = {"FIFO", "SJF", "SRT", "Round Robin", "Priority"};
    for (int i = 0; i < 5; i++)
    {
        alg_check[i] = gtk_check_button_new_with_label(nombres[i]);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(alg_check[i]), TRUE);
        gtk_box_pack_start(GTK_BOX(hbox), alg_check[i], FALSE, FALSE, 2);
    }

    GtkWidget *btn_cargar = gtk_button_new_with_label("Upload Processes");
    GtkWidget *btn_run = gtk_button_new_with_label("Run Simulation");
    GtkWidget *lbl_q = gtk_label_new("Quantum:");
    entry_quantum = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry_quantum), "2");
    gtk_widget_set_size_request(entry_quantum, 40, -1);

    gtk_box_pack_start(GTK_BOX(hbox), btn_cargar, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), btn_run, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), lbl_q, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(hbox), entry_quantum, FALSE, FALSE, 2);

    gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, FALSE, 2);

    // Área con scroll
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gantt_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(scroll), gantt_area);
    gtk_box_pack_start(GTK_BOX(main_vbox), scroll, TRUE, TRUE, 2);

    // Métricas
    metrics_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_box_pack_start(GTK_BOX(main_vbox), metrics_box, FALSE, FALSE, 5);

    gtk_container_add(GTK_CONTAINER(ventana), main_vbox);

    // Señales
    g_signal_connect(ventana, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(btn_cargar, "clicked", G_CALLBACK(cargar_archivo), NULL);
    g_signal_connect(btn_run, "clicked", G_CALLBACK(simular_algoritmos), NULL);

    gtk_widget_show_all(ventana);
    gtk_main();
}

void mostrar_ventana_sincronizacion()
{
    GtkWidget *ventana = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(ventana), "Simulador B: Sincronización");
    gtk_window_set_default_size(GTK_WINDOW(ventana), 1000, 600);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    GtkWidget *controles = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    GtkWidget *modo_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(modo_combo), "Mutex");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(modo_combo), "Semáforo");
    gtk_combo_box_set_active(GTK_COMBO_BOX(modo_combo), 0);

    GtkWidget *btn_proc = gtk_button_new_with_label("Cargar Procesos");
    GtkWidget *btn_rec = gtk_button_new_with_label("Cargar Recursos");
    GtkWidget *btn_acc = gtk_button_new_with_label("Cargar Acciones");
    GtkWidget *btn_run = gtk_button_new_with_label("Run Simulation");

    gtk_box_pack_start(GTK_BOX(controles), gtk_label_new("Modo:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controles), modo_combo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controles), btn_proc, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controles), btn_rec, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controles), btn_acc, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controles), btn_run, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), controles, FALSE, FALSE, 0);

    // Gantt + Scroll
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gantt_area_b = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_add(GTK_CONTAINER(scroll), gantt_area_b);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 2);

    panel_metrics_b = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX(vbox), panel_metrics_b, FALSE, FALSE, 4);

    gtk_container_add(GTK_CONTAINER(ventana), vbox);

    // Conectar eventos
    // g_signal_connect(btn_proc, "clicked", G_CALLBACK(on_cargar_procesos), NULL);
    // g_signal_connect(btn_rec, "clicked", G_CALLBACK(on_cargar_recursos), NULL);
    // g_signal_connect(btn_acc, "clicked", G_CALLBACK(on_cargar_acciones), NULL);
    // g_signal_connect(btn_run, "clicked", G_CALLBACK(on_run_simulacion_b), NULL);

    g_signal_connect(ventana, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(ventana);
}