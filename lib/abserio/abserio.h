//===-- lib/abserio/abserio.h - Interfaz para AbSerIO -----------------------------------------------------*- C -*-===//
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
/// Esta es la interfaz para la biblioteca AbSerIO.
///
//===--------------------------------------------------------------------------------------------------------------===//


#include <glib.h>

#ifdef _WIN32
#include <windows.h>
#define STD_BAUD_110                    CBR_110
#define STD_BAUD_300                    CBR_300
#define STD_BAUD_600                    CBR_600
#define STD_BAUD_1200                   CBR_1200
#define STD_BAUD_2400                   CBR_2400
#define STD_BAUD_4800                   CBR_4800
#define STD_BAUD_9600                   CBR_9600
#define STD_BAUD_14400                  CBR_14400
#define STD_BAUD_19200                  CBR_19200
#define STD_BAUD_38400                  CBR_38400
#define STD_BAUD_57600                  CBR_57600
#define STD_BAUD_115200                 CBR_115200
#define STD_BAUD_128000                 CBR_128000
#define STD_BAUD_256000                 CBR_256000
#else
#include <termios.h>
#define STD_BAUD_110                    B110
#define STD_BAUD_300                    B300
#define STD_BAUD_600                    B600
#define STD_BAUD_1200                   B1200
#define STD_BAUD_2400                   B2400
#define STD_BAUD_4800                   B4800
#define STD_BAUD_9600                   B9600
#define STD_BAUD_14400                  B14400
#define STD_BAUD_19200                  B19200
#define STD_BAUD_38400                  B38400
#define STD_BAUD_57600                  B57600
#define STD_BAUD_115200                 B115200
#endif // _WIN32

#define BAUDS_AVAIL                     15
#define PARITY_ODD                      TRUE
#define PARITY_EVEN                     FALSE
#define PARITY_ENABLE                   TRUE
#define PARITY_DISABLE                  FALSE
extern const long bauds[BAUDS_AVAIL];

// Esta interfaz representa un dispositivo serial abstracto. Contiene funciones y propiedades del dispositivo serial.
struct AbstractSerialDevice {
  // Información interna
  void *_internal_info;
  // Esta función configura el baudrate
  gboolean (*set_baud_rate)(glong baud_rate, struct AbstractSerialDevice **dev);
  // Esta función devuelve el baudrate actual
  glong (*get_baud_rate)(struct AbstractSerialDevice **dev);
  // Configura el bit de pariedad
  gboolean (*set_parity_bit)(gboolean bit_enable, gboolean odd_neven, struct AbstractSerialDevice **dev);
  // Devuelve el estado del bit de pariedad
  gboolean (*get_parity_bit)(struct AbstractSerialDevice **dev);
  // Devuelve TRUE mientras el bit de pariedad sea impar
  gboolean (*get_parity_odd_neven)(struct AbstractSerialDevice **dev);
};

// Esta función toma un puntero a un puntero de un Abstract Serial Device, reserva memoria, abre el puerto y devuelve
// el resultado de la operación.
//  -> Verifica si el puntero no es null
//  -> Reserva memoria para el driver
//  -> Intenta abrir el puerto
//  -> Si falla, el puerto no se abre, el puntero se vuelve NULL y retorna FALSE
//  -> Si lo logra, llena el driver con los pertinentes
gboolean open_serial_port(struct AbstractSerialDevice **dev, GString *os_dev);

// Esta función cierra un puerto serial y libera los recursos asociados.
void close_serial_port(struct AbstractSerialDevice **dev);
