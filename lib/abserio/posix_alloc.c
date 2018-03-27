//===-- lib/abserio/abserio.h - Driver serial (versión POSIX) ---------------------------------------------*- C -*-===//
//
// Copyright (c) 2018 Oever González
//
//  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
//                                 the License. You may obtain a copy of the License at
//
//                                      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software  distributed under the License is distributed on
//  an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
//                    specific language governing permissions and limitations under the License.
//
//===--------------------------------------------------------------------------------------------------------------===//
///
/// Este bloque de código es específico de POSIX. Abre, cierra, configura y usa el puerto serial de acuerdo a la
/// especificación POSIX.
///
/// https://www.cmrr.umn.edu/~strupp/serial.html#2_5_2
///
//===--------------------------------------------------------------------------------------------------------------===//

#include "abserio.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

//===--------------------------------------------------------------------------------------------------------------===//
//                                                 Estructuras de datos
//===--------------------------------------------------------------------------------------------------------------===//

// Estructura de datos interna
struct InternalRepresentation {
  int kernel_fd;
  struct termios *options;
};

#define IR(x) ((struct InternalRepresentation *) (x))
#define INT_INFO(x) IR((x)->_internal_info)

//===--------------------------------------------------------------------------------------------------------------===//
//                                           Implementación de la interfaz
//===--------------------------------------------------------------------------------------------------------------===//



gboolean set_baud_rate(glong baud_rate, struct AbstractSerialDevice **dev) {
  // Obtener la información de TERMIOS
  tcgetattr(INT_INFO(*dev)->kernel_fd, INT_INFO(*dev)->options);
  // Configurar el baudrate
  if (cfsetispeed(INT_INFO(*dev)->options, (speed_t) baud_rate)==0) {
    if (cfsetospeed(INT_INFO(*dev)->options, (speed_t) baud_rate)==0) {
      // Aplica los cambios
      if (tcsetattr(INT_INFO(*dev)->kernel_fd, TCSANOW, INT_INFO(*dev)->options)==0) {
        return TRUE;
      }
      return FALSE;
    }
    return FALSE;
  }
  return FALSE;
}

//===--------------------------------------------------------------------------------------------------------------===//
//                                          Funciones de control del puerto
//===--------------------------------------------------------------------------------------------------------------===//

void free_sources(struct AbstractSerialDevice **dev) {
  free(INT_INFO(*dev)->options);
  free(INT_INFO(*dev));
  free(*dev);
  *dev = NULL;
}

bool open_serial_port(struct AbstractSerialDevice **dev, GString *os_dev) {
  if (dev==NULL || *dev!=NULL) {
    // Si no es NULL, podemos estar cayendo encima de un driver reservado que ya no se podrá liberar.
    return FALSE;
  }
  if (os_dev!=NULL && os_dev->str!=NULL) {
    // Reservar memoria para el driver abstracto
    *dev = malloc(sizeof(struct AbstractSerialDevice));
    (*dev)->_internal_info = malloc(sizeof(struct InternalRepresentation));
    INT_INFO(*dev)->options = malloc(sizeof(struct termios));

    int k_fd = open(os_dev->str, O_RDWR | O_NOCTTY | O_NDELAY);
    if (k_fd==-1) {
      free_sources(dev);
      return false;
    }
    // Resetear todas las banderas para este archivo
    fcntl(k_fd, F_SETFL, 0);
    // Guardar el FD en la IR
    INT_INFO(*dev)->kernel_fd = k_fd;
    // Obtener la información de TERMIOS
    tcgetattr(k_fd, INT_INFO(*dev)->options);
    // Línea local y activa el receptor
    (INT_INFO(*dev)->options)->c_cflag |= (CLOCAL | CREAD);
    // Aplica los cambios
    tcsetattr(k_fd, TCSANOW, INT_INFO(*dev)->options);

    // Configura las funciones del driver
    (*dev)->set_baud_rate = set_baud_rate;

    return TRUE;
  }
  return false;
}

void close_serial_port(struct AbstractSerialDevice **dev) {
  if (dev!=NULL && *dev!=NULL) {
    close(INT_INFO(*dev)->kernel_fd);
    free_sources(dev);
  }
}

