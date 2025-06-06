#include <stdio.h>
#include "scheduler.h"
#include "file_loader.h"
#include "gui.h"

int main()
{
    Process procesos[MAX_PROCESOS];
    int cantidad = cargar_procesos("data/procesos.txt", procesos, MAX_PROCESOS);

    if (cantidad <= 0)
    {
        printf("No se cargaron procesos.\n");
        return 1;
    }

    fifo(procesos, cantidad);
    iniciar_gui_fifo(procesos, cantidad);

    return 0;
}
