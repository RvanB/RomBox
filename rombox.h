#pragma once

void toggle_sensitive(GtkWidget *interactive_widget, GtkWidget *label);
cJSON * get_array_item_with_kv_pair(cJSON *arr, char * key, char * value);