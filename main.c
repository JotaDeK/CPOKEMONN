#include "pokemon_battle.h"

int main(void) {
    srand((unsigned int)time(NULL));

    int continuar = 1;
    while (continuar) {
        EstadoJuego estado;
        memset(&estado, 0, sizeof(EstadoJuego));
        estado.semillaPartida = (unsigned int)time(NULL);
        srand(estado.semillaPartida);

        configurarPartidaConsola(&estado);

        if (estado.cola.total < 2) {
            printf(COLOR_ROJO "  Error: se necesitan al menos 2 jugadores.\n"
                   COLOR_RESET);
            liberarJuego(&estado);
            break;
        }

        if (!estado.modoRepeticion) {
            iniciarRegistroAcciones(&estado, "ACCIONES.TXT");
        }

        jugar(&estado);
        menuPostBatalla(&estado);

        printf(COLOR_AMARILLO "\n  Jugar otra partida? [s/n]: " COLOR_RESET);
        fflush(stdout);
        char buf[10];
        if (!fgets(buf, sizeof(buf), stdin)) break;
        if (buf[0] != 's' && buf[0] != 'S') continuar = 0;

        liberarJuego(&estado);
    }

    printf(COLOR_CYAN "\n  Gracias por jugar! Hasta la proxima batalla!\n\n"
           COLOR_RESET);
    return 0;
}
