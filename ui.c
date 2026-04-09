#include "pokemon_battle.h"

void limpiarPantalla(void) {
    system(CLEAR);
}

void pausar(int ms) {
    SLEEP(ms);
}

void mostrarTitulo(void) {
    printf(COLOR_AMARILLO);
    printf("  +=================================================+\n");
    printf("  |                                                 |\n");
    printf("  |      *** POKEMON BATTLE SIMULATOR ***          |\n");
    printf("  |        Proyecto Pilas & Colas - IS304          |\n");
    printf("  |                                                 |\n");
    printf("  +=================================================+\n");
    printf(COLOR_RESET "\n");
}

void mostrarBarraVida(Pokemon* p) {
    int barraLen = 30;
    int llenos = 0;
    const char* colorBarra;
    double porcentaje;

    if (!p || p->saludMax <= 0) return;

    llenos = (p->salud * barraLen) / p->saludMax;
    if (llenos < 0) llenos = 0;
    if (llenos > barraLen) llenos = barraLen;

    porcentaje = (double)p->salud / p->saludMax;
    if (porcentaje > 0.50)      colorBarra = COLOR_VERDE;
    else if (porcentaje > 0.25) colorBarra = COLOR_AMARILLO;
    else                        colorBarra = COLOR_ROJO;

    printf("  %s %-14s (%-12s)%s [", p->emoji, p->entrenador, p->nombre,
           p->estaVivo ? "" : COLOR_ROJO " (KO)" COLOR_RESET);
    printf("%s", colorBarra);

    int i;
    for (i = 0; i < llenos; i++) printf("|");
    for (i = llenos; i < barraLen; i++) printf(".");

    printf(COLOR_RESET "] %s%3d%s/%d HP",
           colorBarra, p->salud, COLOR_RESET, p->saludMax);
    printf("  DEF:%d  Acc:%d\n", p->defensa, p->numAcciones);
}

void mostrarEstadoBatalla(EstadoJuego* estado) {
    if (!estado->modoRepeticion) {
        limpiarPantalla();
        mostrarTitulo();
    } else if (estado->turnoActual == 1) {
        limpiarPantalla();
        mostrarTitulo();
    }

    printf(COLOR_CYAN "  --- CAMPO DE BATALLA --- Turno #%d ---\n\n" COLOR_RESET,
           estado->turnoActual);

    if (!estado->cola.frente) return;
    Pokemon* actual = estado->cola.frente;
    int i;
    for (i = 0; i < estado->cola.total; i++) {
        if (actual == estado->cola.frente && i == 0) {
            printf(COLOR_AMARILLO "  >>> EN TURNO:\n" COLOR_RESET);
        } else if (i == 1) {
            printf(COLOR_BLANCO "\n  --- RIVAL:\n" COLOR_RESET);
        } else if (i == 2) {
            printf(COLOR_GRIS "\n  --- OTROS:\n" COLOR_RESET);
        }
        mostrarBarraVida(actual);
        actual = actual->sig;
    }

    printf("\n" COLOR_GRIS "  ----------------------------------------\n"
           COLOR_RESET);
}

void mostrarMenuAcciones(Pokemon* p) {
    (void)p;
}

void mostrarMensaje(const char* color, const char* msg) {
    printf("%s  %s%s\n", color, msg, COLOR_RESET);
}

void animarAtaque(const char* nombreAtaque, const char* colorAtacante) {
    printf("\n");
    printf("%s  >> Usando: %-20s <<\n" COLOR_RESET,
           colorAtacante, nombreAtaque);
    SLEEP(300);
    printf("  . . .");
    fflush(stdout);
    SLEEP(400);
    printf(" . . .\n");
    SLEEP(300);
}

void agregarHistorial(HistorialCombate* hist, const char* msg) {
    if (!hist || !msg) return;
    if (hist->cantidad < MAX_HISTORIAL) {
        copiarTextoSeguro(hist->eventos[hist->cantidad],
                          (int)sizeof(hist->eventos[hist->cantidad]), msg);
        hist->cantidad++;
    }
}

void pedirNombreJugador(char* destino, int tam, const char* nombreDefault,
                        const char* prompt) {
    char buf[80];
    int i;

    if (!destino || tam <= 1) return;
    destino[0] = '\0';

    printf("%s", prompt);
    fflush(stdout);

    if (!fgets(buf, sizeof(buf), stdin)) {
        copiarTextoSeguro(destino, tam, nombreDefault);
        return;
    }

    for (i = 0; buf[i] != '\0'; i++) {
        if (buf[i] == '\n' || buf[i] == '\r') {
            buf[i] = '\0';
            break;
        }
    }

    if (buf[0] == '\0') {
        copiarTextoSeguro(destino, tam, nombreDefault);
        return;
    }

    copiarTextoSeguro(destino, tam, buf);
}

void etiquetaParticipante(const Pokemon* p, char* out, int outSize) {
    if (!p || !out || outSize <= 0) return;

    if (strcmp(p->entrenador, p->nombre) == 0) {
        snprintf(out, outSize, "%s", p->nombre);
    } else {
        snprintf(out, outSize, "%s (%s)", p->entrenador, p->nombre);
    }
}

void mostrarResultado(EstadoJuego* estado) {
    limpiarPantalla();
    mostrarTitulo();

    printf(COLOR_AMARILLO "\n  *** FIN DE LA BATALLA ***\n\n" COLOR_RESET);

    Pokemon* ganador = NULL;
    if (estado->cola.frente) {
        Pokemon* actual = estado->cola.frente;
        int i;
        for (i = 0; i < estado->cola.total; i++) {
            if (actual->estaVivo) { ganador = actual; break; }
            actual = actual->sig;
        }
    }

    if (ganador) {
        printf(COLOR_VERDE "  GANADOR: %s %s!\n" COLOR_RESET,
               ganador->emoji, ganador->nombre);
        printf("  Vida restante: %d/%d HP\n", ganador->salud, ganador->saludMax);
        printf("  Defensa final: %d\n\n", ganador->defensa);
    } else {
        printf(COLOR_ROJO "  Empate! Todos fueron derrotados.\n\n" COLOR_RESET);
    }

    printf("  Total de turnos: %d\n", estado->turnoActual);
    printf("  Eventos registrados: %d\n\n", estado->historial.cantidad);
}

void mostrarHistorial(HistorialCombate* hist) {
    int i;
    printf(COLOR_CYAN "\n  === HISTORIAL DE COMBATE ===\n\n" COLOR_RESET);
    if (!hist) return;
    for (i = 0; i < hist->cantidad; i++) {
        printf("  %3d. %s\n", i + 1, hist->eventos[i]);
    }
    printf("\n");
}

void mostrarMenuPrincipal(void) {
    limpiarPantalla();
    mostrarTitulo();

    printf("  Selecciona el modo de juego:\n\n");
    printf(COLOR_VERDE "  [1]" COLOR_RESET " Jugador 1 vs Jugador 2 (dos humanos)\n");
    printf(COLOR_AZUL  "  [2]" COLOR_RESET " Jugador vs Maquina\n");
    printf(COLOR_MAGENTA "  [3]" COLOR_RESET " Maquina vs Maquina (espectador)\n");
    printf(COLOR_AMARILLO "  [4]" COLOR_RESET " Repeticion desde archivo (ACCIONES.TXT)\n");
    printf(COLOR_ROJO  "  [5]" COLOR_RESET " Salir\n");
    printf("\n  Opcion: ");
    fflush(stdout);
}

void mostrarPokemonDisponibles(void) {
    const PokemonTemplate* disponibles = obtenerPokemonDisponibles();
    int cantidad = obtenerCantidadPokemonDisponibles();
    int i;

    printf("\n  Pokemon disponibles:\n\n");
    for (i = 0; i < cantidad; i++) {
        const char* colorTipo = colorTipoPokemon(disponibles[i].tipo);

        printf("  %s[%2d]%s %s%-12s%s | Tipo: %s%-9s%s | HP:%3d | DEF:%2d | VEL:%3d\n",
               COLOR_CYAN, i + 1, COLOR_RESET,
               COLOR_BLANCO, disponibles[i].nombre, COLOR_RESET,
               colorTipo, disponibles[i].tipo, COLOR_RESET,
               disponibles[i].salud,
               disponibles[i].defensa,
               disponibles[i].velocidad);
    }
    printf("\n");
}

Pokemon* elegirPokemon(const char* nombreJugador, int esHumano) {
    int opcion;
    int cantidad = obtenerCantidadPokemonDisponibles();

    mostrarPokemonDisponibles();

    if (esHumano) {
        printf("  %s, elige tu Pokemon (1-%d): ",
               nombreJugador, cantidad);
        fflush(stdout);
        char buf[10];
        if (!fgets(buf, sizeof(buf), stdin)) opcion = 1;
        else opcion = atoi(buf);
    } else {
        opcion = rand() % cantidad + 1;
        printf("  La maquina elige automaticamente...\n");
        SLEEP(800);
    }

    if (opcion < 1 || opcion > cantidad) opcion = 1;
    opcion--;

    Pokemon* p = crearPokemonDesdeTemplateIndice(opcion, nombreJugador);
    if (!p) return NULL;
    printf(COLOR_VERDE "\n  %s ha elegido a %s %s!\n\n" COLOR_RESET,
           nombreJugador, p->emoji, p->nombre);
    SLEEP(600);
    return p;
}

void configurarPartidaConsola(EstadoJuego* estado) {
    char nombreJ1[MAX_ENTRENADOR];
    char nombreJ2[MAX_ENTRENADOR];

    mostrarMenuPrincipal();

    char buf[10];
    if (!fgets(buf, sizeof(buf), stdin)) buf[0] = '5';
    int opcion = atoi(buf);

    estado->turnoActual = 1;
    estado->historial.cantidad = 0;
    estado->modoRepeticion = 0;
    estado->totalAccionesReplay = 0;
    estado->indiceAccionReplay = 0;
    estado->semillaReplay = 0;
    inicializarCola(&estado->cola);
    resetearRepeticion(estado);

    switch (opcion) {
        case 1:
            estado->jugador1EsHumano = 1;
            estado->jugador2EsHumano = 1;
            estado->modoAutomatico   = 0;

            pedirNombreJugador(nombreJ1, sizeof(nombreJ1), "Jugador 1",
                               "  Nombre del Jugador 1: ");
            pedirNombreJugador(nombreJ2, sizeof(nombreJ2), "Jugador 2",
                               "  Nombre del Jugador 2: ");

            limpiarPantalla();
            mostrarTitulo();
            printf(COLOR_VERDE "  === MODO: 2 JUGADORES ===\n\n" COLOR_RESET);

            printf(COLOR_AMARILLO "  -- JUGADOR 1, elige tu Pokemon --\n" COLOR_RESET);
            encolarJugador(&estado->cola, elegirPokemon(nombreJ1, 1));

            printf(COLOR_AZUL   "  -- JUGADOR 2, elige tu Pokemon --\n" COLOR_RESET);
            encolarJugador(&estado->cola, elegirPokemon(nombreJ2, 1));
            break;

        case 2:
            estado->jugador1EsHumano = 1;
            estado->jugador2EsHumano = 0;
            estado->modoAutomatico   = 0;

            pedirNombreJugador(nombreJ1, sizeof(nombreJ1), "Jugador",
                               "  Tu nombre de entrenador: ");

            limpiarPantalla();
            mostrarTitulo();
            printf(COLOR_VERDE "  === MODO: JUGADOR vs MAQUINA ===\n\n" COLOR_RESET);

            printf(COLOR_AMARILLO "  -- TU, elige tu Pokemon --\n" COLOR_RESET);
            encolarJugador(&estado->cola, elegirPokemon(nombreJ1, 1));

            printf(COLOR_MAGENTA "  -- LA MAQUINA elige su Pokemon --\n" COLOR_RESET);
            encolarJugador(&estado->cola, elegirPokemon("Maquina", 0));
            break;

        case 3:
            estado->jugador1EsHumano = 0;
            estado->jugador2EsHumano = 0;
            estado->modoAutomatico   = 1;

            limpiarPantalla();
            mostrarTitulo();
            printf(COLOR_MAGENTA "  === MODO: MAQUINA vs MAQUINA ===\n\n" COLOR_RESET);
            printf("  La maquina elige ambos Pokemon...\n\n");
            SLEEP(400);

            encolarJugador(&estado->cola, elegirPokemon("IA-Alpha", 0));
            encolarJugador(&estado->cola, elegirPokemon("IA-Beta",  0));
            break;

        case 4: {
            estado->jugador1EsHumano = 0;
            estado->jugador2EsHumano = 0;
            estado->modoAutomatico   = 1;
            limpiarPantalla();
            mostrarTitulo();
            printf("  Cargando repeticion desde ACCIONES.TXT...\n");
            int nAcciones = cargarAccionesDeArchivo(estado, "ACCIONES.TXT");
            if (nAcciones <= 0) {
                printf(COLOR_ROJO
                       "  Error: ACCIONES.TXT no contiene una repeticion valida.\n"
                       "  Debe incluir al menos 2 jugadores y acciones registradas.\n"
                       "  Volviendo al menu...\n" COLOR_RESET);
                SLEEP(1500);
                configurarPartidaConsola(estado);
            } else {
                estado->modoRepeticion = 1;
                if (estado->semillaReplay != 0) estado->semillaPartida = estado->semillaReplay;
                srand(estado->semillaPartida);
                printf(COLOR_VERDE
                       "  Repeticion activada con %d acciones desde ACCIONES.TXT.\n"
                       COLOR_RESET, nAcciones);
                SLEEP(800);
            }
            break;
        }

        default:
            exit(0);
    }
}

void menuPostBatalla(EstadoJuego* estado) {
    mostrarResultado(estado);

    printf("  Que deseas hacer?\n\n");
    printf("  [1] Ver historial de combate\n");
    printf("  [2] Guardar historial en archivo\n");
    printf("  [3] Jugar de nuevo\n");
    printf("  [4] Salir\n");
    printf("\n  Opcion: ");
    fflush(stdout);

    char buf[10];
    if (!fgets(buf, sizeof(buf), stdin)) return;
    int opcion = atoi(buf);

    switch (opcion) {
        case 1:
            mostrarHistorial(&estado->historial);
            printf("  [Presiona ENTER para continuar...]");
            fflush(stdout);
            consumirEntradaHastaNuevaLinea();
            menuPostBatalla(estado);
            break;
        case 2:
            guardarHistorial(&estado->historial, "historial_batalla.txt");
            SLEEP(1000);
            menuPostBatalla(estado);
            break;
        case 3:
            liberarJuego(estado);
            break;
        default:
            break;
    }
}
