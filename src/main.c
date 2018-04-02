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
#include <abserio/abserio.h>
#include <errno.h>
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
volatile char *print_format;
const struct AbstractSerialDevice *abstract_port = NULL;
GString *os_port;
gchar data_readed;

//===--------------------------------------------------------------------------------------------------------------===//
//                                                   Funciones extra
//===--------------------------------------------------------------------------------------------------------------===//
void print_formatted_input(void) {
  // Obtiene el valor binario a partir de lo switches
  unsigned long val = 0x00;
  for (int i = 0; i < APP_SWI_SIZE; i++) {
    unsigned long binval = (unsigned long) gtk_switch_get_state(GTK_SWITCH(input_swi[i]));
    binval = (binval & 0x01);
    val |= binval << i;
  }
  char *use_format = "";
  char formatted[10];
  // Detecta el formato de salida
  if (strcmp((const char *) print_format, APP_STR_ASCII)==0) {
    // Usar el valor decimal con un \ para los valores de 0 a 31.
    // De 31 a 254, usar el valor ASCII normal
    switch (val) {
      case 0 ... 31://
        use_format = "\'\\%02d\'";
        break;
      case 0x7F://
        use_format = "\'\\DEL\'";
        break;
      case 0x80 ... 0xFF://
        use_format = "\'\\-%02d\'";
        break;
      default://
        use_format = "\'%c\'";
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
  unsigned long parsed_value = 0x00;
  if (strcmp((const char *) print_format, APP_STR_ASCII)==0) {
    if (ctext[0]=='\'' && ctext[2]=='\'') {
      parsed_value = (unsigned long) *(ctext + 1);
    }
  } else if (strcmp((const char *) print_format, APP_STR_HEX)==0) {
    char **tmp = NULL;
    parsed_value = strtoul(ctext, tmp, 16);
  } else if (strcmp((const char *) print_format, APP_STR_DEC)==0) {
    char **tmp = NULL;
    parsed_value = strtoul(ctext, tmp, 10);
  } else if (strcmp((const char *) print_format, APP_STR_OCT)==0) {
    char **tmp = NULL;
    parsed_value = strtoul(ctext, tmp, 8);
  }
  for (int i = 0; i < APP_SWI_SIZE; i++) {
    gboolean bit_n = (gboolean) ((parsed_value >> i) & 0x01);
    gtk_switch_set_state(GTK_SWITCH(input_swi[i]), bit_n);
  }
}

void setup_port_diag(GtkButton *button, GtkWindow *window) {
  char formatted[100];
  sprintf(formatted, APP_SETUP_SR_DIALOG_TITLE, os_port->str);
  GtkDialog *setup_port_dialog = (GtkDialog *) gtk_dialog_new_with_buttons(
      formatted,
      GTK_WINDOW(window),
      GTK_DIALOG_MODAL,
      APP_OK,
      GTK_RESPONSE_ACCEPT,
      APP_CANCEL,
      GTK_RESPONSE_REJECT,
      NULL);
  GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(setup_port_dialog));
  // Usar grid como Layout Manager
  GtkWidget *grid_dialog;
  grid_dialog = gtk_grid_new();
  // Agrega el grid a la ventana del dialog
  gtk_container_add(GTK_CONTAINER(content_area), grid_dialog);
  gtk_widget_show_all(GTK_WIDGET(content_area));
  // Combo box para los bauds
  GtkWidget *combo_bauds = gtk_combo_box_text_new();
  for (int j = 0; j < BAUDS_AVAIL; j++) {
    long curr_baud = bauds[j];
    if (curr_baud==0) {
      break;
    }
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_bauds),
                              g_strdup_printf("%lu", curr_baud),
                              g_strdup_printf("%lu", curr_baud));
  }
  gtk_grid_attach(GTK_GRID(grid_dialog), gtk_label_new(APP_DIALOG_BAUD_RATE), 0, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(grid_dialog), combo_bauds, 1, 0, 1, 1);
  gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo_bauds),
                              g_strdup_printf("%lu", abstract_port->get_baud_rate(&abstract_port)));
  // Switch para el parity bit
  GtkWidget *switch_parity_enable = gtk_switch_new();
  GtkWidget *switch_parity_odd = gtk_switch_new();
  gtk_switch_set_state(GTK_SWITCH(switch_parity_enable), abstract_port->get_parity_bit(&abstract_port));
  gtk_switch_set_state(GTK_SWITCH(switch_parity_odd), abstract_port->get_parity_odd_neven(&abstract_port));
  gtk_grid_attach(GTK_GRID(grid_dialog), gtk_label_new(APP_DIALOG_PARITY_ENABLE), 0, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(grid_dialog), switch_parity_enable, 1, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(grid_dialog), gtk_label_new(APP_DIALOG_PARITY_ODD), 0, 2, 1, 1);
  gtk_grid_attach(GTK_GRID(grid_dialog), switch_parity_odd, 1, 2, 1, 1);

  // Switch para el control por software
  GtkWidget *switch_swofl_enable = gtk_switch_new();
  gtk_switch_set_state(GTK_SWITCH(switch_swofl_enable), abstract_port->get_software_control_flow(&abstract_port));
  gtk_grid_attach(GTK_GRID(grid_dialog), gtk_label_new(APP_DIALOG_SWCTL), 0, 3, 1, 1);
  gtk_grid_attach(GTK_GRID(grid_dialog), switch_swofl_enable, 1, 3, 1, 1);

  // Muestra y ejecuta el diálogo
  gtk_widget_show_all(GTK_WIDGET(content_area));
  gint dialog_response = gtk_dialog_run(setup_port_dialog);
  char *str2p = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo_bauds));
  gboolean parity_enable_boolean_switch = gtk_switch_get_state(GTK_SWITCH(switch_parity_enable));
  gboolean parity_odd_boolean_switch = gtk_switch_get_state(GTK_SWITCH(switch_parity_odd));
  gboolean switch_swofl_boolean_switch = gtk_switch_get_state(GTK_SWITCH(switch_swofl_enable));
  switch (dialog_response) {
    case GTK_RESPONSE_ACCEPT://
      errno = 0x00;
      // Configurar el puerto
      if (str2p!=NULL) {
        abstract_port->set_baud_rate((glong) strtoull(str2p, NULL, 10), &abstract_port);
        if (errno!=0) goto on_errno_not_zero_setup_port;
      }
      abstract_port->set_parity_bit(parity_enable_boolean_switch, parity_odd_boolean_switch, &abstract_port);
      if (errno!=0) goto on_errno_not_zero_setup_port;
      abstract_port->set_software_control_flow(switch_swofl_boolean_switch, &abstract_port);
      if (errno!=0) goto on_errno_not_zero_setup_port;
    on_errno_not_zero_setup_port:
      if (errno!=0x00) {
        GtkWidget *error_chg_serial = gtk_message_dialog_new(GTK_WINDOW(setup_port_dialog),
                                                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                             GTK_MESSAGE_ERROR,
                                                             GTK_BUTTONS_CLOSE,
                                                             "No se han cambiado las configuraciones de “%s”: %s",
                                                             os_port->str,
                                                             g_strerror(errno));
        gtk_dialog_run(GTK_DIALOG(error_chg_serial));
        gtk_widget_destroy(GTK_WIDGET(error_chg_serial));
      }
      break;
    case GTK_RESPONSE_REJECT://
      break;
    default://
      break;
  }

  // Destruir hasta que termine de usarlo
  gtk_widget_destroy(GTK_WIDGET(setup_port_dialog));
}

void send_byte(GtkButton *button, GtkWindow *window) {
  // Obtiene el valor binario a partir de lo switches
  unsigned long val = 0x00;
  for (int i = 0; i < APP_SWI_SIZE; i++) {
    unsigned long binval = (unsigned long) gtk_switch_get_state(GTK_SWITCH(input_swi[i]));
    binval = (binval & 0x01);
    val |= binval << i;
  }
  gboolean success = abstract_port->write_byte((gchar) val, &abstract_port);
  if (!success) {
    GtkWidget *error_send_serial = gtk_message_dialog_new(GTK_WINDOW(window),
                                                          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                          GTK_MESSAGE_ERROR,
                                                          GTK_BUTTONS_CLOSE,
                                                          "No se ha enviado el byte con valor “%d”: %s",
                                                          (int) val,
                                                          g_strerror(errno));
    gtk_dialog_run(GTK_DIALOG(error_send_serial));
    gtk_widget_destroy(GTK_WIDGET(error_send_serial));
  }
}

void deactivate(GtkWidget *object, gpointer user_data) {
  // Libera el puerto serial
  close_serial_port(&abstract_port);
}

//===--------------------------------------------------------------------------------------------------------------===//
//                                                 Hilos de ejecución
//===--------------------------------------------------------------------------------------------------------------===//
gboolean update_from_serial(gpointer data) {
  guchar readed = *((guchar *) data);
  for (int i = 0; i < APP_SWO_SIZE; i++) {
    gboolean bit_n = (gboolean) ((readed >> i) & 0x01);
    gtk_switch_set_state(GTK_SWITCH(output_swo[i]), bit_n);
  }
  char *use_format = "";
  char formatted[10];
  // Detecta el formato de salida
  if (strcmp((const char *) print_format, APP_STR_ASCII)==0) {
    // Usar el valor decimal con un \ para los valores de 0 a 31.
    // De 31 a 254, usar el valor ASCII normal
    switch (readed) {
      case 0 ... 31://
        use_format = "\'\\%02d\'";
        break;
      case 0x7F://
        use_format = "\'\\DEL\'";
        break;
      case 0x80 ... 0xFF://
        use_format = "\'\\-%02d\'";
        break;
      default://
        use_format = "\'%c\'";
        break;
    }
  } else if (strcmp((const char *) print_format, APP_STR_HEX)==0) {
    use_format = "0x%02X";
  } else if (strcmp((const char *) print_format, APP_STR_DEC)==0) {
    use_format = "%d";
  } else if (strcmp((const char *) print_format, APP_STR_OCT)==0) {
    use_format = "0%0o";
  }
  sprintf(formatted, use_format, readed);
  gtk_entry_set_text(GTK_ENTRY(hex_tbo), formatted);
  return FALSE;
}
static gpointer blocking_listener(gpointer user_data) {
  while (abstract_port!=NULL) {
    errno = 0x00;
    data_readed = abstract_port->read_byte(&abstract_port);
    if (errno==ECANCELED) {
      return NULL;
    }
    gdk_threads_add_idle(update_from_serial, &data_readed);
  }
  return NULL;
}
//===--------------------------------------------------------------------------------------------------------------===//
//                                              Inicialización de la GUI
//===--------------------------------------------------------------------------------------------------------------===//
static void activate(GtkApplication *app, gpointer user_data) {
  GtkWidget *window;
  window = gtk_application_window_new(app);

  //===-------------------------------------------------------------------------
  // Dialogo modal para introducir el puerto serial
  GtkDialog *ask_serial_dialog = (GtkDialog *) gtk_dialog_new_with_buttons(
      APP_SERIAL_DIALOG_TITLE,
      GTK_WINDOW(window),
      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
      APP_OK,
      GTK_RESPONSE_ACCEPT,
      APP_CANCEL,
      GTK_RESPONSE_REJECT,
      NULL);
  GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(ask_serial_dialog));
  // Usar grid como Layout Manager
  GtkWidget *grid_dialog;
  grid_dialog = gtk_grid_new();
  // Agrega el grid a la ventana del dialog
  gtk_container_add(GTK_CONTAINER(content_area), grid_dialog);
  // Label para el mensaje de entrada
#ifdef _WIN32
  GtkWidget *msg_lbl = gtk_label_new(APP_DIALOG_ASK_MSG_WIN);
#else
  GtkWidget *msg_lbl = gtk_label_new(APP_DIALOG_ASK_MSG_POS);
#endif
  gtk_grid_attach(GTK_GRID(grid_dialog), msg_lbl, 0, 0, 1, 1);
  GtkWidget *os_port_input = gtk_entry_new();
  gtk_grid_attach(GTK_GRID(grid_dialog), os_port_input, 0, 1, 1, 1);

  // Muestra el dialogo
  gtk_widget_show_all(GTK_WIDGET(content_area));

  gint dialog_response = gtk_dialog_run(ask_serial_dialog);
  if (dialog_response==GTK_RESPONSE_ACCEPT) {
    os_port = g_string_new(gtk_entry_get_text(GTK_ENTRY(os_port_input)));
    gboolean open_result = open_serial_port(&abstract_port, os_port);
    if (!open_result) {
      GtkWidget *error_open_serial = gtk_message_dialog_new(GTK_WINDOW(ask_serial_dialog),
                                                            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                            GTK_MESSAGE_ERROR,
                                                            GTK_BUTTONS_CLOSE,
                                                            "Error al intentar abrir el puerto serial “%s”: %s",
                                                            os_port->str,
                                                            g_strerror(errno));
      gtk_dialog_run(GTK_DIALOG (error_open_serial));
      gtk_widget_destroy(window);
      return;
    }
  } else if (dialog_response==GTK_RESPONSE_REJECT || dialog_response==GTK_RESPONSE_DELETE_EVENT) {
    // Destruye la ventana, lo que resulta en la terminación inmediata de la aplicación
    gtk_widget_destroy(window);
    return;
  }
  gtk_widget_destroy(GTK_WIDGET(ask_serial_dialog));

  //===-------------------------------------------------------------------------
  gtk_window_set_title(GTK_WINDOW(window), APP_STR_MAIN_TITLE);

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
  GtkWidget *send_bto = gtk_button_new();
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
  GSList *format_rbg = gtk_radio_button_get_group(GTK_RADIO_BUTTON(ascii_rbt));

  // Botón para configurar el puerto
  GtkWidget *setup_port = gtk_button_new();
  gtk_grid_attach(GTK_GRID(grid), setup_port, 4, 4, 1, 1);
  gtk_button_set_label(GTK_BUTTON(setup_port), APP_STR_SETUP_PORT);


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
  // Conecta a la señal que se produce al dar enter en el text box
  g_signal_connect(hex_tbi, "activate", G_CALLBACK(on_inputhex_change), NULL);
  // Conecta al botón para mostrar el menú de configuración
  g_signal_connect(setup_port, "clicked", G_CALLBACK(setup_port_diag), window);
  // Conecta al botón para enviar el byte
  g_signal_connect(send_bto, "clicked", G_CALLBACK(send_byte), window);
  // Conecta la aplicación a la señal `destroy`, que finaliza el hilo escucha
  g_signal_connect(window, "destroy", G_CALLBACK(deactivate), NULL);

  /*
   * En realidad no estoy seguro de lo que pasa en Windows al crear un nuevo g_thread. En POSIX pues tenemos
   * Posix Thread (pthreads) y soporte completo para los hilos de C11. Voy a suponer que GTK sabe lo mejor para Windows.
   */
  g_thread_new(NULL, blocking_listener, NULL);

  // Muestra la ventana ya diseñada
  gtk_widget_show_all(window);
}

//===--------------------------------------------------------------------------------------------------------------===//
//                                                        Main
//===--------------------------------------------------------------------------------------------------------------===//
int main(int argc, char **argv) {
  // Reservar recursos

  // Crea una nueva aplicación de GTK
  GtkApplication *app;
  // Esta variable almacena el estado de retorno de la aplicación
  int status;
  // Devuelve una nueva instancia de la App de GTK
  app = gtk_application_new(APP_ID, G_APPLICATION_FLAGS_NONE);
  // Conecta la aplicación a la señal `activate`, con el callback a la función `activate`, sin datos para pasar
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

  // Lanza la aplicación `app` de GTK, con los argumentos argc, argv y bloquea hasta que la aplicación termina
  status = g_application_run(G_APPLICATION(app), argc, argv);
  // Libera la instancia de la `app` (liberando memoria)
  g_object_unref(app);

  // Retorna
  return status;
}
