#include "config.h"
#include <gtk/gtk.h>

GtkWidget *input_swi[APP_SWI_SIZE];
GtkWidget *output_swo[APP_SWO_SIZE];
GtkWidget *hex_tbi;
GtkWidget *hex_tbo;
GtkWidget *send_bto;

static void activate(GtkApplication *app, gpointer user_data) {
  // Crea una nueva ventana
  GtkWidget *window;
  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), APP_MAIN_TITLE);
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
    char str[21];
    sprintf(str, "%d", i);
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
    char str[21];
    sprintf(str, "%d", j);
    GtkWidget *lbl = gtk_label_new(str);
    gtk_grid_attach(GTK_GRID(grid), lbl, 2, j, 1, 1);
  }

  // Crea un textbox para cada columna y un botón para enviar en la columna izquierda
  hex_tbi = gtk_entry_new();
  gtk_grid_attach(GTK_GRID(grid), hex_tbi, 0, APP_SWO_SIZE, 2, 1);
  hex_tbo = gtk_entry_new();
  gtk_grid_attach(GTK_GRID(grid), hex_tbo, 2, APP_SWO_SIZE, 2, 1);
  gtk_widget_set_sensitive(GTK_WIDGET(hex_tbo), FALSE); // deshabilita este textbox
  send_bto = gtk_button_new();
  gtk_grid_attach(GTK_GRID(grid), send_bto, 0, APP_SWO_SIZE + 1, 4, 1);
  gtk_button_set_label(GTK_BUTTON(send_bto), "Enviar byte");

  // Muestra la ventana
  gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
  // Crea una nueva aplicación de GTK
  GtkApplication *app;
  // Esta variable almacena el estado de retorno de la aplicación
  int status;
  // Devuelve una nueva instancia de la App de GTK
  app = gtk_application_new(APP_ID, G_APPLICATION_FLAGS_NONE);
  // Conecta la aplicación a la señal `activate`, con el callback a la función `activate`, sin datos para pasar
  g_signal_connect (app, "activate", G_CALLBACK(activate), NULL);
  // Lanza la aplicación `app` de GTK, con los argumentos argc, argv y bloquea hasta que la aplicación termina
  status = g_application_run(G_APPLICATION (app), argc, argv);
  // Libera la instancia de la `app` (liberando memoria)
  g_object_unref(app);
  // Retorna
  return status;
}