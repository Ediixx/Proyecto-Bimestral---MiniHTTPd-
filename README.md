# minihttpd: Servidor Web HTTP/1.1 en C

**Asignatura:** Computación Distribuida  
**Proyecto:** Implementación de Servidor Web Monohilo 
**Entorno de Operación:** Linux / WSL (Ubuntu 24.04 LTS)  
**Estándar de Lenguaje:** C11 (GNU Compiler Collection - GCC)  

## 1. Objetivos de Aprendizaje
* **Programación de Red con Sockets POSIX:** Creación, configuración, enlace (*bind*), escucha (*listen*) y aceptación (*accept*) de sockets TCP/IP.
* **Procesamiento y Semántica del Protocolo HTTP:** Deserialización de peticiones (`Request Line`, encabezados) y ensamblado manual de tramas de respuesta bajo especificación HTTP/1.1.
* **Modelo Concurrente y Gestión de Eventos:** Comprensión de arquitecturas de atención secuencial/event-driven en entornos monohilo.
* **Manejo Seguro del Sistema de Archivos:** Lectura de recursos en disco, tipificación MIME para contenido estático (HTML, CSS) y binario (imágenes), así como prevención de fugas de descriptores de archivo.
* **Seguridad Ofensiva/Defensiva en Redes:** Mitigación de desbordamiento de búfer (*Buffer Overflow*) y control de acceso contra escalamiento de rutas (*Path Traversal*).

## 2. Requisitos del Entorno
* **Sistema Operativo:** Distribución Linux (compatible con WSL Ubuntu 24.04 LTS o superior).
* **Compilador:** `gcc` con soporte para estándar C11 y POSIX.
* **Automatización de Construcción:** `make` (GNU Make 4.x o superior).
* **Herramientas de Diagnóstico y Test:** `curl`, `netcat` (`nc`), `python3`.

## 3. Estructura y Arquitectura del Proyecto

```text
minihttpd/
├── Makefile                # Reglas de compilación y limpieza de binarios
├── README.md               # Informe técnico y documentación del proyecto
├── include/                # Interfaces y cabeceras del sistema (.h)
│   ├── server.h            # Configuración de red y ciclo de vida del socket
│   ├── http.h              # Estructuras de datos y parsing del protocolo HTTP
│   ├── mime.h              # Mapeo de tipos de contenido MIME
│   └── files.h             # Abstracción para I/O del sistema de archivos
├── src/                    # Implementación de módulos (.c)
│   ├── main.c              # Punto de entrada y gestión de argumentos CLI
│   ├── server.c            # Primitivas de socket y bucle de despacho de eventos
│   ├── http.c              # Parser sintáctico y generador de respuestas HTTP
│   ├── mime.c              # Clasificador de extensiones y Content-Type
│   └── files.c             # Lectura segura y transferencia de bytes al socket
└── www/                    # Directorio raíz de recursos web estáticos (DocumentRoot)
    ├── index.html          # Documento HTML principal
    ├── style.css           # Hoja de estilo en cascada de prueba
    └── image.png           # Recurso multimedia binario
```

### Descripción Modular

| Módulo / Archivo | Rol en la Arquitectura |
| :--- | :--- |
| **`include/server.h` / `src/server.c`** | Encapsula la inicialización del socket maestro: configuración de `sockaddr_in`, opción `SO_REUSEADDR`, `bind` y `listen`. |
| **`include/http.h` / `src/http.c`** | Analiza la línea de petición (`METHOD`, `URI`, `VERSION`) y los encabezados (`Host`, `Connection`, `User-Agent`), controla conexiones persistentes (`keep-alive`) y genera las respuestas HTTP. |
| **`include/mime.h` / `src/mime.c`** | Asigna el encabezado `Content-Type` adecuado (`text/html`, `text/css`, `application/javascript`, `image/png`, `image/jpeg`) analizando la extensión del recurso solicitado. |
| **`include/files.h` / `src/files.c`** | Controla el acceso seguro al sistema de archivos dentro de `www/` usando `realpath()`, verifica existencia, permisos y tamaño (`Content-Length`), y transfiere los datos en bloques. |
| **`src/main.c`** | Valida el puerto, ignora `SIGPIPE`, crea la instancia de `epoll` y ejecuta el bucle de eventos para atender múltiples clientes y conexiones persistentes. |

---

## 4. Compilación y Construcción

El proyecto se compila con banderas de diagnóstico estricto (`-Wall -Wextra -std=c11 -D_DEFAULT_SOURCE -Iinclude`):

```bash
# Compilar el binario ejecutable
make

# Limpiar el binario generado
make clean
```

---

## 5. Despliegue y Ejecución

Por convención de la IANA, el protocolo HTTP opera sobre el **puerto 80**. Debido a que en sistemas POSIX los puertos inferiores a 1024 son catalogados como **privilegiados**, se requiere elevación de permisos (`sudo`). Opcionalmente se puede pasar otro puerto como argumento para pruebas sin `sudo`:

```bash
# Ejecución en el puerto estándar HTTP (80)
sudo ./minihttpd

# Ejecución en un puerto alternativo (sin sudo)
./minihttpd 8080
```

---

## 6. Verificación Funcional y Matriz de Códigos de Estado

La implementación evalúa y garantiza la emisión de respuestas bajo los siguientes códigos de estado de la RFC 7231 / 9110:

| Código HTTP | Significado | Condición de Disparo |
| :---: | :--- | :--- |
| **`200 OK`** | Petición exitosa | El recurso solicitado existe en `www/`, tiene permisos de lectura y se transmite correctamente. |
| **`400 Bad Request`** | Petición malformada | La trama enviada no cumple la sintaxis mínima de HTTP/1.1, falta el encabezado `Host`, la versión no es `HTTP/1.1`, o se excede el tamaño máximo de línea, encabezado o URI. |
| **`403 Forbidden`** | Acceso prohibido | Se detecta un intento de navegación fuera del directorio raíz (`../`) o acceso a recursos restringidos. |
| **`404 Not Found`** | Recurso no encontrado | La URI solicitada no coincide con ningún archivo dentro de `www/`. |
| **`405 Method Not Allowed`** | Método no soportado | El cliente invoca un verbo distinto a `GET` (por ejemplo, `POST`, `PUT`, `DELETE`). |
| **`500 Internal Server Error`** | Error interno del servidor | Fallos inesperados del sistema subyacente (por ejemplo, error en llamadas al sistema de I/O). |

### 6.1 Protocolo de Pruebas Funcionales

```bash
# 1. Petición GET estándar (200 OK)
curl -i http://localhost/index.html

# 2. Comprobación de recurso CSS (Content-Type: text/css)
curl -i http://localhost/style.css

# 3. Transferencia de binario PNG (Content-Type: image/png)
curl -i http://localhost/image.png --output /tmp/test_download.png

# 4. Solicitud de recurso inexistente (404 Not Found)
curl -i http://localhost/recurso_inexistente.html

# 5. Método no permitido (405 Method Not Allowed)
curl -i -X POST http://localhost/index.html
```

---

## 7. Pruebas de Seguridad y Robustez

### 7.1 Mitigación de Salto de Directorio (*Path Traversal*)
* **Vector de Ataque:** Secuencias relativas `../` o su codificación hexadecimal URL `%2e%2e%2f` con el fin de acceder a archivos confidenciales del sistema anfitrión (ej. `/etc/passwd`).
* **Verificación:**
  ```bash
  # Intento de escape canónico
  curl -i --path-as-is http://localhost/../../../../etc/passwd

  # Intento con evasión URL-encoded
  curl -i http://localhost/%2e%2e%2f%2e%2e%2fetc/passwd
  ```
* **Resultado Esperado:** Código `403 Forbidden` (o `404 Not Found`), bloqueando la resolución de ruta fuera de `www/`.

### 7.2 Inyección de Peticiones Corruptas (*Malformed Requests*)
* **Vector de Ataque:** Cargas útiles arbitrarias que buscan provocar fallos de segmentación (*Segmentation Fault*) al desreferenciar punteros nulos durante el *parsing*.
* **Verificación:**
  ```bash
  printf "CARGA_CORRUPTA_SIN_HTTP\r\n\r\n" | nc localhost 80
  ```
* **Resultado Esperado:** Código `400 Bad Request` y cierre ordenado de la conexión TCP, manteniendo el proceso del servidor activo.

### 7.3 Simulación de Fallo Interno de I/O (*Fault Injection*)
* **Vector:** Error al intentar resolver el directorio raíz `www/` con `realpath()`, simulando un fallo interno de acceso al sistema de archivos.
* **Verificación:**
  ```bash
  mv www www_bak
  curl -i http://localhost/index.html
  mv www_bak www
  ```
* **Resultado Esperado:** Código `500 Internal Server Error`, sin exposición de trazas ni rutas internas al cliente.

### 7.4 Prueba de Resistencia por Desbordamiento (*Buffer Overflow / DoS*)
* **Vector:** Envío de cadenas de URI sobredimensionadas para verificar la contención de los límites del búfer de lectura en memoria (`MAX_URI`, `MAX_LINE`, `BUFSZ`).
* **Verificación:**
  ```bash
  python3 -c "import urllib.request; urllib.request.urlopen('http://localhost/' + 'A'*5000)"
  ```
* **Resultado Esperado:** Retorno de código `400 Bad Request` (Python mostrará `urllib.error.HTTPError: HTTP Error 400: Bad Request`), conservando la estabilidad del servidor.
