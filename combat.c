#include "pokemon_battle.h"

int calcularDanio(Accion* ataque, Pokemon* atacante, Pokemon* defensor) {
    if (!ataque || ataque->poder == 0) return 0;

    double factor = 0.85 + ((rand() % 26) / 100.0);
    double efectividad = 1.0;
    int especial = (strcmp(ataque->tipo, "especial") == 0);

    int danio = (int)(ataque->poder * factor);
    if (especial) {
        danio = (int)(danio * 1.08);
    }

    int reduccion = defensor->defensa / 2;
    if (reduccion > (danio * 6) / 10) reduccion = (danio * 6) / 10;
    danio -= reduccion;

    efectividad = obtenerMultiplicadorTipos(atacante->tipo, defensor->tipo);
    danio = (int)(danio * 0.72 * efectividad);

    if (defensor->salud == defensor->saludMax) {
        int tope = especial ? (defensor->saludMax * 62) / 100
                            : (defensor->saludMax * 52) / 100;
        if (danio > tope) danio = tope;
    }

    if (danio < 1) danio = 1;
    return danio;
}

double obtenerMultiplicadorTipos(const char* tipoAtacante, const char* tipoDefensor) {
    if (!tipoAtacante || !tipoDefensor) return 1.0;

    if (strcmp(tipoAtacante, "Fuego") == 0) {
        if (strcmp(tipoDefensor, "Planta") == 0) return 1.40;
        if (strcmp(tipoDefensor, "Agua") == 0) return 0.78;
    } else if (strcmp(tipoAtacante, "Agua") == 0) {
        if (strcmp(tipoDefensor, "Fuego") == 0) return 1.38;
        if (strcmp(tipoDefensor, "Planta") == 0) return 0.80;
        if (strcmp(tipoDefensor, "Electrico") == 0) return 0.90;
    } else if (strcmp(tipoAtacante, "Planta") == 0) {
        if (strcmp(tipoDefensor, "Agua") == 0) return 1.38;
        if (strcmp(tipoDefensor, "Fuego") == 0) return 0.78;
    } else if (strcmp(tipoAtacante, "Electrico") == 0) {
        if (strcmp(tipoDefensor, "Agua") == 0) return 1.45;
        if (strcmp(tipoDefensor, "Planta") == 0) return 0.80;
    } else if (strcmp(tipoAtacante, "Psiquico") == 0) {
        if (strcmp(tipoDefensor, "Normal") == 0) return 1.20;
        if (strcmp(tipoDefensor, "Psiquico") == 0) return 0.92;
    } else if (strcmp(tipoAtacante, "Normal") == 0) {
        if (strcmp(tipoDefensor, "Psiquico") == 0) return 0.95;
    }

    return 1.0;
}

void ejecutarAccion(Accion* accion, Pokemon* atacante, Pokemon* defensor,
                    HistorialCombate* hist) {
    char buffer[150];
    char atacanteLabel[64];
    char defensorLabel[64];

    etiquetaParticipante(atacante, atacanteLabel, sizeof(atacanteLabel));
    etiquetaParticipante(defensor, defensorLabel, sizeof(defensorLabel));

    int tirada = rand() % 100 + 1;
    if (tirada > accion->precision) {
        snprintf(buffer, sizeof(buffer),
                 "[FALLO] %s uso %s pero fallo! (tirada=%d > %d%%)",
                 atacanteLabel, accion->nombre, tirada, accion->precision);
        agregarHistorial(hist, buffer);
        printf(COLOR_GRIS "  ~~ El ataque %s FALLO! ~~\n" COLOR_RESET,
               accion->nombre);
        return;
    }

    if (strcmp(accion->tipo, "ataque") == 0 ||
        strcmp(accion->tipo, "especial") == 0) {

        int danio = calcularDanio(accion, atacante, defensor);
        double efectividad = obtenerMultiplicadorTipos(atacante->tipo, defensor->tipo);
        defensor->salud -= danio;
        if (defensor->salud < 0) defensor->salud = 0;

        snprintf(buffer, sizeof(buffer),
                 "[ATAQUE] %s uso %s -> %d de dano a %s (vida restante: %d)",
                 atacanteLabel, accion->nombre, danio,
                 defensorLabel, defensor->salud);
        agregarHistorial(hist, buffer);

        if (strcmp(accion->tipo, "especial") == 0) {
            printf(COLOR_MAGENTA "  *** ATAQUE ESPECIAL: %s ***\n" COLOR_RESET,
                   accion->nombre);
        }
        if (efectividad >= 1.20) {
            printf(COLOR_AMARILLO "  Es super efectivo!\n" COLOR_RESET);
        } else if (efectividad <= 0.85) {
            printf(COLOR_GRIS "  No es muy efectivo...\n" COLOR_RESET);
        }
        printf(COLOR_ROJO "  Dano causado: %d puntos!\n" COLOR_RESET, danio);
        printf("  %s -> Vida: %d/%d\n",
             defensorLabel, defensor->salud, defensor->saludMax);

        if (defensor->salud == 0) {
            defensor->estaVivo = 0;
            printf(COLOR_ROJO "\n  *** %s ha sido DERROTADO! ***\n\n" COLOR_RESET,
                     defensorLabel);
            snprintf(buffer, sizeof(buffer), "[KO] %s fue derrotado por %s",
                    defensorLabel, atacanteLabel);
            agregarHistorial(hist, buffer);
        }

    } else if (strcmp(accion->tipo, "defensa") == 0) {
        int efecto = 0;
        if (strcmp(accion->nombre, "Proteccion") == 0) {
            atacante->defensa += 15;
            efecto = 15;
            printf(COLOR_CYAN "  %s se PROTEGE! Defensa +15 (total: %d)\n" COLOR_RESET,
                     atacanteLabel, atacante->defensa);
        } else if (strcmp(accion->nombre, "Sintesis") == 0 ||
                   strcmp(accion->nombre, "Refugio") == 0) {
            efecto = 25 + rand() % 20;
            atacante->salud += efecto;
            if (atacante->salud > atacante->saludMax)
                atacante->salud = atacante->saludMax;
            printf(COLOR_VERDE "  %s se CURA! Recupera %d de vida (total: %d/%d)\n"
                   COLOR_RESET,
                                     atacanteLabel, efecto, atacante->salud, atacante->saludMax);
        } else {
            efecto = 10;
            atacante->defensa += efecto;
            printf(COLOR_CYAN "  %s usa %s! Defensa +%d\n" COLOR_RESET,
                                     atacanteLabel, accion->nombre, efecto);
        }
        snprintf(buffer, sizeof(buffer),
                                 "[DEFENSA] %s uso %s (efecto=%d)", atacanteLabel,
                 accion->nombre, efecto);
        agregarHistorial(hist, buffer);

    } else if (strcmp(accion->tipo, "mover") == 0) {
        printf(COLOR_AMARILLO "  %s realiza un movimiento estrategico!\n" COLOR_RESET,
                             atacanteLabel);
        snprintf(buffer, sizeof(buffer), "[MOVER] %s se reposiciona",
                                 atacanteLabel);
        agregarHistorial(hist, buffer);
    }
}

void turnoJugador(EstadoJuego* estado, Pokemon* atacante, Pokemon* defensor) {
    Accion* elegida = NULL;
    int opcion;
    char atacanteLabel[64];

    etiquetaParticipante(atacante, atacanteLabel, sizeof(atacanteLabel));

    Accion* lista[MAX_ACCIONES];
    int conteo = 0;
    Accion* tmp = atacante->pilaAcciones;
    while (tmp && conteo < MAX_ACCIONES) {
        lista[conteo++] = tmp;
        tmp = tmp->sig;
    }

    if (conteo == 0) {
        printf(COLOR_AMARILLO "  %s no tiene acciones disponibles! Pasa turno.\n"
             COLOR_RESET, atacanteLabel);
        atacante->turnosSinAtacar++;
        return;
    }

    printf(COLOR_AMARILLO "\n  === Turno de %s %s ===\n" COLOR_RESET,
            atacante->emoji, atacanteLabel);
    printf("  Elige una accion:\n\n");

    int i;
    for (i = 0; i < conteo; i++) {
        const char* colorTipo = colorTipoAccion(lista[i]->tipo);

        printf("  %s[%d]%s %-20s | Poder: %3d | Precision: %3d%% | %s%s\n",
               COLOR_AMARILLO, i + 1, COLOR_RESET,
               lista[i]->nombre,
               lista[i]->poder,
               lista[i]->precision,
               colorTipo, lista[i]->descripcion);
        printf(COLOR_RESET);
    }

    printf("\n  Tu eleccion (1-%d): ", conteo);
    fflush(stdout);

    char buf[10];
    if (!fgets(buf, sizeof(buf), stdin)) opcion = 1;
    else opcion = atoi(buf);

    if (opcion < 1 || opcion > conteo) {
        printf(COLOR_GRIS "  Opcion invalida. Se usara el primer ataque disponible.\n"
               COLOR_RESET);
        opcion = 1;
    }

    elegida = extraerAccionPorIndice(atacante, opcion - 1);

    if (!elegida) {
        printf(COLOR_GRIS "  No se pudo obtener la accion. Turno perdido.\n" COLOR_RESET);
        return;
    }

    printf("\n");
    animarAtaque(elegida->nombre, COLOR_AMARILLO);
    ejecutarAccion(elegida, atacante, defensor, &estado->historial);
    registrarAccionEnArchivo(estado, atacante, elegida, "ACCIONES.TXT");
    free(elegida);
}

void turnoIA(EstadoJuego* estado, Pokemon* ia, Pokemon* defensor) {
    char iaLabel[64];
    etiquetaParticipante(ia, iaLabel, sizeof(iaLabel));

    if (!ia->pilaAcciones) {
        printf(COLOR_GRIS "  %s no tiene acciones!\n" COLOR_RESET, iaLabel);
        return;
    }

    SLEEP(800);
    printf(COLOR_MAGENTA "\n  [IA] %s %s esta eligiendo...\n" COLOR_RESET,
           ia->emoji, iaLabel);
    SLEEP(600);

    Accion* elegida      = NULL;
    Accion* mejorAtaque  = NULL;
    Accion* mejorDefensa = NULL;

    Accion* cur = ia->pilaAcciones;
    while (cur) {
        if ((strcmp(cur->tipo, "ataque") == 0 ||
             strcmp(cur->tipo, "especial") == 0) && !mejorAtaque) {
            mejorAtaque = cur;
        }
        if (strcmp(cur->tipo, "defensa") == 0 && !mejorDefensa) {
            mejorDefensa = cur;
        }
        cur = cur->sig;
    }

    double porcentajeVida = (double)ia->salud / ia->saludMax;

    if (porcentajeVida < 0.30 && mejorDefensa) {
        elegida = mejorDefensa;
    } else if (mejorAtaque) {
        elegida = mejorAtaque;
    } else if (mejorDefensa) {
        elegida = mejorDefensa;
    } else {
        elegida = ia->pilaAcciones;
    }

    Accion* temporal[MAX_ACCIONES];
    int tempCount = 0;
    Accion* actual;
    while ((actual = popAccion(ia)) != NULL) {
        if (actual == elegida) break;
        temporal[tempCount++] = actual;
    }
    int j;
    for (j = tempCount - 1; j >= 0; j--) pushAccion(ia, temporal[j]);

    if (!elegida) {
        printf(COLOR_GRIS "  [IA] No pudo seleccionar accion.\n" COLOR_RESET);
        return;
    }

    printf(COLOR_MAGENTA "  [IA] %s usa: %s!\n" COLOR_RESET,
            iaLabel, elegida->nombre);
    SLEEP(400);

    animarAtaque(elegida->nombre, COLOR_MAGENTA);
    ejecutarAccion(elegida, ia, defensor, &estado->historial);
    registrarAccionEnArchivo(estado, ia, elegida, "ACCIONES.TXT");
    free(elegida);
}

void turnoRepeticion(EstadoJuego* estado, Pokemon* atacante, Pokemon* defensor) {
    const char* nombreAccion = NULL;

    if (!atacante->pilaAcciones) {
        printf(COLOR_GRIS "  [REPLAY] %s no tiene acciones.\n" COLOR_RESET,
               atacante->nombre);
        return;
    }

    if (estado->indiceAccionReplay < estado->totalAccionesReplay) {
        RegistroAccionReplay* reg = &estado->accionesReplay[estado->indiceAccionReplay++];
        nombreAccion = reg->accion;
        if (strcmp(reg->atacante, atacante->nombre) != 0) {
            printf(COLOR_GRIS
                   "  [REPLAY] Aviso: se esperaba accion de %s, turno actual %s.\n"
                   COLOR_RESET,
                   reg->atacante, atacante->nombre);
        }
    }

    {
        Accion* elegida = extraerAccionPorNombre(atacante, nombreAccion);
        if (!elegida) {
            printf(COLOR_GRIS "  [REPLAY] No se pudo extraer accion.\n" COLOR_RESET);
            return;
        }

        printf(COLOR_CYAN "  [REPLAY] %s usa: %s\n" COLOR_RESET,
               atacante->nombre, elegida->nombre);
        SLEEP(350);
        animarAtaque(elegida->nombre, COLOR_CYAN);
        ejecutarAccion(elegida, atacante, defensor, &estado->historial);
        free(elegida);
    }
}

int quedanJugadoresVivos(ColaTurnos* cola) {
    if (!cola || !cola->frente) return 0;
    int vivos = 0;
    Pokemon* actual = cola->frente;
    int i;
    for (i = 0; i < cola->total; i++) {
        if (actual->estaVivo) vivos++;
        actual = actual->sig;
    }
    return vivos;
}

Pokemon* buscarEnemigoVivo(ColaTurnos* cola, Pokemon* actual) {
    if (!cola || !cola->frente) return NULL;
    Pokemon* cur = cola->frente;
    int i;
    for (i = 0; i < cola->total; i++) {
        if (cur != actual && cur->estaVivo) return cur;
        cur = cur->sig;
    }
    return NULL;
}

void jugar(EstadoJuego* estado) {
    printf(COLOR_AMARILLO "\n  Presiona ENTER para comenzar la batalla!\n"
           COLOR_RESET);
    fflush(stdout);
    consumirEntradaHastaNuevaLinea();

    while (quedanJugadoresVivos(&estado->cola) > 1) {
        Pokemon* atacante = frenteCola(&estado->cola);

        if (!atacante->estaVivo) {
            avanzarTurno(&estado->cola);
            continue;
        }

        Pokemon* defensor = buscarEnemigoVivo(&estado->cola, atacante);
        if (!defensor) break;

        mostrarEstadoBatalla(estado);

        printf(COLOR_CYAN "\n  Turno #%d\n" COLOR_RESET, estado->turnoActual);

        int esHumano = 0;
        if (estado->cola.total >= 1) {
            if (estado->turnoActual % 2 == 1 && estado->jugador1EsHumano) {
                esHumano = 1;
            } else if (estado->turnoActual % 2 == 0 && estado->jugador2EsHumano) {
                esHumano = 1;
            }
        }

        if (estado->modoRepeticion) {
            turnoRepeticion(estado, atacante, defensor);
        } else if (esHumano) {
            turnoJugador(estado, atacante, defensor);
        } else {
            turnoIA(estado, atacante, defensor);
        }

        if (!defensor->estaVivo) {
            eliminarDeCola(&estado->cola, defensor);
            mostrarEstadoBatalla(estado);
            printf(COLOR_ROJO "\n  %s %s fue eliminado de la batalla!\n"
                   COLOR_RESET, defensor->emoji, defensor->nombre);
            if (!estado->modoRepeticion) {
                SLEEP(1200);
            }
        } else {
            if (!estado->modoRepeticion) {
                SLEEP(estado->modoAutomatico ? 1000 : 400);
            }
        }

        if (estado->modoRepeticion) {
            printf(COLOR_GRIS "\n  [REPLAY] Presiona ENTER para la siguiente accion..." COLOR_RESET);
            fflush(stdout);
            consumirEntradaHastaNuevaLinea();
        } else if (estado->modoAutomatico) {
            printf(COLOR_GRIS "\n  [IA vs IA] Presiona ENTER para la siguiente accion..." COLOR_RESET);
            fflush(stdout);
            consumirEntradaHastaNuevaLinea();
        }

        avanzarTurno(&estado->cola);
        estado->turnoActual++;

        if (!estado->modoAutomatico) {
            printf(COLOR_GRIS "\n  [Presiona ENTER para continuar...]" COLOR_RESET);
            fflush(stdout);
            consumirEntradaHastaNuevaLinea();
        }
    }
}
