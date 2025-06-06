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
