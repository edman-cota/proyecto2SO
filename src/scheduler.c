#include <stdio.h>
#include "scheduler.h"
#include <stdbool.h>

// Ordenar por arrival_time
void ordenar_por_llegada(Process *procesos, int n)
{
    for (int i = 0; i < n - 1; i++)
    {
        for (int j = 0; j < n - i - 1; j++)
        {
            if (procesos[j].arrival_time > procesos[j + 1].arrival_time)
            {
                Process temp = procesos[j];
                procesos[j] = procesos[j + 1];
                procesos[j + 1] = temp;
            }
        }
    }
}

void fifo(Process *procesos, int n)
{
    ordenar_por_llegada(procesos, n);
    int tiempo = 0;

    for (int i = 0; i < n; i++)
    {
        if (procesos[i].arrival_time > tiempo)
        {
            tiempo = procesos[i].arrival_time;
        }
        procesos[i].start_time = tiempo;
        procesos[i].finish_time = tiempo + procesos[i].burst_time;
        procesos[i].waiting_time = procesos[i].start_time - procesos[i].arrival_time;
        procesos[i].turnaround_time = procesos[i].finish_time - procesos[i].arrival_time;
        tiempo = procesos[i].finish_time;
    }

    imprimir_diagrama_gantt(procesos, n);
    imprimir_metricas(procesos, n);
}

// SRT (Shortest Remaining Time)
void srt(Process *procesos, int n)
{
    int tiempo = 0, completados = 0;
    int tiempo_restante[n];
    for (int i = 0; i < n; i++)
        tiempo_restante[i] = procesos[i].burst_time;

    int ultimo = -1;
    while (completados < n)
    {
        int idx = -1, menor_restante = 1e9;
        for (int i = 0; i < n; i++)
        {
            if (procesos[i].arrival_time <= tiempo && tiempo_restante[i] > 0 && tiempo_restante[i] < menor_restante)
            {
                menor_restante = tiempo_restante[i];
                idx = i;
            }
        }

        if (idx == -1)
        {
            tiempo++;
            continue;
        }

        if (tiempo_restante[idx] == procesos[idx].burst_time)
            procesos[idx].start_time = tiempo;

        tiempo_restante[idx]--;
        tiempo++;

        if (tiempo_restante[idx] == 0)
        {
            procesos[idx].finish_time = tiempo;
            procesos[idx].turnaround_time = procesos[idx].finish_time - procesos[idx].arrival_time;
            procesos[idx].waiting_time = procesos[idx].turnaround_time - procesos[idx].burst_time;
            completados++;
        }
    }

    imprimir_diagrama_gantt(procesos, n);
    imprimir_metricas(procesos, n);
}

void sjf(Process *procesos, int n)
{
    bool completado[n];
    for (int i = 0; i < n; i++)
        completado[i] = false;

    int tiempo = 0, completados = 0;

    while (completados < n)
    {
        int idx = -1, menor_bt = 1e9;

        for (int i = 0; i < n; i++)
        {
            if (!completado[i] && procesos[i].arrival_time <= tiempo && procesos[i].burst_time < menor_bt)
            {
                menor_bt = procesos[i].burst_time;
                idx = i;
            }
        }

        if (idx == -1)
        {
            tiempo++;
        }
        else
        {
            procesos[idx].start_time = tiempo;
            procesos[idx].finish_time = tiempo + procesos[idx].burst_time;
            procesos[idx].waiting_time = procesos[idx].start_time - procesos[idx].arrival_time;
            procesos[idx].turnaround_time = procesos[idx].finish_time - procesos[idx].arrival_time;
            tiempo = procesos[idx].finish_time;
            completado[idx] = true;
            completados++;
        }
    }

    imprimir_diagrama_gantt(procesos, n);
    imprimir_metricas(procesos, n);
}

void round_robin(Process *procesos, int n, int quantum)
{
    int tiempo = 0, completados = 0;
    int tiempo_restante[n];
    bool iniciado[n];
    for (int i = 0; i < n; i++)
    {
        tiempo_restante[i] = procesos[i].burst_time;
        iniciado[i] = false;
    }

    int cola[n];
    int frente = 0, fin = 0;
    bool en_cola[n];
    for (int i = 0; i < n; i++)
        en_cola[i] = false;

    // Agrega los primeros procesos
    for (int i = 0; i < n; i++)
    {
        if (procesos[i].arrival_time == 0)
        {
            cola[fin++] = i;
            en_cola[i] = true;
        }
    }

    while (completados < n)
    {
        if (frente == fin)
        {
            tiempo++;
            for (int i = 0; i < n; i++)
            {
                if (procesos[i].arrival_time == tiempo && !en_cola[i])
                {
                    cola[fin++] = i;
                    en_cola[i] = true;
                }
            }
            continue;
        }

        int idx = cola[frente++];
        en_cola[idx] = false;

        if (!iniciado[idx])
        {
            procesos[idx].start_time = (tiempo > procesos[idx].arrival_time) ? tiempo : procesos[idx].arrival_time;
            tiempo = procesos[idx].start_time;
            iniciado[idx] = true;
        }

        int tiempo_ejec = (tiempo_restante[idx] < quantum) ? tiempo_restante[idx] : quantum;
        tiempo_restante[idx] -= tiempo_ejec;
        tiempo += tiempo_ejec;

        for (int i = 0; i < n; i++)
        {
            if (!iniciado[i] && !en_cola[i] && procesos[i].arrival_time > tiempo - tiempo_ejec && procesos[i].arrival_time <= tiempo)
            {
                cola[fin++] = i;
                en_cola[i] = true;
            }
        }

        if (tiempo_restante[idx] > 0)
        {
            cola[fin++] = idx;
            en_cola[idx] = true;
        }
        else
        {
            procesos[idx].finish_time = tiempo;
            procesos[idx].turnaround_time = procesos[idx].finish_time - procesos[idx].arrival_time;
            procesos[idx].waiting_time = procesos[idx].turnaround_time - procesos[idx].burst_time;
            completados++;
        }
    }

    imprimir_diagrama_gantt(procesos, n);
    imprimir_metricas(procesos, n);
}

// PRIORITY con envejecimiento
void priority(Process *procesos, int n)
{
    int tiempo = 0, completados = 0;
    bool completado[n];
    for (int i = 0; i < n; i++)
        completado[i] = false;

    while (completados < n)
    {
        int idx = -1, mejor_prio = 1e9;
        for (int i = 0; i < n; i++)
        {
            if (!completado[i] && procesos[i].arrival_time <= tiempo)
            {
                int prioridad_con_envejecimiento = procesos[i].priority - (tiempo - procesos[i].arrival_time) / 5;
                if (prioridad_con_envejecimiento < mejor_prio)
                {
                    mejor_prio = prioridad_con_envejecimiento;
                    idx = i;
                }
            }
        }

        if (idx == -1)
        {
            tiempo++;
        }
        else
        {
            procesos[idx].start_time = tiempo;
            procesos[idx].finish_time = tiempo + procesos[idx].burst_time;
            procesos[idx].waiting_time = procesos[idx].start_time - procesos[idx].arrival_time;
            procesos[idx].turnaround_time = procesos[idx].finish_time - procesos[idx].arrival_time;
            tiempo = procesos[idx].finish_time;
            completado[idx] = true;
            completados++;
        }
    }

    imprimir_diagrama_gantt(procesos, n);
    imprimir_metricas(procesos, n);
}
