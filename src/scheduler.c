#include "scheduler.h"
#include <string.h>

int simular_fifo(Process *procesos, int n, TimelineEntry timeline[], int *ciclos)
{
    int tiempo = 0, count = 0;

    for (int i = 0; i < n - 1; ++i)
    {
        for (int j = 0; j < n - i - 1; ++j)
        {
            if (procesos[j].arrival_time > procesos[j + 1].arrival_time)
            {
                Process tmp = procesos[j];
                procesos[j] = procesos[j + 1];
                procesos[j + 1] = tmp;
            }
        }
    }

    for (int i = 0; i < n; ++i)
    {
        if (procesos[i].arrival_time > tiempo)
            tiempo = procesos[i].arrival_time;

        procesos[i].start_time = tiempo;
        for (int j = 0; j < procesos[i].burst_time; ++j)
        {
            strcpy(timeline[count].pid, procesos[i].pid);
            count++;
            tiempo++;
        }
        procesos[i].finish_time = tiempo;
        procesos[i].waiting_time = procesos[i].start_time - procesos[i].arrival_time;
        procesos[i].turnaround_time = procesos[i].finish_time - procesos[i].arrival_time;
    }

    *ciclos = count;
    return 0;
}

int simular_sjf(Process *procesos, int n, TimelineEntry timeline[], int *ciclos)
{
    int completado[n], tiempo = 0, count = 0, hechos = 0;

    for (int i = 0; i < n; i++)
        completado[i] = 0;

    while (hechos < n)
    {
        int idx = -1, min_bt = 1e9;

        for (int i = 0; i < n; i++)
        {
            if (!completado[i] && procesos[i].arrival_time <= tiempo && procesos[i].burst_time < min_bt)
            {
                min_bt = procesos[i].burst_time;
                idx = i;
            }
        }

        if (idx == -1)
        {
            tiempo++;
            continue;
        }

        procesos[idx].start_time = tiempo;
        for (int j = 0; j < procesos[idx].burst_time; j++)
        {
            strcpy(timeline[count++].pid, procesos[idx].pid);
            tiempo++;
        }
        procesos[idx].finish_time = tiempo;
        procesos[idx].waiting_time = procesos[idx].start_time - procesos[idx].arrival_time;
        procesos[idx].turnaround_time = procesos[idx].finish_time - procesos[idx].arrival_time;
        completado[idx] = 1;
        hechos++;
    }

    *ciclos = count;
    return 0;
}

int simular_srt(Process *procesos, int n, TimelineEntry timeline[], int *ciclos)
{
    int tiempo = 0, completados = 0, count = 0;
    int rem_bt[n];
    for (int i = 0; i < n; i++)
        rem_bt[i] = procesos[i].burst_time;
    int start_set[n];
    for (int i = 0; i < n; i++)
        start_set[i] = 0;

    while (completados < n)
    {
        int idx = -1, min = 1e9;

        for (int i = 0; i < n; i++)
        {
            if (procesos[i].arrival_time <= tiempo && rem_bt[i] > 0 && rem_bt[i] < min)
            {
                min = rem_bt[i];
                idx = i;
            }
        }

        if (idx == -1)
        {
            tiempo++;
            continue;
        }

        if (!start_set[idx])
        {
            procesos[idx].start_time = tiempo;
            start_set[idx] = 1;
        }

        rem_bt[idx]--;
        strcpy(timeline[count++].pid, procesos[idx].pid);
        tiempo++;

        if (rem_bt[idx] == 0)
        {
            procesos[idx].finish_time = tiempo;
            procesos[idx].turnaround_time = procesos[idx].finish_time - procesos[idx].arrival_time;
            procesos[idx].waiting_time = procesos[idx].turnaround_time - procesos[idx].burst_time;
            completados++;
        }
    }

    *ciclos = count;
    return 0;
}

int simular_rr(Process *procesos, int n, int quantum, TimelineEntry timeline[], int *ciclos)
{
    int tiempo = 0, completados = 0, count = 0;
    int rem_bt[n], ready[n];
    for (int i = 0; i < n; i++)
        rem_bt[i] = procesos[i].burst_time;

    int cola[MAX_CICLOS], frente = 0, fin = 0, en_cola[n];
    for (int i = 0; i < n; i++)
        en_cola[i] = 0;

    for (int i = 0; i < n; i++)
    {
        if (procesos[i].arrival_time == 0)
        {
            cola[fin++] = i;
            en_cola[i] = 1;
        }
    }

    int start_set[n];
    for (int i = 0; i < n; i++)
        start_set[i] = 0;

    while (completados < n)
    {
        if (frente == fin)
        {
            tiempo++;
            for (int i = 0; i < n; i++)
            {
                if (!en_cola[i] && procesos[i].arrival_time == tiempo)
                {
                    cola[fin++] = i;
                    en_cola[i] = 1;
                }
            }
            continue;
        }

        int idx = cola[frente++];
        en_cola[idx] = 0;

        if (!start_set[idx])
        {
            procesos[idx].start_time = tiempo > procesos[idx].arrival_time ? tiempo : procesos[idx].arrival_time;
            tiempo = procesos[idx].start_time;
            start_set[idx] = 1;
        }

        int exec_time = (rem_bt[idx] < quantum) ? rem_bt[idx] : quantum;
        for (int j = 0; j < exec_time; j++)
        {
            strcpy(timeline[count++].pid, procesos[idx].pid);
            tiempo++;
        }

        rem_bt[idx] -= exec_time;

        for (int i = 0; i < n; i++)
        {
            if (!en_cola[i] && rem_bt[i] > 0 && procesos[i].arrival_time > tiempo - exec_time && procesos[i].arrival_time <= tiempo)
            {
                cola[fin++] = i;
                en_cola[i] = 1;
            }
        }

        if (rem_bt[idx] > 0)
        {
            cola[fin++] = idx;
            en_cola[idx] = 1;
        }
        else
        {
            procesos[idx].finish_time = tiempo;
            procesos[idx].turnaround_time = procesos[idx].finish_time - procesos[idx].arrival_time;
            procesos[idx].waiting_time = procesos[idx].turnaround_time - procesos[idx].burst_time;
            completados++;
        }
    }

    *ciclos = count;
    return 0;
}

int simular_priority(Process *procesos, int n, TimelineEntry timeline[], int *ciclos)
{
    int completados = 0, tiempo = 0, count = 0;
    int hecho[n];
    for (int i = 0; i < n; i++)
        hecho[i] = 0;

    while (completados < n)
    {
        int idx = -1, mejor = 1e9;

        for (int i = 0; i < n; i++)
        {
            if (!hecho[i] && procesos[i].arrival_time <= tiempo)
            {
                int prio_envejecido = procesos[i].priority - (tiempo - procesos[i].arrival_time) / 5;
                if (prio_envejecido < mejor)
                {
                    mejor = prio_envejecido;
                    idx = i;
                }
            }
        }

        if (idx == -1)
        {
            tiempo++;
            continue;
        }

        procesos[idx].start_time = tiempo;
        for (int j = 0; j < procesos[idx].burst_time; j++)
        {
            strcpy(timeline[count++].pid, procesos[idx].pid);
            tiempo++;
        }

        procesos[idx].finish_time = tiempo;
        procesos[idx].waiting_time = procesos[idx].start_time - procesos[idx].arrival_time;
        procesos[idx].turnaround_time = procesos[idx].finish_time - procesos[idx].arrival_time;
        hecho[idx] = 1;
        completados++;
    }

    *ciclos = count;
    return 0;
}

void calcular_metricas(Process *procesos, int n, double *avg_wt, double *avg_tt, double *avg_ct)
{
    double total_wt = 0, total_tt = 0, total_ct = 0;
    for (int i = 0; i < n; i++)
    {
        total_wt += procesos[i].waiting_time;
        total_tt += procesos[i].turnaround_time;
        total_ct += procesos[i].finish_time;
    }
    *avg_wt = total_wt / n;
    *avg_tt = total_tt / n;
    *avg_ct = total_ct / n;
}