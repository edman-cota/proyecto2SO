#ifndef SCHEDULER_H
#define SCHEDULER_H

#define MAX_PID_LEN 10
#define MAX_PROCESOS 100

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

void fifo(Process *procesos, int n);
void sjf(Process *procesos, int n);
void srt(Process *procesos, int n);
void round_robin(Process *procesos, int n, int quantum);
void priority(Process *procesos, int n);

#endif