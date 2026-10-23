# Cola vigente (21/10/2026, reparto con Codex)

Única cola de trabajo; los demás .md son evidencia. Lo terminado sale de aquí (queda en git y en CHANGELOG.md).

Reglas del usuario:
- Claude y Codex commitean y pushean a main; si uno lleva más de 10 min sin commits ni archivos tocados, el otro toma
  sus puntos anotando hora y evidencia aquí.
- Todo lo visual se entrega como copia jugable + .bat y espera aprobación del usuario; lo aprobado va a
  LOGICA_APROBADOS.md; método en PRUEBAS_VISUALES.md.
- La validación en hardware real no es pendiente: llega como reporte.
- Nunca borrar carpetas ReKHCoM-vX sin que el usuario lo pida.
- Publicar (releases, comentarios en issues) solo con el OK del usuario.

## Estado (21/10 ~19:15)

- **v1.9.14 publicada (estable, Latest)**, commit a97268171 (main): PC (Windows, Linux, Steam Deck) estable; Android,
  Switch, Vita, PS4, PS5 (PPSA98511.zip, etiqueta ps5-beta) y Anbernic como `-beta`. Incluye J, #37, #38, I (PPU en
  varios núcleos, 90 eventos sin MISMATCH), mod HD (beta). Android 64 con imagen movida: 37/37 ok. Comentado en #22,
  #25, #33-#38; #25 cerrado. Los demás se cierran cuando el reporter confirme. Releases v1.9.12 y v1.9.13-pre2: borrar
  (el clasificador bloqueó `gh release delete`; lo hace el usuario). Escenas Preliminar V1 de Codex sin aprobar: NO
  integradas (rama claude/integrate-20261021).


- **Publicado:** v1.9.12 estable (Latest), todas las plataformas + alphas de Switch y Vita; web en Pages.
  v1.9.13-pre2 de prueba (Windows, APK 32/64, Switch GPU) comentada en #22, #33, #34, #35.
- **Rama `claude/perf-android-reloc`** (no está en main; va a main cuando pasen sus dos baterías):
  - J, Android 64 en teléfonos donde 0x01000000 está ocupada (#22 Poco F8 Ultra, #34 Mate XT): la imagen se carga en
    otro hueco y se reubica. En el emulador con `KHCOM_IMAGE_MOVE=1` carga en 0xe2600000 y los menús salen bien.
  - Caja de error con "Share log" (modelo, Android, logs). Si el juego se cae, `kh_com_host.log` trae la señal y la
    cadena de llamadas del juego (direcciones del enlace: `llvm-symbolizer --obj=guest-<región>.macho`).
  - #37 (sin teclado no se salía de "Press..."): un botón del mando, un toque o Atrás cancelan la captura.
  - #38: "ocultar controles táctiles" con un mando conectado no dibuja nada, ni en menús (sin mando queda el botón
    de opciones). Solo Android.
  - I, PPU en varios núcleos (Windows): falta la verificación.
- **21/10 ~17:00, pausadas por espacio:** el emulador reservaba 9 GB y el pagefile creció a 35,7 GB (C: llegó a
  0,3 GB). Android (menús y fixtures con imagen movida: todo ok salvo el selector de eventos, ya arreglado) y la PPU
  (80/194, 0 `MISMATCH`) se retoman con `--resume` cuando haya RAM/disco; la de escritorio sigue. Reiniciar Windows
  devuelve el pagefile. Arreglados después: Event_0 (selector de eventos) y los dos sitios del Bosque de los 100 Acres
  (`BUS_PTR`, `port/src/port_bus.c`), vistos en el informe de escritorio (`scenes` ahora va primero en las baterías).
- **Baterías (estado anterior):**
  - Android, imagen movida, EU menús/fixtures/jefes: `build/android-move-battery.log`,
    `build/android-scenarios/report-eu-khcom_image_move.json`. Relanzar: el mismo comando con `--resume`
    (`build/move_cmd.txt`).
  - PPU: `KHCOM_PPU_VERIFY=1`, eventos US en vista 16:9 (`--wide`; en vista GBA no hay filas fuera de la imagen y la
    verificación no prueba nada): `build/ppuverify-us.log`, `build/ppuverify/us_wide/*/kh_com.log` sin `MISMATCH`.
  - Escritorio (`run_batteries.py --status`): eventos EU hechos, US en curso; luego JP, 16:9 y salas.
- **Protecciones nuevas (21/10):** `check_sources.py` (constantes SDL del shim de consolas con el valor de SDL,
  literales de Java cerrados) corre en `build_android.py` y en `release_check.py`; ya encontró `SDLK_DELETE` mal en el
  shim. El arnés Android ya no da "ok" a un caso que todavía no arrancó, cuenta como caída el `crash:` del host y marca
  `BAD-APK` si el APK de prueba se armó sin `--smoke-env KHCOM_FAST=1` (corría la batalla de prueba en cada caso).

## Estado 22/10 ~15:00 (Claude, sesión principal; usuario: "sigue con lo que queda y haz un debuggeo grande")
- **Hecho y en main hoy:** oclusión de piezas HD en la imagen y en la capa de pantalla (`MainHdOver`, `HdOverLayer`);
  cartas HD en el menú de barajas y la vista Original; **22 cartas de mapa HD** (también en la síntesis de estancias);
  **13 mundos HD** (`cl_###` recortados con la silueta GBA, grupo 4); borde 1 del GBA clásico (elegido por el usuario)
  con 6 cartas de mapa reales leídas de la ROM; dígitos de cartas = los de Re:CoM (la fuente de diálogos en cartas fue
  rechazada: solo HUD y diálogos). 190 piezas HD; el importador del launcher = Python (dif. máx. 1).
- **Arte HD 1:1 con GBA: ~95 %.** Falta: proponer al usuario los tentativos (medidores, contador, recarga) con capturas.
- **Depuración grande en curso:** batería de escritorio con HD encendido (`run_batteries.py --hd --detach`,
  `build/batteries-hd.log`, informe `build/debugkit-hd/report.md`); la de HD apagado terminó limpia (294 casos, 0
  caídas; resultados en `build/batt-hdoff-20261022`). Ganchos nuevos: `KHCOM_CONFIG="k=v;k=v"`, `KHCOM_HD_SPRITES`,
  `KHCOM_MODE=worlds:MASK`. Luego: Android 64 con HD e imagen movida; consolas contra el shim.
- Comprobado 22/10 ~15:00: cartas de mapa, mundos, combate y borde 1 con HD en EU/US/JP (vista Original y Auto) y en
  el build reubicable (`configure_pc.py --reloc`, como Switch/Vita): 0 fallos; `check_sources.py` ok (shim).
- **Usuario 22/10 ~15:30:** le gustan el anillo de vida de Sora (Re:CoM `pl_gauge`) y las cartas; pidió mejorar la carta
  de recarga → **hecho**: `com_card02` (carta negra con borde plateado, 1:1) y su contador 1..19 con los dígitos blancos de
  `card_no` (`gReloadCounterWhite`, 19 composiciones). El "píxel" junto a la guarda es la cadena del llavero del arte HD.
  **Anillo de vida: primera versión en main** (`PortHdHpRing`, `pl_gauge` sin la cara horneada, pista vacía gris,
  se vacía desde la punta de la barra y luego el anillo; la cara HD se corre a (14,15) para que entre). La lógica de
  Re:CoM vive en su ejecutable (el .RTM solo trae texturas: `UK_gauge.imz` tiene las piezas: cuartos de anillo gris/
  verde/negro, barra, dígitos, HP, MAX, D-POINT). **Usuario: igualar la lógica real del HUD de Sora de Re:CoM**
  (es compleja): buscar una implementación (mods, OpenKh, ingeniería inversa) y reimplementarla.
- **HUD de combate como el 1.5 PC (usuario 22/10 ~16:00):** dígitos de cartas de `Cockpit.imz` (los de Re:CoM, no
  `card_no`), contador CARDS de mazo restante, carta de recarga con su número, anillo con la lógica real. Hallazgos y
  plan en HUD_HD_RECOM.md ("HUD de combate de Re:CoM 1.5 PC"). Ajustar tamaños/posiciones de todo lo HD al original.
- **Decompilación positiva (22/10 ~16:40):** CPlayerGauge leído (HUD_HD_RECOM.md "Decompilado"). Sigue:
  `port_hdlayout.c` (lector/dibujante LAYD/SEQD en C, como el SequenceRenderer de OpenKh) y el medidor de vida de 1.5 con
  esa lógica; luego CCardRevolver (pila, contador CARDS, recarga). Prueba de juego del anillo actual: daño, cura y
  >280 ok, 0 fallos; audio: test_hd_audio 0/9, test_hd_audio_flow 7/8 (el título se cortó por tiempo: revisar).
- **22/10 ~16:30 (Claude que retomó):** medidor de vida de Re:CoM 1.5 en main (977f064cf, `port_hdlayout.c`). Inventario de los menús de Re:CoM (268 layouts, texturas en español, 28 clases decompiladas): HUD_HD_RECOM.md "Menús de Re:CoM". Sigue: un menú por vez con su lógica decompilada (orden propuesto: pausa, subida de nivel, naipe conseguido, guardar/cargar, elegir mundo, síntesis, campamento, tienda, diario, minimapa).
- **Issues abiertos (22/10 21:00, usuario: leerlos todos y replicar hasta arreglarlos):** #46 Android se cierra al
  ganar el primer combate del evento de Agrabah (US 124 ya se cortaba por tiempo en la batería: era este bug); #44
  efectos/voces mudos con la música HD (v1.9.15-pre1); #45 voces undub que suenan en inglés (ataque en el campo, 2º
  golpe aéreo) [**arreglado**: el mapa de Undub no tenía el primer instrumento del grupo de voces (SONG_VO_SR_ATTACK00
  seguía con la muestra inglesa); `check_undub.py` lo halla, `test_undub.py` compara las 83 voces EU/US con el JP: 83/83] [#44: **arreglado en main** (31cbfd92d, posterior a la pre1 del reporter); `test_hd_audio.py` ahora
  toca la música HD de verdad (`KHCOM_HD_MUSIC`; antes corría sin ella y pasaba sin probar nada) y suma punto de
  guardado, ataque en el campo y voces undub: 0/13]; #42 finales HD: solo créditos [**arreglado en main** (38ee801ce): con "videos HD" el importador copia también zz_event_151/621/631 y el juego los toca (IsEnding en port_hdmovie.c) aunque `hd_scenes` esté apagado; `test_hd_scenes.py --movies-only` 4/4, mod completo 97/97; respondido en el issue (reimportar videos)]; #43 Switch: colores en cartas superpuestas [las fotos muestran PALETAS mal (contornos rojos en Sora y los soldados, colores cambiados en Sora), no teselas; en PC la misma batalla (evento 095, soldados de cartas) se ve bien, sin paletas agotadas (registro temporal en LoadObjPalette: 0 fallos) y el salto de cuadros (KHCOM_SKIPTEST en combate) solo cambia las filas 0-10 durante el mosaico; el reporter usa la build de prueba v1.9.15-pre1 de #36 (sin mod HD). 23/10: con el salto de cuadros apagado se arregla (pero el audio va mal): es del salto. En PC, combate 095 con KHCOM_SKIPTEST, el único efecto era el mosaico de rotura de carta: las filas de arriba de la vista ancha (gy<0) empiezan a mitad de bloque y tomaban la referencia afín del último cuadro DIBUJADO; arreglado (MosaicBlockStart, port_ppu.c): 0/194 cuadros distintos con y sin salto. Los contornos rojos de las fotos no salen en PC ni en reubicable: si siguen, falta una build de prueba de Switch (pedir OK al usuario)]. Pedidos datos de equipo en
  #46, #42, #43. #40 arreglado (avisar al publicar). #46 (23/10): las cadenas evento→combate dieron 0 bios/crash; los fallos de "no llega" eran del arnés: bosskill golpeaba piezas intocables (Guard Armor: torso antes que las extremidades, enemyCount daba la vuelta a 250) y dejaba un knockbackLift viejo (un Bandido volaba miles de cuadros); arreglado en main.c (bosskill) + modo btllog. En Android los casos largos eran inválidos: el getenv del invitado (port/ps5/guest/src/libc.c, Android 64/PS5/Switch/Vita) cortaba cada variable a 159 bytes, así que KHCOM_MODE/KHCOM_KEYS perdían las pulsaciones y el evento se quedaba parado; ahora la ranura crece. Con eso, emulador Android UBSan US y EU: evento 107 → combate → 108 → campo → dos combates más hasta el cierre programado, sin crash; Jafar (113) ok. No se reproduce: falta el modelo/versión del reporter. Guard Armor (008) con bosskill sigue sin cerrar: la derrota espera BgFxIsActive() (chara.c, CharaObjUpdateDefeat) y un efecto de muerte de pieza no termina; investigar si es del arnés (todas las extremidades a la vez) o del juego.
- **Pendientes HD (22/10 noche, Claude; se van tachando):**
  - [x] TIENDA en alemán e italiano (22/10 20:35): MOGRY-LADEN y NEGOZIO MOGURI en HD (`--language de/it`, registro
    `KHCOM_HDDRAWLOG`).
  - [x] Icono de Monstruo en US/JP: fila propia `gWorldIconMonstroHDUs` (mode_worldinspect: 12 iconos HD, nada fuera
    de la caja).
  - [x] `check_hd_sprites.py` (nombres únicos, símbolos por versión, piezas dentro de la caja, tiles que el código
    nombra) corre en `check_sources.py`: 300 filas, 0 problemas.
  - [x] Registro `KHCOM_HDDRAWLOG` y batería `test_hd_menus.py`: **68/68 ok** en EU (22/10 21:00): cada menú
    en 5 idiomas + medidor a 3..100 %, comprobando que el HD sigue en el cuadro quieto (no solo al entrar).
  - [ ] Contador de mazo (`gRevCount*`) y medidor del enemigo como Re:CoM: buscar capturas del HUD de combate de
    Re:CoM en internet y reconstruirlos con Cockpit/gauge.lad y el código decompilado (CCardRevolver, CEnemyGauge).
  - [ ] Revisar en el juego BONUS ÉLITE / PREMIUM BONUS y VER BARAJA (Riku): filas hechas, no vistas.
  - [ ] US y JP: correr la batería de menús también en esas versiones (solo se vio EU en 5 idiomas).
  - [x] Gemelo Python del dibujante: `port/tools/khhd_seqdraw.py`.
  - [ ] Batería HD de escritorio (`run_batteries.py --hd`): corría con el build anterior; repetirla con los cambios
    de hoy al terminar.
  - Sin equivalente en Re:CoM (se quedan GBA salvo decisión del usuario): TRANSFERT, BOUTIQUE MOG, REISEBERICHT y
    los informes de Riku del diario, entradas de la pantalla de título (faltan letras), iconos de bonus del nivel.
- **22/10 noche (Claude):** en main: títulos de menú HD en 5 idiomas (69 carteles), VER BARAJAS quieto
  (`PortHdStillSprite`), iconos de los 13 mundos (world_icon), **medidor de vida corregido** (a1be11b8c: bucle, barra,
  máscaras, layouts remasterizados). Todo en HUD_HD_RECOM.md. Sigue (necesita referencia visual de Re:CoM o decisión
  del usuario): contador de mazo a lo "CARDS", medidor del enemigo, entradas del título (faltan letras), "NUEVO" →
  "¡NUEVO!". La batería HD (`run_batteries.py --hd`) seguía corriendo a las 18:25 con el build anterior.
- Arreglado: las muescas de la corona dejaban ver la ilustración HD (se borra su franja superior, tapada por el marco).
- Propuesta original: `pruebas/HD tentativos (medidores contador recarga).png` (medidor de vida en anillo de
  Re:CoM, medidor del enemigo, contador CARD/S, números de recarga): no son 1:1, espera su decisión.
- **Espera al usuario:** Darkside (arenas), OK para builds v2.0.
- **23/10 madrugada (Claude):** v2.0-pre1 local para el usuario en la raíz (`ReKHCoM-v2.0-pre1`, `Probar ReKHCoM v2.0
  pre1.bat`): paquete de `package_release.py` (build/pc, pc-us, pc-jp reconfigurados), limpia (sin partidas ni ini),
  ROMs EU/US/JP y HD importado de cero con khhd_import.py --scenes + khhd_sprites.py (103 videos, música, 300 sprites,
  subtítulos). Baterías v2.0 desde cero (`run_batteries.py --detach`, build/batteries.log; build/pc-eu estaba
  configurado sin los .c nuevos y no enlazaba: reconfigurado). Después: `run_batteries.py --hd --detach`.
  Ojo: `python` en el PATH ahora es el venv de hermes (sin numpy): usar `py -3`.
- **23/10 ~03:30 (Claude):** carpetas del HD ordenadas (262fdb5c9): `hd/movies` (ROP, RSE, RRE), `hd/scenes`
  (todas las zz_event, también las de los finales), `hd/music`, `hd/subs`, `hd/sprites`. El launcher de PC mueve
  las escenas de una importación vieja (renombrar), el de Android copia también `scenes`, el juego busca primero en
  `hd/scenes` y si no en `hd/movies`. test_hd_scenes: nuevo 97/97 (Re:CoM 97/97, finales 3/3), viejo 97/97
  (`hd-completo` queda con el orden viejo a propósito: prueba el respaldo). La pre1 de la raíz ya tiene el orden
  nuevo y el programa con este cambio. Espacio: borradas pruebas viejas (eventtest, batt-hdoff, ppuverify, mx_*,
  las de Codex del 18/10): C: de 13,9 a 25,3 GB libres; se guardan codex-evidence y preliminar-v1 (revisión de arte).
  Baterías v2.0 corriendo (scenes EU/US/JP ok; sigue events).
- **Comentarios del usuario sobre la v2.0-pre1 (23/10, en orden; se van tachando):**
  - [x] **Bug (arreglado ac3e2c4f1 + hermanos):** abrir Opciones en la transición de una escena HD al juego y volver dejó la pantalla negra con la
    pausa de escena abierta e invisible (memoria del proceso: modo Event, sScenePause=1, sOpen=0, sReturn=0); START la
    cerró. El evento abre la pausa al soltar un toque corto de START (event_message.c UpdateEventSeq).
  - [x] Logo HD del título con líneas finas (arreglado y visto a 1080p): bordes de piezas en medio píxel + la inclinación de 0,01 px
    (port_hdsprites.c Quad); arreglo hecho (esquinas a píxel entero), falta verificar en el título estático.
  - [x] (3de66108c) "Elige un mundo": la imagen grande del mundo se ve mal (máscara del sprite GBA, bordes pixelados); en
    "Naipes mundo" los iconos HD se ven bien: usar el mismo arte/transparencia, o volver al pixel si no se puede.
  - [ ] "Naipes mapa": la carta grande ya es HD; las chicas de la grilla siguen en pixel: hacerlas HD.
  - [ ] "Estado": las cartas de aliados (Donald, Goofy) siguen en pixel: hacerlas HD.
  - [x] (3de66108c, mismas filas) Diario (Historia): la imagen del mundo es la misma versión mala que "Elige un mundo" (mismo arreglo).
  - [ ] Opcional: mejorar los fondos de esos menús (p. ej. "Naipes mundo").
  - [ ] Diario > Historia: el submenú no tiene las mejoras de vista ancha de los demás (el usuario marcó el margen
    izquierdo, el borde derecho y la barra de arriba a la derecha que se corta): dejarlo como los otros submenús.
  - [x] (3de66108c) Imagen de mundo HD (Elige un mundo, Diario): reemplazar las feas por el arte de "Naipes mundo" que se ve bien.
  - [ ] Anillo de vida HD: bordes lisos (antialias) en la cara y el final de la barra.
  - [ ] Números de las cartas HD quizá muy chicos: comparar con Re:CoM (referencias/capturas del HD).
  - [ ] Cara HD de Sora al recibir daño: el pelo sale cortado.
  - [ ] Editar baraja (Quitar/Añadir naipes): al cambiar con L/R los textos HD aparecen un cuadro antes del fundido;
    el número de la carta grande parpadea al cambiar de carta; el título "QUITAR NAIPES" en HD como los demás.
  - [ ] Pequeños cortes de audio al cambiar de menú: la música HD sin fundido de entrada/salida como el juego;
    revisar la lógica general del mod HD y depurar.
  - [ ] Importar HD con calidad a elegir (herramientas libres, "voltron" de repos): Fuente (tal cual), Buena
    (comprimido con la menor pérdida posible), Optimizada para Android / ahorro (resolución y bitrate de pantalla
    chica), para que el mod corra también en Android. La compresión va integrada en el importador (el jugador no instala nada).
  - [ ] Ripear más assets aprendiendo de otros mods (MelonMIX HD y similares, "voltron" de repos): qué assets de
    Re:CoM sirven y cómo los usan.
  - [ ] v2.0: pasada de rendimiento para todas las consolas, solo lo medible y sin riesgo (medir antes/después,
    pruebas píxel a píxel).
  - [ ] Instalador: barra de progreso al importar/comprimir el HD; instalar en PC en `X:/Games/ReKHCoM` (opcional)
    con todo lo necesario; ofrecer "Añadir a Steam" (instalador y launcher).
  - [ ] Online (estudio preliminar + alpha): llevar el modo cable link (espera latencia cero) a online moderno
    manteniendo su lógica; rollback si aplica; "voltron" de lógica de juegos online y de decomps que añadieron online
    (y cómo, p. ej. con Steam / AppID 480 "Spacewar"); reconstruir en alpha la lógica del multijugador de KHCoM.
  - Todo esto antes de publicar la v2.0 (usuario 23/10 ~05:00, se fue a dormir: seguir sin pedir decisiones).
  - [ ] Barra de vida de los enemigos como Re:CoM (CEnemyGauge decompilado, con su lógica como la de Sora).
  - [ ] **Bug:** un cuadro negro en medio del primer combate de Agrabah con HUD HD (el usuario dejará savestate);
    el HD debe ir igual de fluido que sin HD. Descartado en PC (23/10): combate de Agrabah con cartas/HUD/texto HD, 2451 cuadros normales y 817 con salto
    forzado, ninguna caída aislada de brillo; los savestates del usuario son del evento 000 (slot1) y de editar
    baraja (slot2), no del combate. Falta un savestate del combate o probar con Direct3D 11 real.
  - [ ] Sonido al moverse en la elección HD/GBA del primer arranque (como en Opciones).
  - [ ] Ajustes: "Intros HD" (apertura, créditos, finales; encendido por defecto salvo modo GBA Original),
    "Escenas HD", "HUD HD" (logo del título + cartas + HUD de combate en un solo interruptor).
  - [ ] Nueva partida: preguntar "Escenas HD" y "HUD HD" (con una línea que explique qué es el HUD).
- **Vita (reporte dentro de #44):** "bastante lenta"; sin undub aunque estén las ROM US y JP en la carpeta; texto HD
  no seleccionable. Ninguna versión de Vita reportada a velocidad completa.
- **Switch rendimiento (#35, deko3d):** el host ya presenta por GPU; lo caro es la PPU en CPU y en un solo núcleo
  (PORT_PPU_THREADS es solo de escritorio; el invitado no tiene hilos ni TLS). Opciones: hilos en el invitado (host
  libnx + shim) o la PPU en shaders (deko3d/SDL GPU): proyecto grande, sin hardware para medir.

## v2.0 — próxima estable (pedido del usuario 22/10: "stable nueva con 2.0 y anunciar la inclusión del HD")

Es un lanzamiento grande: no publicar hasta pasar TODA la lista de verificación de abajo. PC (Windows, Linux, Steam Deck)
estable; Android, Switch, Vita, PS4, PS5, Anbernic, web como `-beta`. Al final: revisar issues, arreglar sus bugs,
publicar y anunciar el mod HD en las notas. El usuario prueba con `Probar ReKHCoM v1.9.15 completo.bat` (raíz).

Hecho desde la v1.9.14 (en main): efectos que se silenciaban con la música HD (canales del BGM por su `trackCount`),
música HD al volumen del cartucho, carga en hilo aparte, pausa en el lugar (sin tirar la cola), sigue sonando bajo
Opciones, silencio mientras carga; pausa de escena en videos HD (START/Esc; A ya no corta); 97/100 escenas HD
asignadas (`match_hd_scenes.py`); opciones HD en Mejoras de Re:CoM y en el asistente de partida nueva; inicio de
combate sin el cuadro del corazón chico; arte del título difuminado; videos HD con el renderer por software; barras de
vida sin división por cero (#40); salto de cuadros (encendido en todo); Switch #36; Vita en GPU; Discord en Linux.

### Estado 22/10 ~03:00 (traspaso: al Claude anterior se le acabaron los tokens)
- En main: todo lo de arriba + escenas HD 97/100 (`kHdSceneExtras`: 001/614/631 solo en modo Re:CoM), fundidos de
  0,3 s en videos HD (PC y Android), guardas de división (#40 y parientes), flujo de audio HD arreglado.
- Método de depuración y herramientas: **DEBUG_WORKFLOW.md**. Seguirlo.
- Falta: (1) ~~versión v2.0~~ hecho 22/10 (launcher, rc, web, CHANGELOG; tag **v2.0**: el actualizador de la PS5 lee
  "2.0" como 2.0.0 y `build_all.py`/`build_android.py` ya aceptan vX.Y); (2) builds de todas las plataformas desde el commit final;
  (3) la lista de verificación de abajo completa; (4) revisar issues (#40 abierto: arreglado, avisar al publicar);
  (5) publicar v2.0 anunciando el mod HD, borrar releases viejos si el clasificador deja (si no, el usuario).
- Copia del usuario: `Probar ReKHCoM v1.9.15 completo.bat` (actualizarla con cada build de Windows).

### Estado 22/10 mañana (Claude, sesión principal; el usuario durmiendo, /goal: pendientes + v2.0)
Hecho y en main (todo con la opción apagada = imagen idéntica a antes):
- **Cartas HD** (`hd_cards`): 97 ilustraciones, marcos rojo/verde/azul/negro y dígitos de Re:CoM; mano, enemigo, giro,
  jefes. `port_hdsprites.c` + `PortOamSetHdPart` (engine.c) + receta `port/data/hd_sprites.tsv`.
- **HUD de combate HD** (con `hd_cards`): retratos de Sora y Riku; CORTANAIPE/BLOQUEO/RECUPERA/¡NIVEL↑! en los 5
  idiomas EU (Re:CoM trae la misma traducción que el cartucho). JP no (texto japonés).
- **Título HD** (`hd_title`): logo de Re:CoM, la cinta "Chain of Memories" sin "Re:" (pedido del usuario).
- Importador: `khhd_sprites.py` y el launcher (`--hd-import <juego> --sprites-only`), idénticos; Android copia
  `hd/sprites`. Catálogo de sprites del juego: `KHCOM_HDSPRITE_EXPORT` + `hdsprite_catalog.py`.
- **Modos gráficos**: al primer arranque (graphics_mode=0) el título pregunta Modo HD / Modo GBA → GBA mejorado /
  GBA clásico; launcher "Modo gráfico" (Escoger/HD/GBA mejorado/GBA clásico); Opciones "Restablecer según el modo"
  (confirmar dos veces), filas Cartas HD y Título HD.
- **GBA clásico**: vista Original con borde estilo Super Game Boy (`gba_border` 1..3, 384x216, x5 a 1080p). Las 3
  propuestas: `pruebas/Bordes GBA clasico (elegir 1, 2 o 3).png` → **espera que el usuario elija** (por defecto 1).
  Partida nueva en clásico ofrece "Vista amplia" (el fondo muestra el ancho).
- Bug arreglado: el renderer por software de SDL colgaba con quads alineados espejados/girados/fuera del lienzo.
**Espera al usuario:** Darkside (sesión darkside-arte, commit f14676420 en claude/arenas, sin merge; copia de prueba
en pruebas) y elegir borde 1/2/3.
**Sigue (Claude):** depurar HD en todos los idiomas y plataformas (Android 64, consolas por el puente), intro del
título (logo como fondo afín, sigue GBA), revisar issues, lista de verificación y builds v2.0 (con OK de Darkside).

### TRASPASO 22/10 ~06:30 (al Claude principal se le acaban los tokens; el usuario se fue a dormir)
- Orden: (1) Cartas HD: seguir HUD_HD_RECOM.md "Traspaso" pasos 1-7; (2) cuando Darkside esté aprobado (sesión
  arenas-jefes, [local_d3b029d7]) y Cartas HD en beta: pedir OK al usuario y hacer los builds de la v2.0 con la lista
  de verificación de abajo; (3) modos GBA/HD y lo demás de esta sección; (4) datos de Re:CoM para el modo Re:CoM.
- Repetición sola del combate Android US 124 quedó esperando a que el emulador se libere: `build/android-124-alone.log`.
- Baterías de escritorio restantes (battles/riku/monkey/skip) siguen en `run_batteries.py`; `build/pc-*` ya es HEAD.

### Estado 22/10 ~05:30 (Claude, sesión principal)
- Hecho y en main: puntos 1-4 de abajo, arte x1 + Agrabah encendida, versión v2.0, suelo isométrico rápido, presentación
  sin copia. Comprobado en el build v2.0: combate EU/US/JP limpio, `test_hd_audio.py` 0/9, `match_hd_scenes --check`.
- **Espera al usuario:** (a) su OK para gastar recursos en los builds de la v2.0 (lo pidió el 22/10: "preguntarme antes
  de empezar a usar recursos en la 2.0"); (b) aprobar las arenas de jefes que entrega la sesión "arenas-jefes"
  (Darkside primero; 7 de 8 pendientes); (c) permiso para borrar scratchpads viejos de %TEMP%\claude (~5,4 GB).
- En curso: repetir solo el combate Android US 124 (timeout con dos corridas en el emulador): `build/android-124-alone.log`.
- Después de la v2.0: las 160 filas de la imagen GBA en varios núcleos (RENDIMIENTO.md) para 60 fps a x1.
- Disco: C: ~5 GB libres por `pagefile.sys` 38,7 GB (falta de RAM: emulador + VM + baterías); se achica al reiniciar.

### Nuevo 22/10 (usuario): HUD HD y cartas HD desde el Re:CoM del 1.5+2.5 del jugador
- La v2.0 sale cuando el arte de Darkside esté aprobado (sesión arenas-jefes); mientras, esto.
- Extraer de la instalación del jugador (D:/SteamLibrary/steamapps/common/KINGDOM HEARTS -HD 1.5+2.5 ReMIX-) los
  gráficos del HUD de Re:CoM que coincidan con los de GBA, para el HUD HD (`hd_hud`, hoy piloto con piezas hechas a
  mano/IA), y las **cartas** en una opción aparte nueva: **Cartas HD**. Nada se distribuye: lo importa el launcher,
  como los videos y la música.
- Investigar documentación (OpenKh, decomp/ingeniería inversa de Re:CoM) de los formatos de textura y de la lógica.
- Inventario de texto del HUD que aún no es HD (números y letras) que pueda pasar a HD sin tocar mucho el HUD.
- Estado 22/10: texturas HD halladas (226 cartas, marcos, dígitos, fuentes); 1:1 = cartas de combate y del menú de
  barajas → **Cartas HD** primero. Diseño y estado en HUD_HD_RECOM.md. La v2.0 espera a esto como beta.
- También (usuario 22/10), si los gráficos de Re:CoM lo permiten: **pantalla de título y transiciones en HD**, y
  nuestro **menú de Opciones en HD**.
- **Primer arranque (juego) y launcher: elegir "Modo GBA" o "Modo HD"**, cada uno con su descripción. El Modo GBA
  pregunta después **"GBA mejorado"** (nuestro estándar actual: vista amplia, mejoras) o **"GBA clásico"** (como en
  un emulador: la imagen 240x160 al mayor entero que entre en la ventana, y el resto con un **borde decorativo
  propio estilo Super Game Boy**; el borde es arte nuevo: se entrega con antes/después para aprobar). Se puede
  cambiar luego en el launcher y en Opciones.
  Modo HD = todos los assets HD y todas las mejoras encendidas. GBA mejorado = las mejoras del port como hoy, con
  los extras HD (texto HD, etc.) apagados por defecto. Al empezar partida nueva se sigue preguntando y la elección
  se guarda por partida (como el asistente de partida nueva actual).
  **GBA clásico (usuario 22/10): todo apagado (vista amplia, texto HD, QoL): casi un emulador, pero nativo.** Aun
  así, al empezar partida nueva pregunta si activar lo que aplique (QoL, vista amplia); al mover el interruptor de
  la vista amplia el fondo de esa pantalla cambia a vista amplia para que se note el cambio.
  Decisiones por defecto mientras el usuario no diga otra cosa: Modo HD sin importación = ofrece importar primero;
  borde estilo Super Game Boy = 3 propuestas para elegir; scratchpads de otras sesiones en %TEMP% = no tocar.
- **Usuario 22/10 ~05:40:** en el launcher (Settings) el modo gráfico viene por defecto en **"Escoger"**: así el
  juego muestra la elección de modo la primera vez que el jugador empieza. En nuestras Opciones, junto a "volver todo
  a default", una opción para **volver a los valores por defecto de un modo** (HD, GBA mejorado, GBA clásico), por
  si alguien cambia de opinión.
- **Usuario 22/10:** rescatar **todos** los assets HD de Re:CoM que se puedan (rastrear todo el .pkg, reconocer lo
  que es 1:1 como las cartas) y lograr lo máximo posible: **pantalla de título HD** con esos assets y nuestras
  opciones, HUD, menús. Inventario en HUD_HD_RECOM.md.
- Darkside: sesión [darkside-arte] (worktree D:/01Claude/arenas-wt) lo termina y lo entrega para aprobar.
- Después (pedido del usuario): datos duros de Re:CoM (dificultades, valores de cartas/enemigos, cambios respecto
  a GBA) para el modo Re:CoM, en lógica y jugabilidad, con pruebas para no meter bugs.

### Bugs encontrados y cerrados el 22/10 (todos en main)
- Escena HD: destello de la imagen del evento al terminar (53f756bc7).
- Música del campo bajo el video HD y corte seco; salto sin fundido de imagen y sonido (53f756bc7, PC y Android).
- Pausa del video HD muda (1c8a17352).
- Agrabah 107/108/109 aprobadas con honores pero apagadas para jugadores (d94ce3b56).
- Suelo isométrico: tabla única recalculada por píxel, <5 fps a x1 en 004/057/061/074 (también pesaba en Android,
  web y consolas, un núcleo): 31 ms por cuadro, píxel a píxel igual (e1b635d65).
- Batería: resultados del 21/10 viejos (Event_0, Pooh) ya arreglados; 0 sitios abiertos.
- Pendiente de confirmar: timeout Android US combate 124 (Agrabah) con dos corridas en el mismo emulador; repetir solo.

### Pendientes v2.0 (en orden)
- ~~Arte extendido a x1 en 1080p~~ **hecho 22/10**: a x1 1920x1080 todo lo aprobado ya llega al borde (Guard
  Armor, 004, 008, 057, 061, 074, Agrabah 107/108/109; `build/x1art`). Agrabah 107/108 y 109 (con honores, 21/10)
  estaban detrás de `KHCOM_AGRABAH_WIDE`/`KHCOM_AGRABAH109_WIDE` esperando la integración: ahora encendidas para los
  jugadores (la variable en 0 las apaga para antes/después); `test_agrabah_backdrop.py` 11/11,
  `test_agrabah109_backdrop.py` 10/10 (EU/US/JP, rectángulo GBA idéntico).

1. ~~Escenas HD: disparo real~~ **hecho 22/10**: `test_hd_scenes.py` (cada evento desde el menú de depuración,
   video cortado a 1 s): EU 97/97 en modo original y 97/97 en Re:CoM; 001/614/631 solo en Re:CoM y en su orden. Todos
   los eventos (también los mudos) pasan por `UpdateEventSeq`.
2. ~~Transiciones~~ **hecho 22/10**: al terminar la escena HD el fundido del evento empieza ya en negro (antes volvía
   la imagen del evento a brillo 234 y se fundía 64 cuadros); saltar el video funde imagen y sonido 0,3 s (PC y
   Android); el sonido del juego y la música HD bajan en 150 ms antes del video (antes 200 ms de la música del campo
   sonaban debajo y se cortaban). Medido con el driver "disk" (`build/hdtrans`).
3. ~~358/2 Days~~ **hecho 22/10**: KHDays FM (melonDS, github.com/vitor251093/KHDays_FM) pausa el video HD con
   Saltar/Continuar y sonidos de menú, y baja la música 1 s antes. Tomado: la pausa del video HD suena como los menús
   del juego; la música ya baja antes del video (150 ms, con la pantalla en negro).
4. ~~Consolas~~ **documentado 22/10** en HD_MOD.md (decodificador por consola, qué falta en cada host).
5. Revisar issues abiertos al final, arreglar bugs, responder breve.

### Lista de verificación antes de publicar (no saltear)
- `release_check.py v2.0` antes y `--post` después (versión en launcher, rc, CHANGELOG, web, paquetes, Quiver).
- `check_sources.py` (lo corre build_android y release_check): constantes del shim de consolas.
- Builds: Windows, Linux, Android, PS4, Switch, Vita, PS5 (VM), web — desde el commit final; ningún paquete viejo
  (los scripts de VM ya no reusan paquetes, verificar "fetched" en los logs).
- Combate de prueba con cada ejecutable de PC (`KHCOM_BATTLE=10:4`), sin `bios:`/CRASH.
- `debug_kit.py report` (baterías de escritorio: scenes, events, wide, rooms, battles, riku, monkey, skip) sin
  sitios nuevos.
- Android 64 (`run_android_full.py` / pasos `android ...` de run_batteries): imagen movida + jefes EU/US/JP, Riku.
- `test_hd_audio.py` (ningún efecto tomado por música), `test_hd_audio_flow.py` con `uv run --with numpy --with
  soundfile` (driver "disk": 0 cortes, 0 solapamientos, 0 saltos audibles; incluye Opciones abiertas).
- `test_frameskip.py` (lógica idéntica con salto de cuadros).
- Videos HD: `KHCOM_HDMOVIE_PAUSETEST` (pausa y salto), renderer por software, Android (`KHCOM_MOVIE=1` en el emulador,
  `build/hdtest/run_hdmovie.py` en khcom-claude-hdmov).
- `match_hd_scenes.py --check` (asignaciones consistentes).
- Copia de prueba para el usuario + `.bat`; notas cortas; nunca decir en público que algo no se probó en hardware.

## 21/10 noche (usuario ausente, /goal: bugs y pendientes sin imágenes)

- **v1.9.15-pre1 publicada (prueba: Windows, Switch, Vita)**, comentada en #35 y #36. Incluye: #36 (táctil oculto en
  pausa/Opciones, Minus + Plus 3 s abre Opciones), salto de cuadros (`experimental_frame_skip`, encendido en todo tras
  `test_frameskip.py` 24/24 idénticos y 1/3600 saltados en PC con VSync; RENDIMIENTO.md), Vita en GPU (SDL2 GXM de
  vdpm), cola de audio del puente en unidades de entrada.
- **#36 cerrado** (DrunkECheese probó la pre1: sin quejas).
- **No se pueden cerrar desde acá (21/10 noche):**
  - D. Videos HD en Linux: el paquete es i386 con sus propias librerías Debian; haría falta empaquetar ffmpeg i386
    (libavcodec/libavformat + dependencias, decenas de MB) o portar openh264 + minimp4 + un decodificador AAC.
    Decisión de tamaño/diseño para el usuario. Consolas (sceAvPlayer, nvdec): sin hardware para verificar.
  - I'. PS4 a la GPU (Piglet): sin PS4 ni emulador para verificar; el rasterizador por software sigue, y el salto de
    cuadros ya mantiene la velocidad si no llega.
  - Borrar releases v1.9.12/v1.9.13-pre2 y quitar los worktrees viejos: el clasificador lo bloquea (lo hace el usuario).
- **C. Videos HD en Android: hecho** (RGB por defecto, verificado en el emulador; HD_MOD.md). Rama en main.
- **F. Discord en Linux: hecho.** VM Debian con un socket falso: handshake y SET_ACTIVITY (mundo, piso, nivel, combate).
- #34 cerrado (confirmado). #39 (recortar relleno) respondido: idea, no bug.
- Builds de consolas: `build_ps5.py` enlaza con archivo de respuesta (límite de 32 K) y lee bien los `.d` con rutas de
  Windows; Switch/Vita/PS5 ya no reusan un paquete viejo si la VM falla; Vita rellena 8 KiB si `vita-elf-create` no
  tiene lugar.
- Baterías: `run_batteries.py` suma battles, riku, monkey y skip (EU/US/JP); el AVD se encuentra con ANDROID_AVD_HOME.

## Carpetas de KHCoMPC (21/10, pedido del usuario)

La raíz quedó en: `jugar/` (copias ReKHCoM-vX con partidas del usuario + su .bat), `pruebas/` (copias que esperan
aprobación + .bat), `roms/`, `arte/` (fuentes, imágenes, Showcase, Trailer), `respaldos/`, `toolchain/` y los repos.
Cada una tiene `LEEME.md`. **Codex:** `package_preliminar_v1.py` ahora escribe en `../pruebas/ReKHCoM-preliminarV1`,
`crystal_parity.py` lee `../respaldos/` y el tráiler sale en `../arte/Trailer/` (main); trae main antes de usarlos.

## Reparto

| Quién | Pendiente |
|---|---|
| **Claude** | **J + I a main**: cuando las dos baterías de arriba terminen limpias, build pc-eu/us/jp, prueba de combate, merge a main, APK de prueba. Con el OK del usuario: prerelease y pedir a #22/#34 que prueben; #37/#38 se cierran con la confirmación de xsm2 |
| **Claude** | **A. Batería Android 64 completa**: EU jefes con Riku, US/JP menús y jefes; luego otros perfiles (Android 13/15, tablet, plegable como el Mate XT) y el bot de partida (`ANDROID_64.md`) |
| **Claude** | **B. Batería de escritorio** (`run_batteries.py`): cada `bios:` nuevo se arregla (`KIT_DEPURACION.md`) |
| **Claude** | **C. Videos HD en Android**: rama `claude/android-hd-movies`; el emulador se cuelga en la primera textura YUV. Probar `KHCOM_HDMOVIE_RGB=1` (b4cb4730d) en el emulador; a main solo verificado (`HD_MOD.md`). Luego **D**: consolas y MP4 en Linux |
| **Claude** | **E'. Reportes nuevos (21/10 ~16:15, prioridad del usuario)**: #22 xsm2 Poco F8 Ultra y #34 Mate XT con la pre2: "The game image could not be loaded" = bug J (imagen de 64 bits sin hueco en 0x01000000), arreglado en `claude/perf-android-reloc`; xsm2 no llega a Settings para el log (la rama pone "Share log" en la caja de error). Odin 2 Portal anda bien con la pre2. Casos en la batería: grupo `issues` de `test_android_scenarios.py` (#33 evento 099, Bosque 100 Acres) y paso `android eu move` con `KHCOM_IMAGE_MOVE=1`. `mode_vsbattle` sale de la batería: arnés sin compañero de link (HARNESS_ONLY) |
| **Claude** | **E. Issues**: #33 (Trickmaster, arreglado en pre2) y #35 (Switch lento: si sigue con la GPU, deko3d) esperan a los testers; #36 (Switch se cierra en el título): comentario listo en `build/issue36-comment.md`, sale con la próxima prueba; #25 se cierra cuando el mod HD salga en una estable |
| **Claude** | **G. Integración de Codex**: traer sus commits (`khcom-codex` codex/preliminar-v1 y `khcom`) a main con build y prueba (al día hasta 457d10854) |
| **Claude** | **I'. Rendimiento en consolas**: Vita a la GPU (necesita OK del usuario para `vdpm sdl2` en la VM), PS4 a la GPU (Piglet) (`RENDIMIENTO.md`) |
| **Claude** | **H. Próxima estable**: solo con el OK del usuario; `release_check.py` antes/después, todas las plataformas, web |
| **ChatGPT/Codex** | `port/tools/scene_wide_review.py` no compila (líneas mezcladas por un merge, 18/10): arreglarlo o borrarlo; `check_sources.py` lo excluye (KNOWN_BROKEN) hasta entonces |
| **ChatGPT/Codex** | 39: rutas completas de Sora y de Reverse/Rebirth |
| **ChatGPT/Codex** | 16–24: arte de jefes y escenas, de a uno con antes/después, con el método de abajo ("Para Codex: cómo hacer el arte") |
| **Usuario** | Probar `Probar Discord.bat` y `Probar ReKHCoM v1.9.13 pre2.bat` (pantalla completa sin parpadeo, mod HD, volver al título) |
| **Usuario** | Aprobar el launcher nuevo (`Probar launcher nuevo.bat`, commit 07a6fa9ab); revisar 61–62 cuando Claude las muestre |

## Para Codex: cómo hacer el arte (16–24), según lo aprobado (21/10/2026)

Pedido del usuario: seguir el mismo método con que se aprobaron los arreglos visuales.

- **Procedimiento:** `WORKFLOW_ARTE_AMPLIO.md` (paso a paso), reglas en `LOGICA_APROBADOS.md`
  (mandan ante la duda), entrega en `PRUEBAS_VISUALES.md`, técnica de combates en
  `BATTLE_SCENERY_TECHNIQUE.md`.
- **Modelo "aprobado con honores" (20/10): Guard Armor, escena 008.** El patio del combate cubre
  también el evento previo: la extensión se elige por los cuatro mapas del fondo, no por el modo
  (`port_ppu.c`, `guardPatio`). Lección: buscar TODAS las apariciones de un fondo (combate, evento
  previo, derrota, revancha) e identificarlo por sus datos, nunca por modo/número de combate/bandera.
- **Arte:** solo tiles del juego (repetir/voltear); bordes cortados se cierran con el borde propio
  volteado; nunca ornamentos repetidos o cortados a la mitad (relleno de pared lisa); pixel art
  nuevo solo donde falte, con píxeles y paleta del juego. El rectángulo GBA queda idéntico.
- **Pruebas:** zoom x3 entero (`zoom=6`) a 1920x1080, 2560x1080, 3440x1440 y 3840x1080
  (`KHCOM_WINDOW`), antes/después.
- **Entrega:** copia jugable + `.bat`, de a un jefe/escena, y esperar la aprobación del usuario;
  lo aprobado se documenta en `LOGICA_APROBADOS.md`.

## Vita y Switch (20/10, 17:00)

Por qué es distinto de PC/PS5/PS4: el juego lee los datos del volcado del usuario, y esos datos
llevan punteros absolutos de GBA (0x08xxxxxx). PC, Android, web, PS5 y PS4 cargan la ROM en
0x08000000 y los punteros valen tal cual. Switch (AArch64, código desde 0x08000000, homebrew sin
procesos de 32 bits) y Vita (ARM32, usuario desde 0x81000000) no pueden usar esas direcciones.

Nuestro método (PORT_GBA_RELOC, `include/gba/gba_space.h`): todas las regiones GBA son arreglos del
programa; al cargar la ROM se corrigen sus punteros con la lista exacta del enlazador
(`port/data/rom_pointers_*.inc`, de reenlazar la ROM del decomp con `--emit-relocs`), y los
accesos sueltos a direcciones de bus se traducen (`GbaSpacePtr`). Una misma imagen sirve a las dos.
ggdrn/KH_COM_VITA (MIT) hace lo equivalente de otra forma: enlaza los datos dentro del ejecutable
(el enlazador de Vita corrige los punteros), los pone a cero y los rellena desde la ROM del usuario
al arrancar; los punteros dentro de bloques binarios los traduce con `GBA_PTR()` en cada uso.
Las dos vías necesitan que el decomp compile la ROM idéntica al cartucho.

Hecho:
- Modo reubicable: cabeceras, símbolos fijos (`rom_symbols_reloc.s`), build de comprobación en
  Windows (`configure_pc.py --reloc`) con el rango del bus reservado sin acceso.
- Build idéntica de GBA recuperada (7c7fb0f09, 62777ed3f): ROM EU/US/JP iguales al cartucho.
- Listas de punteros de EU/US/JP (`gen_rom_pointers.py`, 132037/98603/98035 palabras), cada una
  comprobada contra el cartucho; incluyen las desalineadas (GOTO/PATT de las canciones).
- Switch (`port/switch`, README): imagen AArch64 de punteros de 32 bits para Cortex-A57 (sin
  `-mcpu` clang emitía `xpaci` de ARMv8.3 y caía), memcpy/memset de 16 bytes, `.nro` de las tres
  regiones (`build_switch.py host`, devkitA64 de la imagen Docker devkitpro/devkita64 porque
  apt.devkitpro.org responde 403).
- Vita (`port/vita`, README): misma imagen reubicable en ARMv7 (clang, Cortex-A9), aislada en un
  objeto con solo `guest_main`/`gHostTable` globales y enlazada con el host compartido
  (`switch_host.c` con `__vita__`: rasterizador a 960x544, sceAudioOut, sceCtrl); un ejecutable por
  región (eboot.bin EU, eboot_us/jp.self). VitaSDK en la VM de consolas. `build_vita.py test` corre la
  misma imagen en Linux ARM de 32 bits.
- Verificado en la VM ARM64: título, menús y la batalla KHCOM_BATTLE=10:4 en Switch y en Vita; el
  fotograma 1490 sale idéntico píxel a píxel en las dos imágenes.

Sin verificar (no hay consola ni emulador aquí): mapeo de memoria de código en Switch real, pantalla,
audio y mandos del host de Vita, rendimiento. Llega como reporte de quien lo pruebe.


## Esperando a terceros (no se trabaja)

- Quiver: issue tgeorgiadis/quiver-launcher#182, pedimos levantar la retención del
  repositorio (VirusTotal 0/68; la causa fueron releases borrados mientras los revisaban).
