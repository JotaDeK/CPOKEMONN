# Pokemon Battle Simulator
## Proyecto de Pilas y Colas - IS304 2025

---

## Compilacion y ejecucion

```bash
# Linux / Mac
make
./pokemon_battle

# Windows (MinGW)
gcc -Wall -Wextra -std=c99 -o pokemon_battle.exe main.c
pokemon_battle.exe
```

---

## Arquitectura del sistema

### Estructuras de datos principales

| Estructura | Tipo | Uso |
|---|---|---|
| `Accion` | Nodo de **pila** | Acciones del jugador (LIFO) |
| `Pokemon` | Entidad con pila | Estado del jugador + acciones |
| `ColaTurnos` | **Cola circular** | Orden de turnos (FIFO circular) |
| `EstadoJuego` | Agregador | Todo el estado de la partida |
| `HistorialCombate` | Arreglo | Registro de eventos |

### Flujo de ejecucion

```
main()
  └── configurarPartidaConsola()   <- elige modo y pokemon
        └── encolarJugador()       <- COLA CIRCULAR: agrega jugadores
              └── cargarAccionesDefault() <- PILA: inserta acciones

  └── jugar()                      <- bucle principal
        ├── frenteCola()           <- quien ataca ahora
        ├── turnoJugador() / turnoIA()
        │     └── popAccion()      <- PILA: extrae accion del tope
        │     └── ejecutarAccion() <- aplica daño/defensa
        ├── eliminarDeCola()       <- COLA: saca al derrotado
        └── avanzarTurno()         <- COLA: rota al siguiente
```

---

## Modos de juego

1. **Jugador 1 vs Jugador 2** — dos personas en la misma consola
2. **Jugador vs Maquina** — tu vs la IA
3. **Maquina vs Maquina** — modo espectador
4. **Cargar desde archivo** — lee `JUGADORES.TXT`

---

## Archivos de entrada

### JUGADORES.TXT
```
NombrePokemon   Tipo       Emoji  Salud  Defensa  Velocidad
Charizard       Fuego      [C]    110    12       85
Blastoise       Agua       [B]    105    18       78
Pikachu         Electrico  [P]    90     10       95
```

**Tipos soportados:** `Fuego`, `Agua`, `Planta`, `Electrico`, `Psiquico`, `Normal`

---

## Escalabilidad (proximas versiones)

- [ ] Reemplazar `turnoJugador()` con llamadas desde GUI (sin tocar logica)
- [ ] Reemplazar `mostrarBarraVida()` con sprites graficos
- [ ] Reemplazar `animarAtaque()` con animaciones de sprites
- [ ] Reemplazar `cargarAccionesDefault()` con lectura de `ACCIONES.TXT`
- [ ] Agregar sonido en `ejecutarAccion()`
- [ ] Exportar `HistorialCombate` a JSON para replay

---

## Respuestas a preguntas teoricas

1. **¿Ventajas de pila para acciones?**
   La pila (LIFO) permite que el jugador tenga un "flujo natural" de acciones:
   la ultima que preparo es la primera que ejecuta, similar a como un jugador
   real planea su siguiente movimiento. O(1) para push/pop.

2. **¿Por que cola circular para turnos?**
   La cola circular garantiza equidad (cada jugador espera el mismo numero
   de turnos) y permite avanzar indefinidamente sin reconstruir la estructura.
   Cuando un jugador es eliminado se remueve en O(n) y el ciclo continua.

3. **¿Como evitar perdida de memoria?**
   Cada `malloc` tiene su `free` correspondiente. `liberarJuego()` libera
   todos los nodos de pila de cada pokemon y luego cada pokemon mismo.
   La circularidad se rompe antes de liberar para evitar doble-free.

4. **¿Que validaciones necesita un simulador de turnos?**
   - Pokemon sin acciones (turno perdido)
   - Pokemon con vida <= 0 (eliminar de cola)
   - Precision del ataque (tirada de dados)
   - Entrada del usuario fuera de rango
   - Archivos inexistentes o mal formateados

5. **¿Que otras estructuras mejorarían el juego?**
   - **Tabla hash** para buscar acciones por nombre en O(1)
   - **Arbol de decisiones** para IA mas inteligente
   - **Grafo** para mapas con movimiento
   - **Heap** para priorizar turnos por velocidad

---

Elaborado conforme a lineamientos IS304 - 2025
