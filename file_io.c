#include "pokemon_battle.h"

void resetearRepeticion(EstadoJuego* estado) {
    if (!estado) return;
    estado->totalAccionesReplay = 0;
    estado->indiceAccionReplay = 0;
    estado->semillaReplay = 0;
    memset(estado->accionesReplay, 0, sizeof(estado->accionesReplay));
}

int cargarAccionesDeArchivo(EstadoJuego* estado, const char* archivo) {
    FILE* f;
    char linea[256];
    char nombresInferidos[2][MAX_NOMBRE];
    int nombresInferidosCount = 0;
    int jugadoresHeader = 0;

    if (!estado) return 0;

    f = fopen(archivo, "r");
    if (!f) return 0;

    resetearRepeticion(estado);
    inicializarCola(&estado->cola);

    while (fgets(linea, sizeof(linea), f)) {
        char* turnoTxt;
        char* atacante;
        char* accion;
        char* finLinea;

        finLinea = strpbrk(linea, "\r\n");
        if (finLinea) *finLinea = '\0';

        if (linea[0] == '\0' || linea[0] == '#') continue;

        if (strncmp(linea, "SEED;", 5) == 0) {
            estado->semillaReplay = (unsigned int)strtoul(linea + 5, NULL, 10);
            continue;
        }

        if (strncmp(linea, "P1;", 3) == 0 || strncmp(linea, "P2;", 3) == 0) {
            char* tag = strtok(linea, ";");
            char* nombre = strtok(NULL, ";");
            char* tipo = strtok(NULL, ";");
            char* emoji = strtok(NULL, ";");
            char* saludTxt = strtok(NULL, ";");
            char* defensaTxt = strtok(NULL, ";");
            char* velocidadTxt = strtok(NULL, ";");
            char* entrenadorTxt = strtok(NULL, ";");
            Pokemon* p;
            (void)tag;

            if (!nombre || !tipo || !emoji || !saludTxt || !defensaTxt || !velocidadTxt) {
                continue;
            }

            p = crearPokemon(nombre, tipo, emoji,
                             atoi(saludTxt), atoi(defensaTxt), atoi(velocidadTxt));
            if (p) {
                if (entrenadorTxt && entrenadorTxt[0] != '\0') {
                    copiarTextoSeguro(p->entrenador, MAX_ENTRENADOR, entrenadorTxt);
                }
                cargarAccionesDefault(p);
                encolarJugador(&estado->cola, p);
                jugadoresHeader++;
            }
            continue;
        }

        turnoTxt = strtok(linea, ";");
        atacante = strtok(NULL, ";");
        accion   = strtok(NULL, ";");
        if (!turnoTxt || !atacante || !accion) continue;

        if (nombresInferidosCount < 2) {
            int yaExiste = 0;
            int i;
            for (i = 0; i < nombresInferidosCount; i++) {
                if (strcmp(nombresInferidos[i], atacante) == 0) {
                    yaExiste = 1;
                    break;
                }
            }
            if (!yaExiste) {
                copiarTextoSeguro(nombresInferidos[nombresInferidosCount],
                                 MAX_NOMBRE, atacante);
                nombresInferidosCount++;
            }
        }

        if (estado->totalAccionesReplay >= MAX_ACCIONES_REPLAY) break;

        copiarTextoSeguro(estado->accionesReplay[estado->totalAccionesReplay].atacante,
                          MAX_NOMBRE, atacante);
        copiarTextoSeguro(estado->accionesReplay[estado->totalAccionesReplay].accion,
                          40, accion);
        estado->totalAccionesReplay++;
    }

    fclose(f);

    if (estado->cola.total == 0 && nombresInferidosCount == 2) {
        Pokemon* p1 = crearPokemonDesdeNombreTemplate(nombresInferidos[0]);
        Pokemon* p2 = crearPokemonDesdeNombreTemplate(nombresInferidos[1]);
        if (p1 && p2) {
            encolarJugador(&estado->cola, p1);
            encolarJugador(&estado->cola, p2);
        } else {
            if (p1) liberarPokemon(p1);
            if (p2) liberarPokemon(p2);
        }
    }

    if (estado->cola.total < 2 || estado->totalAccionesReplay == 0) {
        liberarJuego(estado);
        return 0;
    }

    (void)jugadoresHeader;
    return estado->totalAccionesReplay;
}

void iniciarRegistroAcciones(const EstadoJuego* estado, const char* archivo) {
    FILE* f;
    if (!estado || estado->modoRepeticion) return;

    f = fopen(archivo, "w");
    if (!f) return;

    fprintf(f, "SEED;%u\n", estado->semillaPartida);
    if (estado->cola.frente && estado->cola.total >= 2) {
        Pokemon* p1 = estado->cola.frente;
        Pokemon* p2 = estado->cola.frente->sig;

        fprintf(f, "P1;%s;%s;%s;%d;%d;%d;%s\n",
                p1->nombre, p1->tipo, p1->emoji,
                p1->saludMax, p1->defensa, p1->velocidad, p1->entrenador);
        fprintf(f, "P2;%s;%s;%s;%d;%d;%d;%s\n",
                p2->nombre, p2->tipo, p2->emoji,
                p2->saludMax, p2->defensa, p2->velocidad, p2->entrenador);
    }
    fclose(f);
}

void registrarAccionEnArchivo(const EstadoJuego* estado,
                              const Pokemon* atacante,
                              const Accion* accion,
                              const char* archivo) {
    FILE* f;
    if (!estado || !atacante || !accion || estado->modoRepeticion) return;

    f = fopen(archivo, "a");
    if (!f) return;

    fprintf(f, "%d;%s;%s\n", estado->turnoActual, atacante->nombre, accion->nombre);
    fclose(f);
}

void guardarHistorial(HistorialCombate* hist, const char* archivo) {
    FILE* f = fopen(archivo, "w");
    int i;

    if (!hist) return;
    if (!f) { printf("No se pudo guardar el historial.\n"); return; }

    fprintf(f, "=== HISTORIAL DE COMBATE POKEMON ===\n\n");
    for (i = 0; i < hist->cantidad; i++) {
        fprintf(f, "%s\n", hist->eventos[i]);
    }
    fclose(f);
    printf(COLOR_VERDE "\n  Historial guardado en: %s\n" COLOR_RESET, archivo);
}
