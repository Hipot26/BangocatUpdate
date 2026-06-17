# Bongo Cat OBS Plugin — Extended Edition

> **Basado en el trabajo original de [a1928370421](https://github.com/a1928370421/Bongobs-Cat-Plugin)**  
> Modificado y extendido con nuevas funcionalidades manteniendo compatibilidad retroactiva.

---

## Créditos

| Autor | Contribución |
|-------|-------------|
| **a1928370421** | Creador original del plugin Bongobs Cat para OBS. Sistema completo de renderizado, integración con Live2D Cubism SDK, captura de teclado via Hook, sistema de modos y toda la arquitectura base. |
| **Live2D Inc.** | Cubism SDK y Framework. Licencia Open Software de Live2D. |
| **Weng Y** | Código original de VtuberPlugin, VtuberDelegate y VtuberFrameWork (2020). |

Este proyecto no sería posible sin el trabajo original. Todos los créditos del código base pertenecen a sus autores.  
Repositorio original: https://github.com/a1928370421/Bongobs-Cat-Plugin

---

## ¿Qué se añadió?

Esta versión extiende el plugin original con las siguientes funcionalidades, sin romper la compatibilidad con los modos y assets existentes:

### 1. Orden Z configurable por modo (`LayerOrder`)
Cada modo puede definir el orden de renderizado de sus capas directamente en su `config.json`. Si no se define, se usa el orden original del plugin (retrocompatible).

**Capas disponibles:**
| Nombre | Descripción |
|--------|-------------|
| `background` | Fondo (bg.png / catbg.png) |
| `cat` | Modelo Live2D del gato |
| `keys` | Teclas iluminadas al presionar |
| `left_hand` | Pata/mano izquierda |
| `right_hand` | Pata/mano derecha |
| `mask` | Máscara/expresión por hotkey (sistema original) |
| `face_anim` | Animación facial por sprites (parpadeo/boca) |
| `skin_back` | Accesorio detrás del gato |
| `skin_front` | Accesorio encima del gato, debajo de manos |
| `skin_over` | Accesorio encima de todo |
| `up_hands` | Patas levantadas cuando no se presiona nada |

**Ejemplo en config.json:**
```json
{
  "LayerOrder": [
    "background",
    "skin_back",
    "cat",
    "face_anim",
    "skin_front",
    "left_hand",
    "right_hand",
    "skin_over",
    "keys",
    "up_hands"
  ],
  ...resto de campos originales...
}
```

---

### 2. Animación facial por sprites (parpadeo + boca)
Sistema de estados de animación que funciona intercambiando imágenes PNG transparentes sobre el gato. No requiere Live2D.

**Cómo funciona:**
- `idle` — expresión base (ojos abiertos, boca cerrada)
- `blink` — parpadeo automático aleatorio sin hablar
- `talk` — boca abierta al detectar voz
- `blink_talk` — parpadeo mientras habla (boca sigue abierta)

**Archivos necesarios:**
```
Resources/Bango Cat/face/
├── config.json     ← configuración de estados y intervalos
├── 0.png           ← ojos abiertos, boca cerrada (idle)
├── 1.png           ← ojos abiertos, boca abierta (talk)
├── 2.png           ← ojos cerrados, boca abierta (blink_talk)
└── 3.png           ← ojos cerrados, boca cerrada (blink)
```

**face/config.json:**
```json
{
  "enabled": true,
  "idleState": "idle",
  "autoBlink": true,
  "blinkIntervalMin": 2.0,
  "blinkIntervalMax": 5.0,
  "states": [
    { "name": "idle",       "loop": true,  "frameDuration": 99.0, "frames": ["0.png"] },
    { "name": "blink",      "loop": false, "frameDuration": 0.06, "frames": ["0.png","3.png","3.png","0.png"] },
    { "name": "talk",       "loop": true,  "frameDuration": 0.12, "frames": ["1.png","1.png","1.png","1.png","2.png","1.png"] },
    { "name": "blink_talk", "loop": false, "frameDuration": 0.06, "frames": ["1.png","2.png","2.png","1.png"] }
  ]
}
```

**Notas para dibujar los sprites:**
- Mismo tamaño que `catbg.png`
- Fondo 100% transparente (canal alpha)
- Dibuja solo ojos/boca, alineados con la cara del gato en `catbg.png`

---

### 3. Captura de voz para activar animación talk
Captura directa del micrófono usando **WASAPI** (Windows Audio Session API), igual que Veadotube Mini. No depende del audio de OBS.

**Controles nuevos en las propiedades del plugin:**
- **Enable Talk Animation** — activa/desactiva la captura de micrófono
- **Microphone** — lista desplegable con todos los micrófonos del sistema
- **Talk Threshold (sensitivity)** — slider de 0.0 a 0.1 (recomendado: 0.02)

El sistema tiene suavizado con ataque rápido y caída lenta para que la boca no parpadee entre sílabas.

---

### 4. Accesorios/Skins con toggle por hotkey y puntos de canal

Sistema de accesorios (gorros, pelucas, gafas, etc.) que se activan/desactivan con tecla o con puntos de canal de Twitch/YouTube.

**Estructura:**
```
Resources/Bango Cat/skins/
├── config.json
├── halloween/
│   └── hat.png
├── santa/
│   └── hat.png
└── sunglasses/
    └── glasses.png
```

**skins/config.json:**
```json
{
  "skins": [
    {
      "id":            "halloween",
      "image":         "halloween/hat.png",
      "hotkey":        "f5",
      "zOrder":        "front",
      "duration":      180,
      "channelPoints": 500
    }
  ]
}
```

**Valores de zOrder:**
- `"back"` — detrás del gato (bufandas, alas)
- `"front"` — encima del gato, debajo de manos (gorros, pelucas)
- `"over_hands"` — encima de todo (efectos, ítems sostenidos)

**Duración:** en segundos. `0` = permanente hasta otro toggle.  
**Toggle:** la misma tecla activa y desactiva el accesorio.  
**Canal:** llamar `view->OnChannelPointReward("halloween", id)` desde tu integración de Twitch.

---

### 5. Offset Y para modelos Live2D por modo
Permite ajustar la posición vertical de los modelos Live2D (como la mano del ratón en el modo `standard`) sin modificar las imágenes PNG.

**En config.json del modo:**
```json
{
  "ModelRightHandOffsetY": -0.078,
  ...
}
```

Unidades: coordenadas Cubism. `1.0` = altura completa del canvas. Negativo = baja, positivo = sube.  
Para calcular el offset de N píxeles: `offset = N / 768 * 2`

---

### 6. Teclas adicionales en KeyDefine
Se añadieron nuevas teclas disponibles para usar en `KeyUse`, hotkeys de skins y face animation:

| Nombre | Tecla |
|--------|-------|
| `lalt` | Alt izquierdo |
| `ralt` | Alt derecho |
| `tab` | Tab |
| `esc` | Escape |
| `num0`–`num8` | Teclado numérico 0-8 |

---

## Compilación

### Requisitos
- Windows 10/11
- Visual Studio 2022 Build Tools (con "Desarrollo para el escritorio con C++")
- CMake 3.10+
- OBS Studio instalado

### Pasos

```cmd
cd "ruta\al\plugin"
mkdir build && cd build

cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_EXE_LINKER_FLAGS="/machine:x64 /OPT:REF" ^
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5

cmake --build . --config Release
```

```cmd
copy /Y "build\Release\bongobs-cat.dll" "C:\Program Files\obs-studio\obs-plugins\64bit\"
xcopy "Resources" "C:\Program Files\obs-studio\data\obs-plugins\bongobs-cat\" /E /I /Y
```

### Dependencias del CMakeLists.txt

Asegúrate de que `target_link_libraries` incluya:
```cmake
target_link_libraries(${APP_NAME}
    "C:/ruta/a/obs.lib"
    Framework
    glfw
    ${OPENGL_LIBRARIES}
    mmdevapi
    ole32
    oleaut32
)
```

Y que `MicCapture.cpp` esté en la lista de sources.

---

## Estructura de archivos modificados

```
Bongobs-Cat-Plugin/
├── InfoReader.hpp/cpp   ← LayerOrder, face anim, skins, ModelRightHandOffsetY
├── View.hpp/cpp         ← Render por LayerOrder, animación facial, skins
├── Live2DManager.hpp/cpp← OnUpdate con offsetY para posicionamiento de modelos
├── VtuberPlugin.hpp/cpp ← Captura WASAPI, lista de micrófonos, slider de sensibilidad
├── MicCapture.hpp/cpp   ← NUEVO: captura de audio via WASAPI
└── Define.cpp           ← Teclas adicionales (lalt, ralt, tab, esc, numpad)
```

---

## Licencia

El código original está bajo **GNU General Public License v2.0**.  
El SDK de Cubism está bajo la **Live2D Open Software License**.  
Las modificaciones de esta versión siguen la misma licencia GPL v2.0.