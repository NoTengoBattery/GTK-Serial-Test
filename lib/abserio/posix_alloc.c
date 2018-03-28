//===-- lib/abserio/posix_alloc.c - Driver serial (versión POSIX) -----------------------------------------*- C -*-===//
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
  long was_ispeed = cfgetispeed(INT_INFO(*dev)->options);
  long was_ospeed = cfgetospeed(INT_INFO(*dev)->options);
  // Configurar el baudrate
  if (cfsetispeed(INT_INFO(*dev)->options, (speed_t) baud_rate)==0) {
    if (cfsetospeed(INT_INFO(*dev)->options, (speed_t) baud_rate)==0) {
      // Aplica los cambios
      if (tcsetattr(INT_INFO(*dev)->kernel_fd, TCSANOW, INT_INFO(*dev)->options)==0) {
        return TRUE;
      }
    }
  }
  cfsetispeed(INT_INFO(*dev)->options, (speed_t) was_ispeed);
  cfsetospeed(INT_INFO(*dev)->options, (speed_t) was_ospeed);
  tcsetattr(INT_INFO(*dev)->kernel_fd, TCSANOW, INT_INFO(*dev)->options);
  return FALSE;
}

glong get_baud_rate(struct AbstractSerialDevice **dev) {
  // Obtener la información de TERMIOS
  tcgetattr(INT_INFO(*dev)->kernel_fd, INT_INFO(*dev)->options);
  speed_t out_speed = cfgetospeed(INT_INFO(*dev)->options);
  speed_t in_speed = cfgetospeed(INT_INFO(*dev)->options);
  if (out_speed==in_speed) {
    return out_speed;
  }
  set_baud_rate(out_speed, dev);
  return -1;
}

gboolean set_parity_bit(gboolean bit_enable, gboolean odd_neven, struct AbstractSerialDevice **dev) {
  // Obtener la información de TERMIOS
  tcgetattr(INT_INFO(*dev)->kernel_fd, INT_INFO(*dev)->options);
  if (bit_enable) {
    (INT_INFO(*dev)->options)->c_cflag |= PARENB;
    (INT_INFO(*dev)->options)->c_iflag |= (INPCK | ISTRIP);
  } else {
    (INT_INFO(*dev)->options)->c_cflag &= ~PARENB;
  }
  if (tcsetattr(INT_INFO(*dev)->kernel_fd, TCSANOW, INT_INFO(*dev)->options)==0) {
    tcgetattr(INT_INFO(*dev)->kernel_fd, INT_INFO(*dev)->options);
    if (odd_neven==PARITY_ODD) {
      (INT_INFO(*dev)->options)->c_cflag |= PARODD;
    } else {
      (INT_INFO(*dev)->options)->c_cflag &= ~PARODD;
    }
    if (tcsetattr(INT_INFO(*dev)->kernel_fd, TCSANOW, INT_INFO(*dev)->options)==0) {
      return TRUE;
    }
  }
  return FALSE;
}

gboolean get_parity_bit(struct AbstractSerialDevice **dev) {
  // Obtener la información de TERMIOS
  tcgetattr(INT_INFO(*dev)->kernel_fd, INT_INFO(*dev)->options);
  return (gboolean) ((INT_INFO(*dev)->options)->c_cflag & PARENB);
}

gboolean get_parity_odd_neven(struct AbstractSerialDevice **dev) {
  // Obtener la información de TERMIOS
  tcgetattr(INT_INFO(*dev)->kernel_fd, INT_INFO(*dev)->options);
  return (gboolean) ((INT_INFO(*dev)->options)->c_cflag & PARODD);
}

gboolean set_software_control_flow(gboolean bit_enable, struct AbstractSerialDevice **dev) {
  // Obtener la información de TERMIOS
  tcgetattr(INT_INFO(*dev)->kernel_fd, INT_INFO(*dev)->options);
  if (bit_enable) {
    (INT_INFO(*dev)->options)->c_iflag |= (IXON | IXOFF | IXANY);
  } else {
    (INT_INFO(*dev)->options)->c_iflag &= ~(IXON | IXOFF | IXANY);
  }
  if (tcsetattr(INT_INFO(*dev)->kernel_fd, TCSANOW, INT_INFO(*dev)->options)==0) {
    return TRUE;
  }
  return FALSE;
}

gboolean get_software_control_flow(struct AbstractSerialDevice **dev) {
  // Obtener la información de TERMIOS
  tcgetattr(INT_INFO(*dev)->kernel_fd, INT_INFO(*dev)->options);
  return (gboolean) ((INT_INFO(*dev)->options)->c_iflag & IXON);
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

gboolean open_serial_port(struct AbstractSerialDevice **dev, GString *os_dev) {
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
      return FALSE;
    }
    // Resetear todas las banderas para este archivo
    fcntl(k_fd, F_SETFL, 0);
    // Guardar el FD en la IR
    INT_INFO(*dev)->kernel_fd = k_fd;
    // Obtener la información de TERMIOS
    tcgetattr(k_fd, INT_INFO(*dev)->options);
    // Línea local y activa el receptor
    (INT_INFO(*dev)->options)->c_cflag |= (CLOCAL | CREAD);
    // Desactivar la entrada canónica (de todas formas, Windows no la soporta)
    (INT_INFO(*dev)->options)->c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    // La salida canónica (preprocesada) tampoco hace sentido
    (INT_INFO(*dev)->options)->c_oflag &= ~OPOST;
    // Aplica los cambios
    tcsetattr(k_fd, TCSANOW, INT_INFO(*dev)->options);

    // Configura las funciones del driver
    (*dev)->set_baud_rate = set_baud_rate;
    (*dev)->get_baud_rate = get_baud_rate;
    (*dev)->set_parity_bit = set_parity_bit;
    (*dev)->get_parity_bit = get_parity_bit;
    (*dev)->get_parity_odd_neven = get_parity_odd_neven;
    (*dev)->set_software_control_flow = set_software_control_flow;
    (*dev)->get_software_control_flow = get_software_control_flow;

    return TRUE;
  }
  return FALSE;
}

void close_serial_port(struct AbstractSerialDevice **dev) {
  if (dev!=NULL && *dev!=NULL) {
    close(INT_INFO(*dev)->kernel_fd);
    free_sources(dev);
  }
}

