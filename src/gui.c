#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include "gui.h"
#include "scheduler.h"
#include "file_loader.h"

#define MAX_LINEAS 5 // Máximo de algoritmos activos

static GtkWidget *lineas_gantt[MAX_LINEAS]; // 1 por algoritmo
static TimelineEntry timelines[MAX_LINEAS][MAX_CICLOS];
static int total_ciclos[MAX_LINEAS];
static int current_ciclo = 0;
static int num_lineas = 0;

static GtkWidget *entry_quantum;
static GtkWidget *gantt_area;
static GtkWidget *metrics_box;
static GtkWidget *alg_check[5];
static Process procesos[MAX_PROCESOS];
static int num_procesos = 0;

static int ciclo_actual_por_linea[MAX_LINEAS] = {0};
static GtkWidget *leyenda_box;

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

static GdkRGBA color_para_pid(const char *pid)
{
    GdkRGBA color;
    unsigned hash = 0;
    for (int i = 0; pid[i]; i++)
        hash = pid[i] + (hash << 6) + (hash << 16) - hash;

    int r = (hash & 0xFF0000) >> 16;
    int g = (hash & 0x00FF00) >> 8;
    int b = (hash & 0x0000FF);

    color.red = (double)(r % 200 + 55) / 255.0;
    color.green = (double)(g % 200 + 55) / 255.0;
    color.blue = (double)(b % 200 + 55) / 255.0;
    color.alpha = 1.0;
    return color;
}

static void construir_leyenda()
{
    gtk_container_foreach(GTK_CONTAINER(leyenda_box), (GtkCallback)gtk_widget_destroy, NULL);

    // Obtener lista única de PIDs usados
    char vistos[MAX_PROCESOS][MAX_PID_LEN];
    int n_vistos = 0;

    for (int l = 0; l < num_lineas; l++)
    {
        for (int c = 0; c < total_ciclos[l]; c++)
        {
            const char *pid = timelines[l][c].pid;

            // Ver si ya está en la lista
            gboolean ya_agregado = FALSE;
            for (int k = 0; k < n_vistos; k++)
            {
                if (strcmp(pid, vistos[k]) == 0)
                {
                    ya_agregado = TRUE;
                    break;
                }
            }

            if (!ya_agregado && strlen(pid) > 0)
            {
                strcpy(vistos[n_vistos++], pid);
            }
        }
    }

    for (int i = 0; i < n_vistos; i++)
    {
        GtkWidget *cuadro = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);

        GtkWidget *color_cuadro = gtk_event_box_new();
        gtk_widget_set_size_request(color_cuadro, 20, 20);

        GdkRGBA color = color_para_pid(vistos[i]);
        gtk_widget_override_background_color(color_cuadro, GTK_STATE_FLAG_NORMAL, &color);

        GtkWidget *label = gtk_label_new(vistos[i]);
        gtk_box_pack_start(GTK_BOX(cuadro), color_cuadro, FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(cuadro), label, FALSE, FALSE, 2);

        gtk_box_pack_start(GTK_BOX(leyenda_box), cuadro, FALSE, FALSE, 5);
    }

    gtk_widget_show_all(leyenda_box);
}

gboolean animar_gantt(gpointer data)
{
    gboolean continuar = FALSE;

    for (int l = 0; l < num_lineas; l++)
    {
        int c = ciclo_actual_por_linea[l];
        if (c < total_ciclos[l])
        {
            GtkWidget *frame = gtk_frame_new(NULL);
            gtk_widget_set_size_request(frame, 25, 25);

            GtkWidget *event = gtk_event_box_new();
            GtkWidget *label = gtk_label_new(timelines[l][c].pid);
            gtk_container_add(GTK_CONTAINER(event), label);

            GdkRGBA color = color_para_pid(timelines[l][c].pid);
            gtk_widget_override_background_color(event, GTK_STATE_FLAG_NORMAL, &color);

            gtk_container_add(GTK_CONTAINER(frame), event);
            gtk_box_pack_start(GTK_BOX(lineas_gantt[l]), frame, FALSE, FALSE, 0);

            ciclo_actual_por_linea[l]++;
            continuar = TRUE; // al menos una línea sigue activa
        }
    }

    gtk_widget_show_all(gantt_area);
    return continuar;
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

    num_lineas = 0;
    current_ciclo = 0;

    // Reiniciar contadores por línea para animación
    for (int i = 0; i < MAX_LINEAS; i++)
        ciclo_actual_por_linea[i] = 0;

    // FIFO
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(alg_check[0])))
    {
        Process copia[MAX_PROCESOS];
        memcpy(copia, procesos, sizeof(Process) * num_procesos);
        simular_fifo(copia, num_procesos, timelines[num_lineas], &total_ciclos[num_lineas]);

        GtkWidget *linea = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
        gtk_box_pack_start(GTK_BOX(linea), gtk_label_new("FIFO:"), FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(gantt_area), linea, FALSE, FALSE, 2);

        lineas_gantt[num_lineas] = linea;
        num_lineas++;
    }

    // SJF
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(alg_check[1])))
    {
        Process copia[MAX_PROCESOS];
        memcpy(copia, procesos, sizeof(Process) * num_procesos);
        simular_sjf(copia, num_procesos, timelines[num_lineas], &total_ciclos[num_lineas]);

        GtkWidget *linea = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
        gtk_box_pack_start(GTK_BOX(linea), gtk_label_new("SJF:"), FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(gantt_area), linea, FALSE, FALSE, 2);

        lineas_gantt[num_lineas] = linea;
        num_lineas++;
    }

    // SRT
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(alg_check[2])))
    {
        Process copia[MAX_PROCESOS];
        memcpy(copia, procesos, sizeof(Process) * num_procesos);
        simular_srt(copia, num_procesos, timelines[num_lineas], &total_ciclos[num_lineas]);

        GtkWidget *linea = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
        gtk_box_pack_start(GTK_BOX(linea), gtk_label_new("SRT:"), FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(gantt_area), linea, FALSE, FALSE, 2);

        lineas_gantt[num_lineas] = linea;
        num_lineas++;
    }

    // Round Robin
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(alg_check[3])))
    {
        Process copia[MAX_PROCESOS];
        memcpy(copia, procesos, sizeof(Process) * num_procesos);
        simular_rr(copia, num_procesos, quantum, timelines[num_lineas], &total_ciclos[num_lineas]);

        GtkWidget *linea = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
        gtk_box_pack_start(GTK_BOX(linea), gtk_label_new("RR:"), FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(gantt_area), linea, FALSE, FALSE, 2);

        lineas_gantt[num_lineas] = linea;
        num_lineas++;
    }

    // Prioridad
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(alg_check[4])))
    {
        Process copia[MAX_PROCESOS];
        memcpy(copia, procesos, sizeof(Process) * num_procesos);
        simular_priority(copia, num_procesos, timelines[num_lineas], &total_ciclos[num_lineas]);

        GtkWidget *linea = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
        gtk_box_pack_start(GTK_BOX(linea), gtk_label_new("Priority:"), FALSE, FALSE, 2);
        gtk_box_pack_start(GTK_BOX(gantt_area), linea, FALSE, FALSE, 2);

        lineas_gantt[num_lineas] = linea;
        num_lineas++;
    }

    // Lanzar animación
    g_timeout_add(1000, animar_gantt, NULL);
    construir_leyenda();
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
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

    gantt_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(scroll), gantt_area);
    gtk_box_pack_start(GTK_BOX(main_vbox), scroll, TRUE, TRUE, 2);

    // Métricas
    metrics_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_box_pack_start(GTK_BOX(main_vbox), metrics_box, FALSE, FALSE, 5);

    // Leyenda
    leyenda_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(main_vbox), leyenda_box, FALSE, FALSE, 5);

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
    gtk_window_set_title(GTK_WINDOW(ventana), "Simulador B: Mecanismos de Sincronización");
    gtk_window_set_default_size(GTK_WINDOW(ventana), 1000, 600);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    // Controles carga y modo
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

    // Scroll horizontal para timeline
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

    GtkWidget *timeline_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(scroll), timeline_area);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 2);

    // Métricas o estados
    GtkWidget *metrics_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), metrics_box, FALSE, FALSE, 2);

    gtk_container_add(GTK_CONTAINER(ventana), vbox);

    // Conectar señales (funciones a implementar)
    g_signal_connect(btn_proc, "clicked", G_CALLBACK(cargar_procesos_b), NULL);
    g_signal_connect(btn_rec, "clicked", G_CALLBACK(cargar_recursos_b), NULL);
    g_signal_connect(btn_acc, "clicked", G_CALLBACK(cargar_acciones_b), NULL);
    g_signal_connect(btn_run, "clicked", G_CALLBACK(run_simulacion_b), NULL);
    g_signal_connect(ventana, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(ventana);
}