//===-- src/config.h - Definiciones de configuración  -----------------------------------------------------*- C -*-===//
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
//===---------------------------------------------------------------------------------------------------------------===//
///
/// Contiene ciertas macros y definiciones de configuración para este proyecto.
///
//===---------------------------------------------------------------------------------------------------------------===//

#ifndef CONFIG_H
#define CONFIG_H

#define APP_ID                          "com.notengo.battery.gtk_serial"
#define APP_SWI_SIZE                    8
#define APP_SWO_SIZE                    8
#define APP_IDX_FORMAT                  "%02d"
#define APP_HEX_ZERO                    "0x00"

#define APP_STR_MAIN_TITLE              "GTK Serial Tester"
#define APP_STR_SEND_BYTE               "Enviar byte"
#define APP_STR_SETUP_PORT              "PORT..."
#define APP_STR_ASCII                   "ASCII"
#define APP_STR_DEC                     "DEC"
#define APP_STR_HEX                     "HEX"
#define APP_STR_OCT                     "OCT"
#define APP_STR_BIN                     "BIN"
#define APP_SERIAL_DIALOG_TITLE         "Seleccionar el puerto serial"
#define APP_SETUP_SR_DIALOG_TITLE       "Configuración del puerto %s"
#define APP_OK                          "Aceptar"
#define APP_CANCEL                      "Cancelar"
#define APP_DIALOG_ASK_MSG_WIN          "Ingrese el número del puerto COM:"
#define APP_DIALOG_ASK_MSG_POS          "Ingrese la ruta hacia el archivo del puerto:"
#define APP_DIALOG_BAUD_RATE            "Baud rate: "
#define APP_DIALOG_PARITY_ENABLE        "Bit de pariedad: "
#define APP_DIALOG_PARITY_ODD           "Bit par/!impar: "
#define APP_DIALOG_SWCTL                "Control de flujo por software: "

#endif // CONFIG_H
