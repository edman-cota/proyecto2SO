#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include "gui.h"
#include "scheduler.h"
#include "file_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCESOS 100
#define MAX_LINEA 128
#define MAX_PID_LEN 16

#define MAX_LINEAS 5 // Máximo de algoritmos activos

#define MAX_RECURSOS 50

Recurso recursos[MAX_RECURSOS];
int num_recursos = 0;

#define MAX_ACCIONES 500

Action acciones[MAX_ACCIONES];
int num_acciones = 0;

Recurso recursos_b[MAX_RECURSOS];
int num_recursos_b;

AccionB acciones_b[MAX_ACCIONES];
int num_acciones_b;

typedef struct
{
    int id;
    char nombre[50];
    // otras propiedades que tengas en procesos
} Proceso;

Proceso procesos_b[MAX_PROCESOS];
int num_procesos_b = 0;

// Función auxiliar para convertir string a ActionType
ActionType string_to_action_type(const char *str)
{
    if (strcmp(str, "READ") == 0)
        return READ;
    if (strcmp(str, "WRITE") == 0)
        return WRITE;
    // Por defecto:
    return READ;
}

#define MAX_CICLOS 1000
typedef enum
{
    WAITING,
    ACCESSED
} EstadoAccion;

EstadoAccion timeline[MAX_PROCESOS][MAX_CICLOS];

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
            gtk_widget_override_color(label, GTK_STATE_FLAG_NORMAL, &(GdkRGBA){1.0, 1.0, 1.0, 1.0});
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

int cargar_procesos_b(const char *filename)
{
    FILE *file = fopen("data/procesos.txt", "r");
    if (!file)
    {
        perror("Error al abrir archivo de procesos");
        return 0;
    }
    char linea[MAX_LINEA];
    num_procesos = 0;
    while (fgets(linea, sizeof(linea), file) && num_procesos < MAX_PROCESOS)
    {
        // Formato: P1, 8, 0, 1
        char pid[MAX_PID_LEN];
        int bt, at, prio;
        if (sscanf(linea, "%[^,], %d, %d, %d", pid, &bt, &at, &prio) == 4)
        {
            strcpy(procesos[num_procesos].pid, pid);
            procesos[num_procesos].burst_time = bt;
            procesos[num_procesos].arrival_time = at;
            procesos[num_procesos].priority = prio;
            num_procesos++;
        }
    }
    fclose(file);
    return 1;
}

int cargar_recursos_b(const char *filename)
{
    FILE *file = fopen("data/recursos.txt", "r");
    if (!file)
    {
        perror("Error al abrir archivo de recursos");
        return 0;
    }
    char linea[MAX_LINEA];
    num_recursos = 0;
    while (fgets(linea, sizeof(linea), file) && num_recursos < MAX_RECURSOS)
    {
        char name[32];
        int contador;
        if (sscanf(linea, "%[^,], %d", name, &contador) == 2)
        {
            strcpy(recursos[num_recursos].name, name);
            recursos[num_recursos].contador = contador;
            num_recursos++;
        }
    }
    fclose(file);
    return 1;
}

int cargar_acciones_b(const char *filename)
{
    FILE *file = fopen("data/acciones.txt", "r");
    if (!file)
    {
        perror("Error al abrir archivo de acciones");
        return 0;
    }
    char linea[MAX_LINEA];
    num_acciones = 0;
    while (fgets(linea, sizeof(linea), file) && num_acciones < MAX_ACCIONES)
    {
        char pid[MAX_PID_LEN], accion_str[8], recurso[32];
        int ciclo;
        if (sscanf(linea, "%[^,], %[^,], %[^,], %d", pid, accion_str, recurso, &ciclo) == 4)
        {
            strcpy(acciones[num_acciones].pid, pid);
            acciones[num_acciones].action = string_to_action_type(accion_str);
            strcpy(acciones[num_acciones].resource, recurso);
            acciones[num_acciones].ciclo = ciclo;
            num_acciones++;
        }
    }
    fclose(file);
    return 1;
}

int buscar_recurso(const char *name)
{
    for (int i = 0; i < num_recursos; i++)
    {
        if (strcmp(recursos[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1; // no encontrado
}

void simular_sincronizacion()
{
    // Inicializar todos los recursos como libres
    for (int i = 0; i < num_recursos_b; i++)
    {
        recursos_b[i].estado = 0;
    }

    // Recorrer todas las acciones en orden de instante
    for (int i = 0; i < num_acciones_b; i++)
    {
        int tipo = acciones_b[i].tipo;
        char *nombre_recurso = acciones_b[i].recurso;

        // Buscar el índice del recurso
        int recurso_idx = -1;
        for (int j = 0; j < num_recursos_b; j++)
        {
            if (strcmp(nombre_recurso, recursos_b[j].nombre) == 0)
            {
                recurso_idx = j;
                break;
            }
        }

        if (recurso_idx == -1)
        {
            printf("❌ Recurso %s no encontrado.\n", nombre_recurso);
            continue;
        }

        if (tipo == 1)
        { // Solicita
            if (recursos_b[recurso_idx].estado == 0)
            {
                // Recurso libre → ocuparlo
                recursos_b[recurso_idx].estado = 1;
                acciones_b[i].valid = 1; // Acción válida
            }
            else
            {
                // Recurso ocupado → acción bloqueada
                acciones_b[i].valid = 0;
            }
        }
        else if (tipo == 2)
        {                                       // Libera
            recursos_b[recurso_idx].estado = 0; // Liberar
            acciones_b[i].valid = 1;
        }
    }
}

void on_cargar_archivos_clicked(GtkWidget *widget, gpointer data)
{
    cargar_procesos_b("procesos.txt");
    cargar_recursos_b("recursos.txt");
    cargar_acciones_b("acciones.txt");
    g_print("✔ Archivos cargados correctamente.\n");
}

void on_ejecutar_simulacion_clicked(GtkWidget *widget, gpointer data)
{
    simular_sincronizacion();      // Simulación lógica
    gtk_widget_queue_draw(widget); // Redibuja
}

gboolean dibujar_sincronizacion(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    int y_offset = 20;
    for (int i = 0; i < num_procesos_b; i++)
    {
        // Dibujar nombre del proceso
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_move_to(cr, 10, y_offset + i * 40);
        cairo_show_text(cr, procesos_b[i].nombre);
    }

    for (int i = 0; i < num_acciones_b; i++)
    {
        if (acciones_b[i].valid == 0)
            cairo_set_source_rgb(cr, 1, 0, 0); // Rojo = bloqueada

        int x = acciones_b[i].instante * 40 + 100;
        int y = acciones_b[i].pid * 40 + y_offset;
        char *recurso = acciones_b[i].recurso;
        int tipo = acciones_b[i].tipo; // 1=solicita, 2=libera

        // Color diferente por tipo de acción
        if (tipo == 1)
            cairo_set_source_rgb(cr, 0.2, 0.6, 0.9); // Azul (solicita)
        else
            cairo_set_source_rgb(cr, 0.2, 0.8, 0.2); // Verde (libera)

        cairo_rectangle(cr, x, y, 30, 30);
        cairo_fill(cr);

        // Texto (letra del recurso)
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_move_to(cr, x + 8, y + 20);
        cairo_show_text(cr, recurso);
    }

    return FALSE;
}

void mostrar_ventana_sincronizacion()
{
    GtkWidget *ventana;
    GtkWidget *caja_vertical;
    GtkWidget *caja_botones;
    GtkWidget *btn_cargar;
    GtkWidget *btn_run;
    GtkWidget *scroll;
    GtkWidget *area_dibujo;

    // Crear ventana
    ventana = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(ventana), "Simulador B - Mecanismos de Sincronización");
    gtk_window_set_default_size(GTK_WINDOW(ventana), 900, 400);
    gtk_window_set_position(GTK_WINDOW(ventana), GTK_WIN_POS_CENTER);
    g_signal_connect(ventana, "destroy", G_CALLBACK(gtk_widget_destroy), ventana);

    // Contenedor principal
    caja_vertical = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(ventana), caja_vertical);

    // Contenedor de botones
    caja_botones = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(caja_vertical), caja_botones, FALSE, FALSE, 5);

    // Botón cargar
    btn_cargar = gtk_button_new_with_label("📂 Cargar archivos");
    gtk_box_pack_start(GTK_BOX(caja_botones), btn_cargar, FALSE, FALSE, 5);
    g_signal_connect(btn_cargar, "clicked", G_CALLBACK(on_cargar_archivos_clicked), NULL);

    // Botón correr
    btn_run = gtk_button_new_with_label("▶ Correr");
    gtk_box_pack_start(GTK_BOX(caja_botones), btn_run, FALSE, FALSE, 5);
    g_signal_connect(btn_run, "clicked", G_CALLBACK(on_ejecutar_simulacion_clicked), NULL);

    // Área de dibujo con scroll
    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scroll, 800, 300);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(caja_vertical), scroll, TRUE, TRUE, 0);

    // Área de dibujo
    area_dibujo = gtk_drawing_area_new();
    gtk_widget_set_size_request(area_dibujo, 1500, 300); // Ajusta el tamaño según cantidad de acciones
    gtk_container_add(GTK_CONTAINER(scroll), area_dibujo);

    g_signal_connect(area_dibujo, "draw", G_CALLBACK(dibujar_sincronizacion), NULL);

    gtk_widget_show_all(ventana);
}

// void mostrar_ventana_sincronizacion()
// {
//     GtkWidget *ventana = gtk_window_new(GTK_WINDOW_TOPLEVEL);
//     gtk_window_set_title(GTK_WINDOW(ventana), "Simulador B: Mecanismos de Sincronización");
//     gtk_window_set_default_size(GTK_WINDOW(ventana), 1000, 600);

//     GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

//     // Controles carga y modo
//     GtkWidget *controles = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

//     GtkWidget *modo_combo = gtk_combo_box_text_new();
//     gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(modo_combo), "Mutex");
//     gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(modo_combo), "Semáforo");
//     gtk_combo_box_set_active(GTK_COMBO_BOX(modo_combo), 0);

//     GtkWidget *btn_proc = gtk_button_new_with_label("Cargar Procesos");
//     GtkWidget *btn_rec = gtk_button_new_with_label("Cargar Recursos");
//     GtkWidget *btn_acc = gtk_button_new_with_label("Cargar Acciones");
//     GtkWidget *btn_run = gtk_button_new_with_label("Run Simulation");

//     gtk_box_pack_start(GTK_BOX(controles), gtk_label_new("Modo:"), FALSE, FALSE, 0);
//     gtk_box_pack_start(GTK_BOX(controles), modo_combo, FALSE, FALSE, 0);
//     gtk_box_pack_start(GTK_BOX(controles), btn_proc, FALSE, FALSE, 0);
//     gtk_box_pack_start(GTK_BOX(controles), btn_rec, FALSE, FALSE, 0);
//     gtk_box_pack_start(GTK_BOX(controles), btn_acc, FALSE, FALSE, 0);
//     gtk_box_pack_start(GTK_BOX(controles), btn_run, FALSE, FALSE, 0);
//     gtk_box_pack_start(GTK_BOX(vbox), controles, FALSE, FALSE, 0);

//     // Scroll horizontal para timeline
//     GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
//     gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

//     GtkWidget *timeline_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
//     gtk_container_add(GTK_CONTAINER(scroll), timeline_area);
//     gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 2);

//     // Métricas o estados
//     GtkWidget *metrics_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
//     gtk_box_pack_start(GTK_BOX(vbox), metrics_box, FALSE, FALSE, 2);

//     gtk_container_add(GTK_CONTAINER(ventana), vbox);

//     // Conectar señales (funciones a implementar)
//     g_signal_connect(btn_proc, "clicked", G_CALLBACK(cargar_procesos_b), NULL);
//     g_signal_connect(btn_rec, "clicked", G_CALLBACK(cargar_recursos_b), NULL);
//     g_signal_connect(btn_acc, "clicked", G_CALLBACK(cargar_acciones_b), NULL);
//     g_signal_connect(ventana, "destroy", G_CALLBACK(gtk_main_quit), NULL);

//     gtk_widget_show_all(ventana);
// }