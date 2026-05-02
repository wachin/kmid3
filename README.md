# kmid3

Port de **KMid 2.4.0** actualizado para compilar y funcionar en 2026 con Qt5, KDE Frameworks 5 y drumstick moderno.

---

## Dependencias de compilación

Instala los paquetes necesarios en Debian/Ubuntu:

```bash
sudo apt-get install -y \
    libkf5coreaddons-dev \
    libkf5i18n-dev \
    libkf5xmlgui-dev \
    libkf5kio-dev \
    libkf5parts-dev \
    libkf5config-dev \
    libkf5configwidgets-dev \
    libkf5widgetsaddons-dev \
    libkf5textwidgets-dev \
    libkf5iconthemes-dev \
    libkf5notifications-dev \
    libdrumstick-dev \
    libasound2-dev \
    qtbase5-dev \
    qttools5-dev \
    extra-cmake-modules
```

Si también quieres los manuales HTML de KDE:

```bash
sudo apt-get install -y libkf5doctools-dev
```

---

## Cómo compilar e instalar

```bash
cd /home/wachin/Dev/kmid2/kmid-2.4.0
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build . -j"$(nproc)"
sudo cmake --install .
```

Para instalar en el mismo prefijo que el resto de KDE/Qt del sistema, muchos empaquetadores usan `-DCMAKE_INSTALL_PREFIX=/usr` y ajustan `CMAKE_INSTALL_PREFIX` según su política.

Con `cmake --install`, el ejecutable **`kmid`**, **`libkmidbackend.so`**, **`kmid_alsa.so`** y **`kmid_part.so`** quedan en las rutas estándar del prefijo para que se carguen el backend ALSA y el KPart.

### Probar sin instalar

```bash
LD_LIBRARY_PATH=/ruta/al/build/bin /ruta/al/build/bin/kmid
```

Ajusta la ruta a tu directorio `build`.

---

## Qué se cambió en el árbol (resumen)

- **CMake / KDE**: rutas `KDE_INSTALL_*` al estilo KF5; documentación opcional con **KF5DocTools** (`libkf5doctools-dev` si quieres los manuales HTML).
- **Drumstick**: includes `<drumstick/…>`, `using namespace drumstick::ALSA`, `drumstick::File::QSmf`, `#include <drumstick/sequencererror.h>`, `-fexceptions` en el plugin ALSA.
- **Tipos**: `Settings` de KConfig es global (no `KMid::Settings`); `KDE_EXPORT` eliminado en `ALSABackend`; `VERSION` en `config.h` desde `@PROJECT_VERSION@`.
- **Qt5/KF5 UI**: cabeceras tipo `KColorButton`, `QFontComboBox`/`QSpinBox` donde KF5 ya no expone el widget antiguo; `QListWidget` + conexión en código para la lista MIDI.
- **Otros**: `QProcess` en sustitución de `KProcess`/`KUrl`; `sendSeqEvent` para no chocar con `QObject::sendEvent`; `setRotation`, `itemAt(..., QTransform())`, `Qt5::Svg`, etc.

---

## Notas

- Los avisos de **iconos** (`hi16-app-kmid.png` frente al formato que espera `ecm_install_icons`) son cosméticos; se pueden renombrar más adelante si quieres silenciar ECM.
- Si al ejecutar falta algún `.so` o plugin en tiempo de ejecución, indica el mensaje exacto de la terminal y se puede resolver ajustando el prefijo de instalación o con `LD_LIBRARY_PATH` / `QT_PLUGIN_PATH`.
