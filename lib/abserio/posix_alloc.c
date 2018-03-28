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

#define G_LOG_DOMAIN                    "PosixDriver"
#include "abserio.h"
#include <errno.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <unistd.h>

//===--------------------------------------------------------------------------------------------------------------===//
//                                                 Estructuras de datos
//===--------------------------------------------------------------------------------------------------------------===//
// Estructura de datos interna
struct InternalRepresentation {
  int kernel_fd;
  struct termios *options;
  GMutex read_lock;
  GMutex write_lock;
  GMutex access_lock;
  volatile atomic_bool open;
  struct timeval timeout;
};

#define IR(x)                           ((struct InternalRepresentation *) (x))
#define INT_INFO(x)                     IR((x)->_internal_info)
#define READ_LOCK                       &INT_INFO(*dev)->read_lock
#define WRITE_LOCK                      &INT_INFO(*dev)->write_lock
#define ACCESS_LOCK                     &INT_INFO(*dev)->access_lock
#define PRINT_ERRNO(x)                  x("Message: \'%s\'", g_strerror(errno))

//===--------------------------------------------------------------------------------------------------------------===//
//                                           Implementación de la interfaz
//===--------------------------------------------------------------------------------------------------------------===//
gboolean set_baud_rate(glong baud_rate, struct AbstractSerialDevice **dev) {
  // Obtener la información de TERMIOS
  g_mutex_lock(ACCESS_LOCK);
  tcgetattr(INT_INFO(*dev)->kernel_fd, INT_INFO(*dev)->options);
  long was_ispeed = cfgetispeed(INT_INFO(*dev)->options);
  long was_ospeed = cfgetospeed(INT_INFO(*dev)->options);
  // Configurar el baudrate
  if (cfsetispeed(INT_INFO(*dev)->options, (speed_t) baud_rate)==0) {
    if (cfsetospeed(INT_INFO(*dev)->options, (speed_t) baud_rate)==0) {
      // Aplica los cambios
      if (tcsetattr(INT_INFO(*dev)->kernel_fd, TCSANOW, INT_INFO(*dev)->options)==0) {
        g_mutex_unlock(ACCESS_LOCK);
        return TRUE;
      }
    }
  }
  g_critical("Unable to change baud rate to \'%lu\'. Restoring the original baud rate (I:%lu, O:%lu).",
             baud_rate,
             was_ispeed,
             was_ospeed);
  PRINT_ERRNO(g_critical);
  cfsetispeed(INT_INFO(*dev)->options, (speed_t) was_ispeed);
  cfsetospeed(INT_INFO(*dev)->options, (speed_t) was_ospeed);
  tcsetattr(INT_INFO(*dev)->kernel_fd, TCSANOW, INT_INFO(*dev)->options);
  g_mutex_unlock(ACCESS_LOCK);
  return FALSE;
}

glong get_baud_rate(struct AbstractSerialDevice **dev) {
  // Obtener la información de TERMIOS
  g_mutex_lock(ACCESS_LOCK);
  tcgetattr(INT_INFO(*dev)->kernel_fd, INT_INFO(*dev)->options);
  speed_t out_speed = cfgetospeed(INT_INFO(*dev)->options);
  speed_t in_speed = cfgetospeed(INT_INFO(*dev)->options);
  if (out_speed==in_speed) {
    g_mutex_unlock(ACCESS_LOCK);
    return out_speed;
  }
  g_critical("Unable to get baud rate info from OS.");
  PRINT_ERRNO(g_critical);
  g_mutex_unlock(ACCESS_LOCK);
  return -1;
}

gboolean set_parity_bit(gboolean bit_enable, gboolean odd_neven, struct AbstractSerialDevice **dev) {
  // Obtener la información de TERMIOS
  g_mutex_lock(ACCESS_LOCK);
  tcgetattr(INT_INFO(*dev)->kernel_fd, INT_INFO(*dev)->options);
  if (bit_enable) {
    (INT_INFO(*dev)->options)->c_cflag |= PARENB;
    (INT_INFO(*dev)->options)->c_iflag |= INPCK;
  } else {
    (INT_INFO(*dev)->options)->c_cflag &= ~PARENB;
    (INT_INFO(*dev)->options)->c_iflag &= ~INPCK;
  }
  if (tcsetattr(INT_INFO(*dev)->kernel_fd, TCSANOW, INT_INFO(*dev)->options)==0) {
    tcgetattr(INT_INFO(*dev)->kernel_fd, INT_INFO(*dev)->options);
    if (odd_neven==SET_PARITY_ODD) {
      (INT_INFO(*dev)->options)->c_cflag |= PARODD;
    } else {
      (INT_INFO(*dev)->options)->c_cflag &= ~PARODD;
    }
    if (tcsetattr(INT_INFO(*dev)->kernel_fd, TCSANOW, INT_INFO(*dev)->options)==0) {
      g_mutex_unlock(ACCESS_LOCK);
      return TRUE;
    }
  }
  g_critical("Unable to set parity bits configuration. Won't restore original.");
  PRINT_ERRNO(g_critical);
  g_mutex_unlock(ACCESS_LOCK);
  return FALSE;
}

gboolean get_parity_bit(struct AbstractSerialDevice **dev) {
  // Obtener la información de TERMIOS
  g_mutex_lock(ACCESS_LOCK);
  tcgetattr(INT_INFO(*dev)->kernel_fd, INT_INFO(*dev)->options);
  g_mutex_unlock(ACCESS_LOCK);
  return (gboolean) ((INT_INFO(*dev)->options)->c_cflag & PARENB);
}

gboolean get_parity_odd_neven(struct AbstractSerialDevice **dev) {
  // Obtener la información de TERMIOS
  g_mutex_lock(ACCESS_LOCK);
  tcgetattr(INT_INFO(*dev)->kernel_fd, INT_INFO(*dev)->options);
  g_mutex_unlock(ACCESS_LOCK);
  return (gboolean) ((INT_INFO(*dev)->options)->c_cflag & PARODD);
}

gboolean set_software_control_flow(gboolean bit_enable, struct AbstractSerialDevice **dev) {
  // Obtener la información de TERMIOS
  g_mutex_lock(ACCESS_LOCK);
  tcgetattr(INT_INFO(*dev)->kernel_fd, INT_INFO(*dev)->options);
  if (bit_enable) {
    (INT_INFO(*dev)->options)->c_iflag |= (IXON | IXOFF | IXANY);
  } else {
    (INT_INFO(*dev)->options)->c_iflag &= ~(IXON | IXOFF | IXANY);
  }
  gboolean res = tcsetattr(INT_INFO(*dev)->kernel_fd, TCSANOW, INT_INFO(*dev)->options)==0;
  if (res) {
    g_mutex_unlock(ACCESS_LOCK);
    return TRUE;
  }
  g_critical("Unable to set software control configuration. Won't restore original.");
  PRINT_ERRNO(g_critical);
  g_mutex_unlock(ACCESS_LOCK);
  return FALSE;
}

gboolean get_software_control_flow(struct AbstractSerialDevice **dev) {
  // Obtener la información de TERMIOS
  g_mutex_lock(ACCESS_LOCK);
  tcgetattr(INT_INFO(*dev)->kernel_fd, INT_INFO(*dev)->options);
  g_mutex_unlock(ACCESS_LOCK);
  return (gboolean) ((INT_INFO(*dev)->options)->c_iflag & IXON);
}

gboolean write_byte(gchar byte, struct AbstractSerialDevice **dev) {
  gboolean isReading = !(g_mutex_trylock(READ_LOCK));
  ssize_t n;
  if (isReading) {
    g_mutex_lock(WRITE_LOCK);
    g_debug("Performing \'write\' operation while a \'read\' operation is running.");
    n = write(INT_INFO(*dev)->kernel_fd, &byte, 1);
    g_mutex_unlock(WRITE_LOCK);
  } else {
    // Debido a que trylock realmente bloquea el mutex, hay que desbloquearlo. De otra forma, se tiene un lindo death
    // lock
    g_mutex_unlock(READ_LOCK);
    g_mutex_lock(ACCESS_LOCK);
    g_mutex_lock(WRITE_LOCK);
    n = write(INT_INFO(*dev)->kernel_fd, &byte, 1);
    g_mutex_unlock(WRITE_LOCK);
    g_mutex_unlock(ACCESS_LOCK);
  }
  if (n==1) {
    return TRUE;
  }
  g_critical("Returning with an invalid number of bytes sent. Expected %d, sent %d", 1, (int) n);
  PRINT_ERRNO(g_critical);
  return FALSE;
}

char read_byte(struct AbstractSerialDevice **dev) {
  ssize_t r;
  fd_set set;
  do {
    if (*dev==NULL) {
      g_debug("Read operation cancelled: resource not available.");
      errno = ECANCELED;
      return -1;
    }
    g_mutex_lock(ACCESS_LOCK);
    if (INT_INFO(*dev)->open==FALSE) {
      g_mutex_unlock(ACCESS_LOCK);
      g_debug("Read operation cancelled: file is closed.");
      errno = ECANCELED;
      return -1;
    }
    FD_ZERO(&set);
    FD_SET(INT_INFO(*dev)->kernel_fd, &set);
    g_mutex_lock(READ_LOCK);
    select(INT_INFO(*dev)->kernel_fd + 1, &set, NULL, NULL, &INT_INFO(*dev)->timeout);
    g_mutex_unlock(READ_LOCK);
    g_mutex_unlock(ACCESS_LOCK);
    g_thread_yield();
  } while (!FD_ISSET(INT_INFO(*dev)->kernel_fd, &set));
  char oneByte;
  g_mutex_lock(ACCESS_LOCK);
  g_mutex_lock(READ_LOCK);
  r = read(INT_INFO(*dev)->kernel_fd, &oneByte, 1);
  g_mutex_unlock(READ_LOCK);
  g_mutex_unlock(ACCESS_LOCK);
  if (r==1) {
    g_debug("Returning from blocking-read, read '%d' from port.", oneByte);
    return oneByte;
  }
  return (char) -1;
}

//===--------------------------------------------------------------------------------------------------------------===//
//                                          Funciones de control del puerto
//===--------------------------------------------------------------------------------------------------------------===//
void free_sources(struct AbstractSerialDevice **dev) {
  g_mutex_clear(ACCESS_LOCK);
  g_mutex_clear(READ_LOCK);
  g_mutex_clear(WRITE_LOCK);
  g_debug("Freeing driver resources for Kernel File Descriptor %d.", INT_INFO(*dev)->kernel_fd);
  free(INT_INFO(*dev)->options);
  free(INT_INFO(*dev));
  free(*dev);
  g_debug("Driver deallocate: setting driver pointer to NULL.");
  *dev = NULL;
}

gboolean open_serial_port(struct AbstractSerialDevice **dev, GString *os_dev) {
  if (dev==NULL || *dev!=NULL) {
    // Si no es NULL, podemos estar cayendo encima de un driver reservado que ya no se podrá liberar.
    g_error("Trying to allocate a driver in a pointer which is not NULL. This is considered a bug.");
    return FALSE;
  }
  if (os_dev!=NULL && os_dev->str!=NULL) {
    // Reservar memoria para el driver abstracto
    *dev = malloc(sizeof(struct AbstractSerialDevice));
    (*dev)->_internal_info = malloc(sizeof(struct InternalRepresentation));
    INT_INFO(*dev)->options = malloc(sizeof(struct termios));

    // Inicializar los mutex
    g_mutex_init(ACCESS_LOCK);
    g_mutex_init(READ_LOCK);
    g_mutex_init(WRITE_LOCK);
    INT_INFO(*dev)->open = TRUE;

    int k_fd = open(os_dev->str, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (k_fd==-1) {
      free_sources(dev);
      return FALSE;
    }
    g_mutex_lock(ACCESS_LOCK);
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
    (*dev)->write_byte = write_byte;
    (*dev)->read_byte = read_byte;

    // Timeout
    INT_INFO(*dev)->timeout.tv_sec = 0;
    // 60 veces por segundo
    INT_INFO(*dev)->timeout.tv_usec = (1000000/60);
    g_debug("Driver polling timeout: %.6f.",
            INT_INFO(*dev)->timeout.tv_sec + (INT_INFO(*dev)->timeout.tv_usec/1000000.0));

    g_mutex_unlock(ACCESS_LOCK);
    g_debug("Successfully created a driver for the file \'%s\' (Kernel File Descriptor: %d).",
            os_dev->str,
            INT_INFO(*dev)->kernel_fd);
    return TRUE;
  }
  return FALSE;
}

void close_serial_port(struct AbstractSerialDevice **dev) {
  if (dev!=NULL && *dev!=NULL) {
    g_mutex_lock(ACCESS_LOCK);
    INT_INFO(*dev)->open = FALSE;
    close(INT_INFO(*dev)->kernel_fd);
    g_mutex_unlock(ACCESS_LOCK);
    g_debug("Kernel File Descriptor %d closed. The driver will be unlocked and this thread will yield.",
            INT_INFO(*dev)->kernel_fd);
    g_thread_yield();
    g_mutex_lock(ACCESS_LOCK);
    g_debug(
        "Thread returned from yield for Kernel File Descriptor %d. The driver will be permanently locked and freed.",
        INT_INFO(*dev)->kernel_fd);
    free_sources(dev);
  }
}

