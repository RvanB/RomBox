#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>

#include "cJSON.h"
#include "rombox.h"
#include "globals.h"

/* Globals */
static GtkWidget *roms_treeview, *emulators_treeview;
static GtkTreeIter roms_toplevel, roms_child, emulators_toplevel, emulators_child;

typedef struct ConfigState {
	cJSON *selected_rom;
	cJSON *selected_emulator;
} ConfigState;
static ConfigState config_state = { .selected_rom = NULL, .selected_emulator = NULL };

void close_configuration(GtkWidget *widget, gpointer data) {
	toggle_sensitive(widgets->button, widgets->play);
	toggle_sensitive(widgets->config_button, widgets->config_label);
	
	gtk_widget_destroy(widget);
}

void select_file(GtkWindow *widget, gpointer data) {
	struct data {
		GtkWidget *window;
		GtkWidget *system_entry;
		GtkWidget *name_entry;
		GtkWidget *path_entry;
		GtkWidget *playtime_spin;
		char * path;
		char * title;
	};
	struct data * passed = (struct data *) data;
	
	GtkWindow *parent = GTK_WINDOW(passed->window);
	GtkFileChooserNative *native = gtk_file_chooser_native_new(passed->title, parent, GTK_FILE_CHOOSER_ACTION_OPEN, "Open", "Cancel");
	gint res = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
	
	if (res == GTK_RESPONSE_ACCEPT) {
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
		passed->path = gtk_file_chooser_get_filename(chooser);
		
		gtk_entry_set_text(GTK_ENTRY(passed->path_entry), gtk_file_chooser_get_filename(chooser));
	}
}

void roms_load_config() {
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(roms_treeview));
	
	gtk_tree_store_clear(GTK_TREE_STORE(model));
	
	cJSON *system = NULL;
	cJSON_ArrayForEach(system, config->systems) {
		gtk_tree_store_append(GTK_TREE_STORE(model), &roms_toplevel, NULL);
		cJSON *system_name_obj = cJSON_GetObjectItemCaseSensitive(system, "system");
		char * system_name = cJSON_GetStringValue(system_name_obj);
		gtk_tree_store_set(GTK_TREE_STORE(model), &roms_toplevel, 0, system_name, -1);
		
		cJSON *roms = cJSON_GetObjectItemCaseSensitive(system, "roms");
		cJSON *rom = NULL;
		cJSON_ArrayForEach(rom, roms) {
			char * name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(rom, "name"));
			char * path = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(rom, "path"));
			int playtime = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(rom, "playtime"));
			
			int max_length = 10;
			char* playtime_str = (char *)malloc(sizeof(char) * (max_length + 1));
			snprintf(playtime_str, max_length + 1, "%d", playtime);
			
			gtk_tree_store_append(GTK_TREE_STORE(model), &roms_child, &roms_toplevel);
			gtk_tree_store_set(GTK_TREE_STORE(model), &roms_child, 0, name, 1, path, 2, playtime_str, -1);
			
			free(playtime_str);
		}
	}
	
	gtk_tree_view_set_model(GTK_TREE_VIEW(roms_treeview), model);
}

void roms_add_item(const char * system, const char * name, const char * path, int playtime) {
	//GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(roms_treeview));
	
	g_print("Add entry: %s, %s, %s, %d\n", system, name, path, playtime);
	
	//gtk_tree_view_set_model(GTK_TREE_VIEW(roms_treeview), model);
	
	roms_load_config();
}

GtkWidget *roms_init_mv(void) {

	/* Create View */
	roms_treeview = gtk_tree_view_new();

	GtkTreeViewColumn *name_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(name_col, "Name");
	gtk_tree_view_append_column(GTK_TREE_VIEW(roms_treeview), name_col);
	gtk_tree_view_column_set_resizable(name_col, TRUE);
	gtk_tree_view_column_set_sort_column_id(name_col, 0);
	
	GtkTreeViewColumn *path_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(path_col, "Path");
	gtk_tree_view_append_column(GTK_TREE_VIEW(roms_treeview), path_col);
	gtk_tree_view_column_set_resizable(path_col, TRUE);
	
	GtkTreeViewColumn *playtime_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(playtime_col, "Playtime");
	gtk_tree_view_append_column(GTK_TREE_VIEW(roms_treeview), playtime_col);
	gtk_tree_view_column_set_resizable(playtime_col, TRUE);
	
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	
	gtk_tree_view_column_pack_start(name_col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(name_col, renderer, "text", 0);
	
	gtk_tree_view_column_pack_start(path_col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(path_col, renderer, "text", 1);
	
	gtk_tree_view_column_pack_start(playtime_col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(playtime_col, renderer, "text", 2);

	
	/* Create Model */
	GtkTreeStore *treestore;
	treestore = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	GtkTreeModel *model = GTK_TREE_MODEL(treestore);
	
	gtk_tree_view_set_model(GTK_TREE_VIEW(roms_treeview), model);
	g_object_unref(model);
}

void roms_update_selection(GtkWidget *widget, gpointer data) {
	GtkTreeIter iter;
	GtkTreeIter temp;
	GtkTreeIter parent;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(roms_treeview));
	
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		if (gtk_tree_model_iter_parent(model, &parent, &iter)) {
			char *parent_name;
			gtk_tree_model_get(model, &parent, 0, &parent_name, -1);
			
			/* has a parent */
			char *name;
			gtk_tree_model_get(model, &iter, 0, &name, -1);
			
			cJSON *system = get_array_item_with_kv_pair(config->systems, "system", parent_name);
			cJSON *roms = cJSON_GetObjectItemCaseSensitive(system, "roms");
			config_state.selected_rom = get_array_item_with_kv_pair(roms, "name", name);
		}
	}
}

void rom_add_dialog_close(GtkWidget *widget, gint response_id, gpointer data) {
	struct data {
		GtkWidget *window;
		GtkWidget *system_entry;
		GtkWidget *name_entry;
		GtkWidget *path_entry;
		GtkWidget *playtime_spin;
		char * path;
		char * title;
	};
	struct data * passed = (struct data *)data;
	
	if (response_id == GTK_RESPONSE_ACCEPT) {
		const char * system = gtk_entry_get_text(GTK_ENTRY(passed->system_entry));
		const char * name = gtk_entry_get_text(GTK_ENTRY(passed->name_entry));
		const char * path = gtk_entry_get_text(GTK_ENTRY(passed->path_entry));
		int playtime = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(passed->playtime_spin));
		
		roms_add_item(system, name, path, playtime);
	}
	
	gtk_widget_destroy(passed->window);
	free(passed);
}

void rom_add_dialog(gpointer data) {
	GtkWindow *parent = (GtkWindow *)data;
	
	GtkWidget *dialog, *content_area;
	GtkDialogFlags flags;
	
	flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
	dialog = gtk_dialog_new_with_buttons("New ROM", NULL, flags, "OK", GTK_RESPONSE_ACCEPT, "Cancel", GTK_RESPONSE_REJECT, NULL);
	content_area = gtk_dialog_get_content_area( GTK_DIALOG(dialog));
	gtk_box_set_spacing(GTK_BOX(content_area), 20);
	
	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
	
	GtkWidget *system_label = gtk_label_new("System:");
	GtkWidget *system_entry = gtk_entry_new();
	
	gtk_grid_attach(GTK_GRID(grid), system_label, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), system_entry, 1, 0, 1, 1);
	
	GtkWidget *name_label = gtk_label_new("Name:");
	GtkWidget *name_entry = gtk_entry_new();
	
	gtk_grid_attach(GTK_GRID(grid), name_label, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), name_entry, 1, 1, 1, 1);
	
	GtkWidget *path_label = gtk_label_new("Path:");
	GtkWidget *path_entry = gtk_entry_new();
	GtkWidget *browse = gtk_button_new_with_label("Browse");
	
	gtk_grid_attach(GTK_GRID(grid), path_label, 0, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), path_entry, 1, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), browse, 2, 2, 1, 1);
	
	GtkWidget *playtime_label = gtk_label_new("Playtime:");
	GtkWidget *playtime_spin = gtk_spin_button_new_with_range(0, INT_MAX, 1.0);	
	gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(playtime_spin), TRUE);
	
	
	gtk_grid_attach(GTK_GRID(grid), playtime_label, 0, 3, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), playtime_spin, 1, 3, 1, 1);
	
	gtk_container_add(GTK_CONTAINER(content_area), grid);
	
	struct data {
		GtkWidget *window;
		GtkWidget *system_entry;
		GtkWidget *name_entry;
		GtkWidget *path_entry;
		GtkWidget *playtime_spin;
		char * path;
		char * title;
	};
	struct data * passed = (struct data *) malloc(sizeof(struct data));
	passed->window = dialog;
	passed->system_entry = system_entry;
	passed->name_entry = name_entry;
	passed->path_entry = path_entry;
	passed->playtime_spin = playtime_spin;
	passed->path = NULL;
	passed->title = "Select ROM file";
	
	g_signal_connect(browse, "clicked", G_CALLBACK(select_file), passed);
	
	g_signal_connect(dialog, "response", G_CALLBACK (rom_add_dialog_close), passed);
	gtk_widget_show_all(dialog);
}

void emulators_add_item(GtkWidget *widget, gpointer data) {
	
}

GtkWidget *emulators_init_mv(void) {
		/* Create View */
	emulators_treeview = gtk_tree_view_new();

	GtkTreeViewColumn *name_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(name_col, "Name");
	gtk_tree_view_append_column(GTK_TREE_VIEW(emulators_treeview), name_col);
	gtk_tree_view_column_set_resizable(name_col, TRUE);
	gtk_tree_view_column_set_sort_column_id(name_col, 0);
	
	GtkTreeViewColumn *path_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(path_col, "Path");
	gtk_tree_view_append_column(GTK_TREE_VIEW(emulators_treeview), path_col);
	gtk_tree_view_column_set_resizable(path_col, TRUE);
	
	GtkTreeViewColumn *flags_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(flags_col, "Flags");
	gtk_tree_view_append_column(GTK_TREE_VIEW(emulators_treeview), flags_col);
	gtk_tree_view_column_set_resizable(flags_col, TRUE);
	
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	
	gtk_tree_view_column_pack_start(name_col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(name_col, renderer, "text", 0);
	
	gtk_tree_view_column_pack_start(path_col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(path_col, renderer, "text", 1);
	
	gtk_tree_view_column_pack_start(flags_col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(flags_col, renderer, "text", 2);

	
	/* Create Model */
	GtkTreeStore *treestore;
	treestore = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_store_append(treestore, &roms_toplevel, NULL);
	gtk_tree_store_set(treestore, &roms_toplevel, 0, "NES", -1);

	gtk_tree_store_append(treestore, &roms_child, &roms_toplevel);
	gtk_tree_store_set(treestore, &roms_child, 0, "NESTOPIA", 1, "Something", 2, "7", -1);
  
	gtk_tree_store_append(treestore, &roms_child, &roms_toplevel);
	gtk_tree_store_set(treestore, &roms_child, 0, "MESEN", 1, "SOMETHING ELSE", 2, "8", -1);
	
	GtkTreeModel *model = GTK_TREE_MODEL(treestore);
	
	gtk_tree_view_set_model(GTK_TREE_VIEW(emulators_treeview), model);
	g_object_unref(model);
}

void emulators_update_selection(GtkWidget *widget, gpointer data) {
	
}

void emulator_add_dialog_close(GtkWidget *widget, gint response_id, gpointer data) {
	
}

void emulator_add_dialog(gpointer data) {
	
}

void configure(GtkWidget *widget, GdkEvent *event, gpointer data) {
	
	toggle_sensitive(widgets->button, widgets->play);
	toggle_sensitive(widgets->config_button, widgets->config_label);
	
	/* Create Window */
	GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
	GtkWidget *config_dialog = gtk_dialog_new_with_buttons("Configuration", NULL, flags, "Save", GTK_RESPONSE_ACCEPT, "Cancel", GTK_RESPONSE_REJECT, NULL);
	GtkWidget *config_content_area = gtk_dialog_get_content_area(GTK_DIALOG(config_dialog));
	
	//gtk_window_set_title(GTK_WINDOW(window), "Configuration");
	gtk_window_set_default_size(GTK_WINDOW(config_dialog), 640, 480);
	
	/* Create roms hbox */
	GtkWidget *hbox_roms;
	hbox_roms = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	
	
	/* Create emulators hbox */
	GtkWidget *hbox_emulators;
	hbox_emulators = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	
	/* Create roms scrolling window */
	GtkWidget *roms_scrollwindow = gtk_scrolled_window_new(NULL, NULL);
	
	/* Create emulators scrolling window */
	GtkWidget *emulators_scrollwindow = gtk_scrolled_window_new(NULL, NULL);
	
	/* Create roms tree view */
	GtkTreeSelection *roms_selection;
	roms_init_mv();
	roms_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(roms_treeview));
	
	/* Create emulators tree view */
	GtkTreeSelection *emulators_selection;
	emulators_init_mv();
	emulators_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(emulators_treeview));
	
	/* Create roms vbox */
	GtkWidget *vbox_roms;
	vbox_roms = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	
	/* Create emulators vbox */
	GtkWidget *vbox_emulators;
	vbox_emulators = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	
	/* Create roms buttons */
	GtkWidget *roms_new, *roms_remove, *roms_edit;
	
	roms_new = gtk_button_new_with_label ("New");
	roms_edit = gtk_button_new_with_label ("Edit");
	roms_remove = gtk_button_new_with_label ("Remove");
	
	g_signal_connect (roms_new, "clicked", G_CALLBACK (rom_add_dialog), config_content_area);
	
	/* Create emulators buttons */
	GtkWidget *emulators_new, *emulators_remove, *emulators_edit;
	
	emulators_new = gtk_button_new_with_label ("New");
	emulators_edit = gtk_button_new_with_label ("Edit");
	emulators_remove = gtk_button_new_with_label ("Remove");
	
	//g_signal_connect (roms_new, "clicked", G_CALLBACK (rom_add_dialog), config_content_area);
	
	/* Add stuff to other stuff */
	
	GtkWidget *roms_frame = gtk_frame_new("ROMs");
	gtk_frame_set_label_align(GTK_FRAME(roms_frame), 0.5, 1);
	
	GtkWidget *emulators_frame = gtk_frame_new("Emulators");
	gtk_frame_set_label_align(GTK_FRAME(emulators_frame), 0.5, 1);
	
	gtk_container_add(GTK_CONTAINER(roms_scrollwindow), roms_treeview);
	gtk_container_add(GTK_CONTAINER(roms_frame), roms_scrollwindow);
	gtk_box_pack_start(GTK_BOX(hbox_roms), roms_frame, TRUE, TRUE, 10);
	
	gtk_container_add(GTK_CONTAINER(emulators_scrollwindow), emulators_treeview);
	gtk_container_add(GTK_CONTAINER(emulators_frame), emulators_scrollwindow);
	gtk_box_pack_start(GTK_BOX(hbox_emulators), emulators_frame, TRUE, TRUE, 10);
	
	gtk_box_pack_start(GTK_BOX(vbox_roms), roms_new, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_roms), roms_edit, FALSE, FALSE, 10);
	gtk_box_pack_start(GTK_BOX(vbox_roms), roms_remove, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_roms), vbox_roms, FALSE, FALSE, 10);
	
	gtk_box_pack_start(GTK_BOX(vbox_emulators), emulators_new, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_emulators), emulators_edit, FALSE, FALSE, 10);
	gtk_box_pack_start(GTK_BOX(vbox_emulators), emulators_remove, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_emulators), vbox_emulators, FALSE, FALSE, 10);
	
	gtk_box_pack_start(GTK_BOX(config_content_area), hbox_roms, TRUE, TRUE, 10);
	gtk_box_pack_start(GTK_BOX(config_content_area), hbox_emulators, TRUE, TRUE, 10);
	

	g_signal_connect(roms_selection, "changed", G_CALLBACK(roms_update_selection), NULL);
	//g_signal_connect(emulators_selection, "changed", G_CALLBACK(emulators_update_selection), NULL);
	  
	g_signal_connect(config_dialog, "response", G_CALLBACK (close_configuration), config_dialog);

	gtk_widget_show_all(config_dialog);
	
	roms_load_config();
}