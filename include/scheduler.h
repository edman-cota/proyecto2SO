#ifndef SCHEDULER_H
#define SCHEDULER_H

#define MAX_PROCESOS 100
#define MAX_CICLOS 1000
#define MAX_PID_LEN 10

typedef struct
{
    char pid[MAX_PID_LEN];
    int burst_time;
    int arrival_time;
    int priority;

    int start_time;
    int finish_time;
    int waiting_time;
    int turnaround_time;
} Process;

typedef struct
{
    char pid[MAX_PID_LEN];
} TimelineEntry;

typedef struct
{
    char pid[MAX_PID_LEN];
    int burst_time;
    int arrival_time;
    int priority;
} ProcessB;

typedef struct
{
    int instante;
    int pid;
    int tipo; // 1=solicita, 2=libera
    char recurso[20];
    int valid; // 1=ejecutada, 0=bloqueada
} AccionB;

typedef struct
{
    char name[32];
    int contador; // cantidad de recursos disponibles (semaforo o mutex=1)
} Resource;

typedef struct
{
    char name[50];
    int estado;
    int contador; // cantidad de recursos disponibles (semaforo o mutex=1)
} Recurso;

typedef enum
{
    READ,
    WRITE
} ActionType;

typedef struct
{
    char pid[MAX_PID_LEN];
    ActionType action;
    char resource[32];
    int ciclo;
} Action;

int simular_fifo(Process *procesos, int n, TimelineEntry timeline[], int *ciclos);
int simular_sjf(Process *procesos, int n, TimelineEntry timeline[], int *ciclos);
int simular_srt(Process *procesos, int n, TimelineEntry timeline[], int *ciclos);
int simular_rr(Process *procesos, int n, int quantum, TimelineEntry timeline[], int *ciclos);
int simular_priority(Process *procesos, int n, TimelineEntry timeline[], int *ciclos);

void calcular_metricas(Process *procesos, int n, double *avg_wt, double *avg_tt, double *avg_ct);

#endif
