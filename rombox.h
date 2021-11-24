#pragma once

void toggle_sensitive(GtkWidget *interactive_widget, GtkWidget *label);
cJSON * get_array_item_with_kv_pair(cJSON *arr, char * key, char * value, int *index);
void update_program_combo(GtkWidget *program_combo);
void update_selected(GtkWidget *combo, gpointer data);
void load_data();
void store_data(char * data);