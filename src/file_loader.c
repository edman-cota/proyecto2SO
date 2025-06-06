#include <stdio.h>
#include <string.h>
#include "file_loader.h"

int cargar_procesos(const char *filename, Process *procesos, int max)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error abriendo archivo");
        return -1;
    }

    int count = 0;
    while (fscanf(file, "%[^,], %d, %d, %d\n",
                  procesos[count].pid,
                  &procesos[count].burst_time,
                  &procesos[count].arrival_time,
                  &procesos[count].priority) == 4)
    {
        count++;
        if (count >= max)
            break;
    }

    fclose(file);
    return count;
}
