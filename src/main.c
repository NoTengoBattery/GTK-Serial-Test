//===-- src/main.c - Programa principal -------------------------------------------------------------------*- C -*-===//
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
/// Contiene la GUI principal de GTK.
///
//===--------------------------------------------------------------------------------------------------------------===//

#include "config.h"
#include <gtk/gtk.h>
#include <memory.h>
#include <stdatomic.h>
#include <stdlib.h>
#ifdef _WIN32
#include <stdint.h>
#endif

//===--------------------------------------------------------------------------------------------------------------===//
//                                                 Estructuras de datos
//===--------------------------------------------------------------------------------------------------------------===//

//===--------------------------------------------------------------------------------------------------------------===//
//                                                      Globales
//===--------------------------------------------------------------------------------------------------------------===//

GtkWidget *input_swi[APP_SWI_SIZE];
GtkWidget *output_swo[APP_SWO_SIZE];
GtkWidget *hex_tbi;
GtkWidget *hex_tbo;
GtkWidget *send_bto;
GSList *format_rbg;
volatile char *print_format;

//===--------------------------------------------------------------------------------------------------------------===//
//                                                   Funciones extra
//===--------------------------------------------------------------------------------------------------------------===//
void print_formatted_input(void) {
  // Obtiene el valor binario a partir de lo switches
  uint8_t val = 0x00;
  for (int i = 0; i < APP_SWI_SIZE; i++) {
    uint8_t binval = (uint8_t) gtk_switch_get_state(GTK_SWITCH(input_swi[i]));
    binval = (uint8_t) (binval & 0x01);
    val |= binval << i;
  }
  char *use_format = "";
  char formatted[10];
  // Detecta el formato de salida
  if (strcmp((const char *) print_format, APP_STR_ASCII)==0) {
    // Usar el valor decimal con un \ para los valores de 0 a 31.
    // De 31 a 254, usar el valor ASCII normal
    switch (val) {
      case 0 ... 31:use_format = "\'\\%02d\'";
        break;
      case 0x7F:use_format = "\'\\DEL\'";
        break;
      case 0x80 ... 0xFF:use_format = "\'\\-%02d\'";
        break;
      default:use_format = "\'%c\'";
        break;
    }
  } else if (strcmp((const char *) print_format, APP_STR_HEX)==0) {
    use_format = "0x%02X";
  } else if (strcmp((const char *) print_format, APP_STR_DEC)==0) {
    use_format = "%d";
  } else if (strcmp((const char *) print_format, APP_STR_OCT)==0) {
    use_format = "0%0o";
  }
  sprintf(formatted, use_format, val);
  gtk_entry_set_text(GTK_ENTRY(hex_tbi), formatted);
}

//===--------------------------------------------------------------------------------------------------------------===//
//                                        Callbacks para los eventos de la GUI
//===--------------------------------------------------------------------------------------------------------------===//
gboolean on_switch_change(GtkSwitch *swi, gboolean state) {
  // Vamos a cambiar el valor directamente aquí porque el "default handler" se llama cuando esta función retorna.
  // Es decir, el botón mantiene su valor anterior durante esta función (es por eso que hace falta el segundo parametro)
  gtk_switch_set_state(swi, state);
  print_formatted_input();
  return FALSE;
}

void on_radiobtn_change(GtkToggleButton *togglebutton, gpointer user_data) {
  if (gtk_toggle_button_get_active(togglebutton)) {
    print_format = (volatile char *) gtk_button_get_label(GTK_BUTTON(togglebutton));
    // Actualizar el valor al nuevo formato
    print_formatted_input();
  }
}

void on_inputhex_change(GtkEditable *editable, gpointer user_data) {
  const char *ctext = gtk_entry_get_text(GTK_ENTRY(editable));
  uint8_t parsed_value = 0x00;
  if (strcmp((const char *) print_format, APP_STR_ASCII)==0) {
    if (ctext[0]=='\'' && ctext[2]=='\'') {
      parsed_value = (uint8_t) *(ctext + 1);
    }
  } else if (strcmp((const char *) print_format, APP_STR_HEX)==0) {
    char **tmp = NULL;
    parsed_value = (uint8_t) strtoul(ctext, tmp, 16);
  } else if (strcmp((const char *) print_format, APP_STR_DEC)==0) {
    char **tmp = NULL;
    parsed_value = (uint8_t) strtoul(ctext, tmp, 10);
  } else if (strcmp((const char *) print_format, APP_STR_OCT)==0) {
    char **tmp = NULL;
    parsed_value = (uint8_t) strtoul(ctext, tmp, 8);
  }
  for (int i = 0; i < APP_SWI_SIZE; i++) {
    gboolean bit_n = (parsed_value >> i) & 0x01;
    gtk_switch_set_state(GTK_SWITCH(input_swi[i]), bit_n);
  }
}

//===--------------------------------------------------------------------------------------------------------------===//
//                                              Inicialización de la GUI
//===--------------------------------------------------------------------------------------------------------------===//
static void activate(GtkApplication *app, gpointer user_data) {
  // Crea una nueva ventana
  GtkWidget *window;
  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), APP_STR_MAIN_TITLE);
  // Configura el tamaño inicial de la ventana
  gtk_window_set_default_size(GTK_WINDOW(window), 100, 100);

  // Usar grid como Layout Manager
  GtkWidget *grid;
  grid = gtk_grid_new();
  // Agrega el grid a la ventana
  gtk_container_add(GTK_CONTAINER(window), grid);

  // Crea 8 switches para entrada
  for (int i = 0; i < APP_SWI_SIZE; i++) {
    GtkWidget *curr_swi = input_swi[i] = gtk_switch_new();
    // Agrega cada uno de los switches al grid
    gtk_grid_attach(GTK_GRID(grid), curr_swi, 1, i, 1, 1);
    // Crear un label para el índice del bit
    char str[3];
    sprintf(str, APP_IDX_FORMAT, i);
    GtkWidget *lbl = gtk_label_new(str);
    gtk_grid_attach(GTK_GRID(grid), lbl, 0, i, 1, 1);
  }
  // 8 para salida
  for (int j = 0; j < APP_SWO_SIZE; j++) {
    GtkWidget *curr_swi = output_swo[j] = gtk_switch_new();
    // Agrega cada uno de los switches al grid
    gtk_grid_attach(GTK_GRID(grid), curr_swi, 3, j, 1, 1);
    // Deshabilita esta columna de switches
    gtk_widget_set_sensitive(GTK_WIDGET(curr_swi), FALSE);
    // Crear un label para el índice del bit
    char str[3];
    sprintf(str, APP_IDX_FORMAT, j);
    GtkWidget *lbl = gtk_label_new(str);
    gtk_grid_attach(GTK_GRID(grid), lbl, 2, j, 1, 1);
  }

  // Crea un textbox para cada columna y un botón para enviar en la columna izquierda
  hex_tbi = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(hex_tbi), APP_HEX_ZERO);
  gtk_grid_attach(GTK_GRID(grid), hex_tbi, 0, APP_SWO_SIZE, 2, 1);
  hex_tbo = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(hex_tbo), APP_HEX_ZERO);
  gtk_grid_attach(GTK_GRID(grid), hex_tbo, 2, APP_SWO_SIZE, 2, 1);
  gtk_widget_set_sensitive(GTK_WIDGET(hex_tbo), FALSE); // deshabilita este textbox
  send_bto = gtk_button_new();
  gtk_grid_attach(GTK_GRID(grid), send_bto, 0, APP_SWO_SIZE + 1, 4, 1);
  gtk_button_set_label(GTK_BUTTON(send_bto), APP_STR_SEND_BYTE);

  // Unos Radio Button que sirven para elegir el formato de representación
  GtkWidget *ascii_rbt = gtk_radio_button_new_with_label(NULL, APP_STR_ASCII);
  gtk_grid_attach(GTK_GRID(grid), ascii_rbt, 4, 0, 1, 1);
  GtkWidget *hex_rbt = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(ascii_rbt), APP_STR_HEX);
  gtk_grid_attach(GTK_GRID(grid), hex_rbt, 4, 1, 1, 1);
  GtkWidget *dec_rbt = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(ascii_rbt), APP_STR_DEC);
  gtk_grid_attach(GTK_GRID(grid), dec_rbt, 4, 2, 1, 1);
  GtkWidget *oct_rbt = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(ascii_rbt), APP_STR_OCT);
  gtk_grid_attach(GTK_GRID(grid), oct_rbt, 4, 3, 1, 1);
  format_rbg = gtk_radio_button_get_group(GTK_RADIO_BUTTON(ascii_rbt));

  //===-------------------------------------------------------------------------
  // Agrega los callback
  //    -> Callback para los switch de entrada
  for (int i = 0; i < APP_SWI_SIZE; i++) {
    g_signal_connect(input_swi[i], "state-set", G_CALLBACK(on_switch_change), NULL);
  }
  //    -> Callback para los radio button de selección de formato
  while (format_rbg) {
    GtkWidget *w = format_rbg->data;
    format_rbg = format_rbg->next;
    g_signal_connect(w, "toggled", G_CALLBACK(on_radiobtn_change), NULL);
  }
  // Esto dispara el handler, necesario para activar el formato correcto
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hex_rbt), TRUE);
  g_signal_connect(hex_tbi, "activate", G_CALLBACK(on_inputhex_change), NULL);
  //===-------------------------------------------------------------------------

  // Muestra la ventana
  gtk_widget_show_all(window);
}

//===--------------------------------------------------------------------------------------------------------------===//
//                                                        Main
//===--------------------------------------------------------------------------------------------------------------===//
int main(int argc, char **argv) {
  // Reservar memoria

  // Crea una nueva aplicación de GTK
  GtkApplication *app;
  // Esta variable almacena el estado de retorno de la aplicación
  int status;
  // Devuelve una nueva instancia de la App de GTK
  app = gtk_application_new(APP_ID, G_APPLICATION_FLAGS_NONE);
  // Conecta la aplicación a la señal `activate`, con el callback a la función `activate`, sin datos para pasar
  g_signal_connect (app, "activate", G_CALLBACK(activate), NULL);
  // Lanza la aplicación `app` de GTK, con los argumentos argc, argv y bloquea hasta que la aplicación termina
  status = g_application_run(G_APPLICATION(app), argc, argv);
  // Libera la instancia de la `app` (liberando memoria)
  g_object_unref(app);

  // Libera memoria

  // Retorna
  return status;
}