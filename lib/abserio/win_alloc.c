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
#include "abserio.h"

//===--------------------------------------------------------------------------------------------------------------===//
//                                                 Estructuras de datos
//===--------------------------------------------------------------------------------------------------------------===//
// Estructura de datos interna
struct InternalRepresentation {
  HANDLE k_com;
  DCB *params;
};

#define IR(x) ((struct InternalRepresentation *) (x))
#define INT_INFO(x) IR((x)->_internal_info)

//===--------------------------------------------------------------------------------------------------------------===//
//                                           Implementación de la interfaz
//===--------------------------------------------------------------------------------------------------------------===//
gboolean set_baud_rate(glong baud_rate, struct AbstractSerialDevice **dev) {
  return FALSE;
}

glong get_baud_rate(struct AbstractSerialDevice **dev) {
  return -1;
}

gboolean set_parity_bit(gboolean bit_enable, gboolean odd_neven, struct AbstractSerialDevice **dev) {
  return FALSE;
}

gboolean get_parity_bit(struct AbstractSerialDevice **dev) {
  return FALSE;
}

gboolean get_parity_odd_neven(struct AbstractSerialDevice **dev) {
  return FALSE;
}

gboolean set_software_control_flow(gboolean bit_enable, struct AbstractSerialDevice **dev) {
  return FALSE;
}

gboolean get_software_control_flow(struct AbstractSerialDevice **dev) {
  return FALSE;
}

//===--------------------------------------------------------------------------------------------------------------===//
//                                          Funciones de control del puerto
//===--------------------------------------------------------------------------------------------------------------===//
void free_sources(struct AbstractSerialDevice **dev) {
  free(INT_INFO(*dev)->params);
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
    INT_INFO(*dev)->params = malloc(sizeof(DCB));

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
      if (k_hd==INVALID_HANDLE_VALUE) {
        // No se puede abrir el puerto COM
        _set_errno((int) GetLastError());
        free_sources(dev);
        return FALSE;
      }
      INT_INFO(*dev)->k_com = k_hd;
    } else {
      INT_INFO(*dev)->k_com = k_hd;
    }
    (INT_INFO(*dev)->params)->DCBlength = sizeof(DCB);
    if (GetCommState(INT_INFO(*dev)->k_com, INT_INFO(*dev)->params)) {
      // Configura las funciones del driver
      (*dev)->set_baud_rate = set_baud_rate;
      (*dev)->get_baud_rate = get_baud_rate;
      (*dev)->set_parity_bit = set_parity_bit;
      (*dev)->get_parity_bit = get_parity_bit;
      (*dev)->get_parity_odd_neven = get_parity_odd_neven;
      (*dev)->set_software_control_flow = set_software_control_flow;
      (*dev)->get_software_control_flow = get_software_control_flow;
    } else {
      // No se puede obtener información del puerto COM
      _set_errno((int) GetLastError());
      free_sources(dev);
      return FALSE;
    }
  }
}

void close_serial_port(struct AbstractSerialDevice **dev) {
  if (dev!=NULL && *dev!=NULL) {
    CloseHandle(INT_INFO(*dev)->k_com);
    free_sources(dev);
  }
}

