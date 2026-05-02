# kmid3 — Roadmap de desarrollo

Este documento describe el estado actual del proyecto, los bugs conocidos con su diagnóstico técnico, y las mejoras planificadas en orden de prioridad. Está escrito para que cualquier desarrollador (o IA) pueda retomar el trabajo sin contexto previo.

---

## Estado actual (mayo 2026)

- Port funcional de KMid 2.4.0 a Qt5 / KDE Frameworks 5 / drumstick moderno
- Compila y corre en Debian 12 con TiMidity++ como sintetizador
- Backend ALSA carga correctamente desde `/usr/local`
- Toolbar con todos los botones de reproducción visible
- Karaoke con letra sincronizada funcionando
- Pianola (piano visual) operativa

---

## Bugs conocidos — a corregir primero

### BUG 1 — Pausa no reanuda desde la posición actual (PRIORIDAD ALTA)

**Síntoma:** Al pulsar Pausa y luego Play, la canción vuelve a empezar desde el inicio en lugar de continuar desde donde se pausó.

**Diagnóstico técnico:**
El método `pause()` en `src/kmid2.cpp` llama a `m_midiobj->pause()` correctamente cuando el estado es `PlayingState`. El problema está en el método `play()`, que llama a `displayLyrics()` antes de `m_midiobj->play()`. `displayLyrics()` llama a `m_midiobj->seek(0)` indirectamente a través de `slotSourceChanged` o reinicia el estado interno del objeto MIDI. Hay que investigar si el backend ALSA (`alsa/alsamidiobject.cpp`) implementa correctamente la reanudación desde posición pausada, o si `play()` necesita distinguir entre "iniciar desde cero" y "reanudar desde pausa".

**Archivos a revisar:**
- `src/kmid2.cpp` — métodos `play()` y `pause()`
- `alsa/alsamidiobject.cpp` — implementación de `pause()` y `play()` en el backend
- Verificar si `m_midiobj->state()` devuelve `PausedState` correctamente antes de llamar a `play()`

**Solución probable:**
En `play()`, comprobar si el estado actual es `PausedState` y en ese caso llamar directamente a `m_midiobj->play()` sin pasar por `displayLyrics()` ni reiniciar el slider.

---

### BUG 2 — Ventana Pianola aparece con tamaño mínimo al abrirse por primera vez (PRIORIDAD MEDIA)

**Síntoma:** La primera vez que se abre Ver > Pianola, la ventana aparece muy pequeña (casi invisible). El usuario tiene que redimensionarla manualmente. A partir de la segunda ejecución del programa, KDE recuerda el tamaño guardado y ya abre bien.

**Diagnóstico técnico:**
La Pianola hereda de `KMainWindow` y usa `setAutoSaveSettings("PlayerPianoWindow", true)` en `src/pianola.cpp`. Esto guarda y restaura geometría automáticamente, pero solo funciona si ya existe una entrada guardada en la configuración de KDE. La primera vez no hay nada guardado, y el tamaño inicial que calcula Qt es demasiado pequeño porque los 16 canales MIDI están ocultos (`m_frame[i]->setVisible(false)`) y el widget central queda vacío.

**Archivos a revisar:**
- `src/pianola.cpp` — constructor de `Pianola`
- `src/pianola.h`

**Solución probable:**
Establecer un tamaño mínimo razonable con `setMinimumSize()` o `resize()` en el constructor de `Pianola`, por ejemplo `resize(800, 120)`, para que la primera apertura tenga un tamaño usable. `setAutoSaveSettings` se encargará de recordar el tamaño que el usuario elija después.

---

### BUG 3 — Diálogo "Abrir archivo" no recuerda el último directorio visitado (PRIORIDAD MEDIA)

**Síntoma:** Cada vez que se usa Archivo > Abrir, el diálogo se abre en el directorio home en lugar del último directorio donde se abrió un archivo.

**Diagnóstico técnico:**
En `src/kmid2.cpp`, el método `fileOpen()` llama a `QFileDialog::getOpenFileUrls()` pasando `QUrl()` como directorio inicial (segundo argumento vacío). Eso hace que Qt siempre abra en el home. La solución es guardar el último directorio usado en la configuración de KDE y pasarlo como directorio inicial.

```cpp
// Código actual (src/kmid2.cpp ~línea 399):
QList<QUrl> urls = QFileDialog::getOpenFileUrls(
    this, i18nc("@title:window","Open MIDI/Karaoke files"),
    QUrl(),   // <-- aquí está el problema
    QStringLiteral("MIDI files (*.mid *.midi *.kar);;All files (*)"));
```

**Archivos a modificar:**
- `src/kmid2.cpp` — método `fileOpen()`
- `library/kmid.kcfg` — añadir clave `last_open_dir` de tipo `String`
- `library/settings.kcfgc` — exponer la nueva clave

**Solución:**
1. Añadir `last_open_dir` al archivo `.kcfg`
2. En `fileOpen()`, leer `m_settings->last_open_dir()` como directorio inicial
3. Tras una selección exitosa, guardar `urls.first().adjusted(QUrl::RemoveFilename)` en `m_settings`

---

## Mejoras planificadas

### MEJORA 1 — Recordar también el directorio de "Guardar letra" y "Cargar/Guardar playlist"

Los mismos diálogos `QFileDialog` en `fileSaveLyrics()`, `slotLoadPlaylist()` y `slotSavePlaylist()` tienen el mismo problema que BUG 3. Aplicar la misma solución con claves separadas en el `.kcfg`.

---

### MEJORA 2 — Soporte para FluidSynth como alternativa a TiMidity

TiMidity++ tiene latencia alta y configuración manual. FluidSynth con `fluidsynth --server` ofrece mejor calidad y menor latencia. Añadir detección automática de FluidSynth en el backend ALSA y documentarlo en el README.

---

### MEJORA 3 — Indicador visual de progreso en la letra (karaoke)

Actualmente la letra se muestra completa y se resalta la sílaba activa con color. Mejorar el resaltado para que sea más visible: fuente más grande para la línea activa, scroll automático más suave, y opción de mostrar solo la línea actual centrada en pantalla.

---

### MEJORA 4 — Soporte para archivos `.kar` con metadatos de título y artista

Los archivos Karaoke `.kar` contienen metadatos en los primeros eventos MIDI. Mostrarlos en la barra de título y en un panel de información al cargar el archivo.

---

### MEJORA 5 — Empaquetado como .deb para Debian/Ubuntu

Crear un `debian/` con los archivos de control necesarios para generar un paquete `.deb` instalable con `dpkg`. Esto facilita la distribución sin necesidad de compilar desde fuente.

---

## Orden de trabajo recomendado

1. **BUG 1** — Pausa/Play (impacto directo en usabilidad básica)
2. **BUG 3** — Último directorio en diálogo Abrir
3. **BUG 2** — Tamaño inicial de la Pianola
4. **MEJORA 1** — Extender BUG 3 a los demás diálogos
5. **MEJORA 3** — Karaoke mejorado
6. **MEJORA 2** — FluidSynth
7. **MEJORA 4** — Metadatos `.kar`
8. **MEJORA 5** — Paquete `.deb`
