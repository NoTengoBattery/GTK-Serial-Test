//===-- lib/abserio/win_alloc.c - Driver serial (versión Win32) -------------------------------------------*- C -*-===//
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
/// Este bloque de código es específico de Windows. Abre, cierra, configura y usa el puerto serial de acuerdo a la
/// API de Win32.
///
/// http://xanthium.in/Serial-Port-Programming-using-Win32-API
/// https://msdn.microsoft.com/en-us/library/windows/desktop/aa363214(v=vs.85).aspx
///
//===--------------------------------------------------------------------------------------------------------------===//


#define G_LOG_DOMAIN                    "Win32AbSerIO"
#include "abserio.h"
#include <errno.h>
#include <stdatomic.h>

//===--------------------------------------------------------------------------------------------------------------===//
//                                                 Estructuras de datos
//===--------------------------------------------------------------------------------------------------------------===//
// Estructura de datos interna
struct InternalRepresentation {
  HANDLE k_com;
  DCB *params;
  GMutex read_lock;
  GMutex write_lock;
  GMutex access_lock;
  volatile atomic_bool open;
  COMMTIMEOUTS *tout;
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
gboolean set_baud_rate(glong baud_rate, const struct AbstractSerialDevice **cdev) {
  struct AbstractSerialDevice **dev = (struct AbstractSerialDevice **) cdev;
  g_mutex_lock(ACCESS_LOCK);
  long was_ospeed;
  if (GetCommState(INT_INFO(*dev)->k_com, INT_INFO(*dev)->params)) {
    was_ospeed = (INT_INFO(*dev)->params)->BaudRate;
    (INT_INFO(*dev)->params)->BaudRate = (DWORD) baud_rate;
    gboolean eval = SetCommState(INT_INFO(*dev)->k_com, INT_INFO(*dev)->params);
    errno = (int) (GetLastError()!=0 ? GetLastError() : (DWORD) errno);
    if (!eval) {
      g_critical("Unable to change baud rate to \'%lu\'. Restoring the original baud rate (%lu).",
                 baud_rate,
                 was_ospeed);
      PRINT_ERRNO(g_critical);
      (INT_INFO(*dev)->params)->BaudRate = (DWORD) was_ospeed;
      SetCommState(INT_INFO(*dev)->k_com, INT_INFO(*dev)->params);
      g_mutex_unlock(ACCESS_LOCK);
      return FALSE;
    }
    g_mutex_unlock(ACCESS_LOCK);
    return TRUE;
  }
  errno = (int) (GetLastError()!=0 ? GetLastError() : (DWORD) errno);
  g_critical("Unable to read port information.");
  PRINT_ERRNO(g_critical);
  g_mutex_unlock(ACCESS_LOCK);
  return FALSE;
}

glong get_baud_rate(const struct AbstractSerialDevice **cdev) {
  struct AbstractSerialDevice **dev = (struct AbstractSerialDevice **) cdev;
  g_mutex_lock(ACCESS_LOCK);
  if (GetCommState(INT_INFO(*dev)->k_com, INT_INFO(*dev)->params)) {
    errno = (int) (GetLastError()!=0 ? GetLastError() : (DWORD) errno);
    g_mutex_unlock(ACCESS_LOCK);
    return (INT_INFO(*dev)->params)->BaudRate;
  }
  errno = (int) (GetLastError()!=0 ? GetLastError() : (DWORD) errno);
  g_critical("Unable to read port information.");
  PRINT_ERRNO(g_critical);
  g_mutex_unlock(ACCESS_LOCK);
  return -1;
}

gboolean set_parity_bit(gboolean bit_enable, gboolean odd_neven, const struct AbstractSerialDevice **cdev) {
  struct AbstractSerialDevice **dev = (struct AbstractSerialDevice **) cdev;
  g_mutex_lock(ACCESS_LOCK);
  if (GetCommState(INT_INFO(*dev)->k_com, INT_INFO(*dev)->params)) {
    (INT_INFO(*dev)->params)->fParity = (DWORD) bit_enable;
    (INT_INFO(*dev)->params)->Parity = (BYTE) (bit_enable ? (BYTE) (odd_neven ? ODDPARITY : EVENPARITY) : NOPARITY);
    gboolean eval = SetCommState(INT_INFO(*dev)->k_com, INT_INFO(*dev)->params);
    errno = (int) (GetLastError()!=0 ? GetLastError() : (DWORD) errno);
    if (!eval) {
      g_critical("Unable to set parity bits configuration. Won't restore original.");
      PRINT_ERRNO(g_critical);
      g_mutex_unlock(ACCESS_LOCK);
      return FALSE;
    }
    g_mutex_unlock(ACCESS_LOCK);
    return TRUE;
  }
  errno = (int) (GetLastError()!=0 ? GetLastError() : (DWORD) errno);
  g_critical("Unable to read port information.");
  PRINT_ERRNO(g_critical);
  g_mutex_unlock(ACCESS_LOCK);
  return FALSE;
}

gboolean get_parity_bit(const struct AbstractSerialDevice **cdev) {
  struct AbstractSerialDevice **dev = (struct AbstractSerialDevice **) cdev;
  g_mutex_lock(ACCESS_LOCK);
  if (GetCommState(INT_INFO(*dev)->k_com, INT_INFO(*dev)->params)) {
    errno = (int) (GetLastError()!=0 ? GetLastError() : (DWORD) errno);
    g_mutex_unlock(ACCESS_LOCK);
    return (gboolean) (INT_INFO(*dev)->params)->fParity;
  }
  errno = (int) (GetLastError()!=0 ? GetLastError() : (DWORD) errno);
  g_critical("Unable to read port information.");
  PRINT_ERRNO(g_critical);
  g_mutex_unlock(ACCESS_LOCK);
  return FALSE;
}

gboolean get_parity_odd_neven(const struct AbstractSerialDevice **cdev) {
  struct AbstractSerialDevice **dev = (struct AbstractSerialDevice **) cdev;
  g_mutex_lock(ACCESS_LOCK);
  if (GetCommState(INT_INFO(*dev)->k_com, INT_INFO(*dev)->params)) {
    errno = (int) (GetLastError()!=0 ? GetLastError() : (DWORD) errno);
    g_mutex_unlock(ACCESS_LOCK);
    return (INT_INFO(*dev)->params)->Parity==ODDPARITY ? TRUE : FALSE;
  }
  errno = (int) (GetLastError()!=0 ? GetLastError() : (DWORD) errno);
  g_critical("Unable to read port information.");
  PRINT_ERRNO(g_critical);
  g_mutex_unlock(ACCESS_LOCK);
  return FALSE;
}

gboolean set_software_control_flow(gboolean bit_enable, const struct AbstractSerialDevice **cdev) {
  struct AbstractSerialDevice **dev = (struct AbstractSerialDevice **) cdev;
  g_mutex_lock(ACCESS_LOCK);
  if (GetCommState(INT_INFO(*dev)->k_com, INT_INFO(*dev)->params)) {
    if (bit_enable) {
      (INT_INFO(*dev)->params)->fOutX = TRUE;
      (INT_INFO(*dev)->params)->fInX = TRUE;
    } else {
      (INT_INFO(*dev)->params)->fOutX = FALSE;
      (INT_INFO(*dev)->params)->fInX = FALSE;
    }
    gboolean eval = SetCommState(INT_INFO(*dev)->k_com, INT_INFO(*dev)->params);
    errno = (int) (GetLastError()!=0 ? GetLastError() : (DWORD) errno);
    if (!eval) {
      g_critical("Unable to set software control configuration. Won't restore original.");
      PRINT_ERRNO(g_critical);
      g_mutex_unlock(ACCESS_LOCK);
      return FALSE;
    }
    g_mutex_unlock(ACCESS_LOCK);
    return TRUE;
  }
  errno = (int) (GetLastError()!=0 ? GetLastError() : (DWORD) errno);
  g_critical("Unable to read port information.");
  PRINT_ERRNO(g_critical);
  g_mutex_unlock(ACCESS_LOCK);
  return FALSE;
}

gboolean get_software_control_flow(const struct AbstractSerialDevice **cdev) {
  struct AbstractSerialDevice **dev = (struct AbstractSerialDevice **) cdev;
  g_mutex_lock(ACCESS_LOCK);
  if (GetCommState(INT_INFO(*dev)->k_com, INT_INFO(*dev)->params)) {
    errno = (int) (GetLastError()!=0 ? GetLastError() : (DWORD) errno);
    g_mutex_unlock(ACCESS_LOCK);
    return (gboolean) (INT_INFO(*dev)->params)->fInX;
  }
  errno = (int) (GetLastError()!=0 ? GetLastError() : (DWORD) errno);
  g_critical("Unable to read port information.");
  PRINT_ERRNO(g_critical);
  g_mutex_unlock(ACCESS_LOCK);
  return FALSE;
}

gboolean write_byte(gchar byte, const struct AbstractSerialDevice **cdev) {
  struct AbstractSerialDevice **dev = (struct AbstractSerialDevice **) cdev;
  gboolean isReading = !(g_mutex_trylock(READ_LOCK));
  DWORD n;
  if (isReading) {
    g_mutex_lock(WRITE_LOCK);
    g_debug("Performing \'write\' operation while a \'read\' operation is running.");
    WriteFile(INT_INFO(*dev)->k_com, &byte, 1, &n, NULL);
    g_mutex_unlock(WRITE_LOCK);
  } else {
    // Debido a que trylock realmente bloquea el mutex, hay que desbloquearlo. De otra forma, se tiene un lindo death
    // lock
    g_mutex_unlock(READ_LOCK);
    g_mutex_lock(ACCESS_LOCK);
    g_mutex_lock(WRITE_LOCK);
    WriteFile(INT_INFO(*dev)->k_com, &byte, 1, &n, NULL);
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

char read_byte(const struct AbstractSerialDevice **cdev) {
  struct AbstractSerialDevice **dev = (struct AbstractSerialDevice **) cdev;
  DWORD n;
  gchar readed;
  do {
    if (*dev==NULL) {
      g_debug("Read operation cancelled: resource not available.");
      errno = ECANCELED;
      return -1;
    }
    g_mutex_lock(ACCESS_LOCK);
    if (INT_INFO(*dev)->open==FALSE) {
      g_mutex_unlock(ACCESS_LOCK);
      g_debug("Read operation cancelled: file HANDLE is closed.");
      errno = ECANCELED;
      return -1;
    }
    g_thread_yield();
    g_mutex_lock(READ_LOCK);
    ReadFile(INT_INFO(*dev)->k_com, &readed, 1, &n, NULL);
    g_mutex_unlock(READ_LOCK);
    g_mutex_unlock(ACCESS_LOCK);
    g_thread_yield();
  } while (n!=1);
  g_debug("Returning from blocking-read, read '%d' from port.", readed);
  return readed;
}

//===--------------------------------------------------------------------------------------------------------------===//
//                                          Funciones de control del puerto
//===--------------------------------------------------------------------------------------------------------------===//
void free_sources(struct AbstractSerialDevice **dev) {
  g_mutex_clear(ACCESS_LOCK);
  g_mutex_clear(READ_LOCK);
  g_mutex_clear(WRITE_LOCK);
  g_debug("Freeing driver resources for HANDLE %d.", INT_INFO(*dev)->k_com);
  free(INT_INFO(*dev)->params);
  free(INT_INFO(*dev)->tout);
  free(INT_INFO(*dev));
  free(*dev);
  g_debug("Driver deallocate: setting driver pointer to NULL.");
  *dev = NULL;
}

gboolean open_serial_port(const struct AbstractSerialDevice **cdev, GString *os_dev) {
  struct AbstractSerialDevice **dev = (struct AbstractSerialDevice **) cdev;
  if (dev==NULL || *dev!=NULL) {
    // Si no es NULL, podemos estar cayendo encima de un driver reservado que ya no se podrá liberar.
    g_error("Trying to allocate a driver in a pointer which is not NULL. This is considered a bug.");
    return FALSE;
  }
  if (os_dev!=NULL && os_dev->str!=NULL) {
    // Reservar memoria para el driver abstracto
    *dev = malloc(sizeof(struct AbstractSerialDevice));
    (*dev)->_internal_info = malloc(sizeof(struct InternalRepresentation));
    INT_INFO(*dev)->params = malloc(sizeof(DCB));
    INT_INFO(*dev)->tout = malloc(sizeof(COMMTIMEOUTS));
    // Inicializar los mutex
    g_mutex_init(ACCESS_LOCK);
    g_mutex_init(READ_LOCK);
    g_mutex_init(WRITE_LOCK);

    // Intentar abrir el puerto directamente. p.e. COM1
    HANDLE k_hd = CreateFile(os_dev->str,
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL);
    if (k_hd==INVALID_HANDLE_VALUE) {
      // Intentar abrir el puerto desde \\.\COM1
      g_string_prepend(os_dev, "\\\\.\\");
      k_hd = CreateFile(os_dev->str,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);
      errno = EINVAL;
      if (k_hd==INVALID_HANDLE_VALUE) {
        // No se puede abrir el puerto COM
        g_critical("Unable to open the COM file.");
        g_critical("Message from Windows: \'%s\'", g_strerror((int) GetLastError()));
        free_sources(dev);
        return FALSE;
      }
      INT_INFO(*dev)->k_com = k_hd;
    } else {
      INT_INFO(*dev)->k_com = k_hd;
    }
    g_mutex_lock(ACCESS_LOCK);
    (INT_INFO(*dev)->params)->DCBlength = sizeof(DCB);
    if (GetCommState(INT_INFO(*dev)->k_com, INT_INFO(*dev)->params)) {
      INT_INFO(*dev)->open = TRUE;
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

      // Configuracion inicial
      (INT_INFO(*dev)->params)->ByteSize = 0x08;
      (INT_INFO(*dev)->params)->fParity = TRUE;
      (INT_INFO(*dev)->params)->Parity = ODDPARITY;
      SetCommState(INT_INFO(*dev)->k_com, INT_INFO(*dev)->params);

      // Timeout
      GetCommTimeouts(INT_INFO(*dev)->k_com, INT_INFO(*dev)->tout);
      (INT_INFO(*dev)->tout)->ReadIntervalTimeout = MAXDWORD;
      (INT_INFO(*dev)->tout)->ReadTotalTimeoutMultiplier = MAXDWORD;
      (INT_INFO(*dev)->tout)->ReadTotalTimeoutConstant = (1000/60);
      SetCommTimeouts(INT_INFO(*dev)->k_com, INT_INFO(*dev)->tout);
      GetCommTimeouts(INT_INFO(*dev)->k_com, INT_INFO(*dev)->tout);
      g_debug("Driver polling timeout: %.6f.",
              ((INT_INFO(*dev)->tout)->ReadTotalTimeoutConstant)/1000.0);

      g_mutex_unlock(ACCESS_LOCK);
      g_debug("Successfully created a driver for the file \'%s\' (HANDLE: %d).",
              os_dev->str,
              INT_INFO(*dev)->k_com);
      return TRUE;
    }
    // No se puede obtener información del puerto COM
    g_mutex_unlock(ACCESS_LOCK);
    errno = EINVAL;
    g_critical("Unable to read port information.");
    PRINT_ERRNO(g_critical);
    free_sources(dev);
    return FALSE;
  }
}

void close_serial_port(const struct AbstractSerialDevice **cdev) {
  struct AbstractSerialDevice **dev = (struct AbstractSerialDevice **) cdev;
  if (dev!=NULL && *dev!=NULL) {
    g_mutex_lock(ACCESS_LOCK);
    INT_INFO(*dev)->open = FALSE;
    CloseHandle(INT_INFO(*dev)->k_com);
    g_mutex_unlock(ACCESS_LOCK);
    g_debug("HANDLE %d closed. The driver will be unlocked and this thread will yield.",
            INT_INFO(*dev)->k_com);
    g_thread_yield();
    g_mutex_lock(ACCESS_LOCK);
    g_debug(
        "Thread returned from yield for HANDLE %d. The driver will be permanently locked and freed.",
        INT_INFO(*dev)->k_com);
    free_sources(dev);
  }
}

