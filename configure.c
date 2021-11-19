#include <gtk/gtk.h>
#include <stdbool.h>

#include "rombox.h"

#include "globals.h"

typedef struct ConfigureState {
	bool configuring;
} ConfigureState;
ConfigureState configure_state = { .configuring = false };

void close_configuration(GtkWidget *widget, gpointer data) {
	configure_state.configuring = false;
	toggle_sensitive(widgets->button, widgets->play);
}

GtkTreeModel *create_and_fill_model(void) {
    
  GtkTreeStore *treestore;
  GtkTreeIter toplevel, child;

  treestore = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);

  gtk_tree_store_append(treestore, &toplevel, NULL);
  gtk_tree_store_set(treestore, &toplevel, 0, "NES", 2, TRUE, -1);

  gtk_tree_store_append(treestore, &child, &toplevel);
  gtk_tree_store_set(treestore, &child, 0, "Python", 1, "Path", 2, TRUE, -1);
  
  gtk_tree_store_append(treestore, &child, &toplevel);
  gtk_tree_store_set(treestore, &child, 0, "Perl", 1, "asdf", 2, TRUE, -1);

  return GTK_TREE_MODEL(treestore);
}

GtkWidget *create_view_and_model(void) {
    
	GtkTreeViewColumn *name_col, *path_col, *editable_col;
	GtkCellRenderer *renderer;
	GtkWidget *view;
	GtkTreeModel *model;

	view = gtk_tree_view_new();

	name_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(name_col, "Name");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), name_col);
	gtk_tree_view_column_set_resizable(name_col, TRUE);
	gtk_tree_view_column_set_sort_column_id(name_col, 0);
	
	path_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(path_col, "Path");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), path_col);
	gtk_tree_view_column_set_resizable(path_col, TRUE);
	
	editable_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(editable_col, "Editable");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), editable_col);
	gtk_tree_view_column_set_visible(editable_col, FALSE);
	
	renderer = gtk_cell_renderer_text_new();
	
	gtk_tree_view_column_pack_start(name_col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(name_col, renderer, "text", 0);
	
	gtk_tree_view_column_pack_start(path_col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(path_col, renderer, "text", 1);
	
	gtk_tree_view_column_pack_end(editable_col, renderer, FALSE);
	gtk_tree_view_column_add_attribute(editable_col, renderer, "editable", 2);

	model = create_and_fill_model();
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);
	g_object_unref(model); 

	return view;
}
/*
void on_changed(GtkWidget *widget, gpointer label) {
    
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *value;

  if (gtk_tree_selection_get_selected(
      GTK_TREE_SELECTION(widget), &model, &iter)) {

    gtk_tree_model_get(model, &iter, LIST_ITEM, &value,  -1);
    gtk_label_set_text(GTK_LABEL(label), value);
    g_free(value);
  }
}
*/
void configure(GtkWidget *widget, GdkEvent *event, gpointer data) {
	if (configure_state.configuring) {
		return;
	}
	configure_state.configuring = true;
	
	toggle_sensitive(widgets->button, widgets->play);
	
	/* Create Window */
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Configuration");
	gtk_window_set_default_size(GTK_WINDOW(window), 500, 300);
	
	/* Create vbox */
	GtkWidget *vbox;
	
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	
	/* Create roms hbox */
	GtkWidget *hbox_roms;
	hbox_roms = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	
	
	/* Create emulators hbox */
	GtkWidget *hbox_emulators;
	hbox_emulators = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	
	/* Create roms scrolling window */
	GtkWidget *roms_scrollwindow = gtk_scrolled_window_new(NULL, NULL);
	
	/* Create emulators scrolling window */
	GtkWidget *emulators_scroll_window = gtk_scrolled_window_new(NULL, NULL);
	
	/* Create roms tree view */
	GtkWidget *roms_view;
	GtkTreeSelection *roms_selection; 
	roms_view = create_view_and_model();
	roms_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(roms_view));
	
	/* Create emulators tree view */
	
	/* Create roms vbox */
	GtkWidget *vbox_roms;
	vbox_roms = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	
	/* Create roms buttons */
	GtkWidget *roms_addromm, *roms_delete, *roms_newsystem;
	
	roms_addromm = gtk_button_new_with_label ("Add ROM");
	roms_delete = gtk_button_new_with_label ("Delete");
	roms_newsystem = gtk_button_new_with_label ("Add System");
	
	/* Create emulators buttons */
	
	/* Add stuff to other stuff */
	
	GtkWidget *roms_label = gtk_label_new("Roms");
	GtkWidget *emulators_label = gtk_label_new("Emulators");
	
	gtk_container_add(GTK_CONTAINER(roms_scrollwindow), roms_view);
	gtk_box_pack_start(GTK_BOX(hbox_roms), roms_scrollwindow, TRUE, TRUE, 0);
	
	gtk_box_pack_start(GTK_BOX(vbox_roms), roms_newsystem, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox_roms), roms_addromm, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox_roms), roms_delete, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox_roms), vbox_roms, FALSE, FALSE, 5);
	//gtk_box_pack_start(GTK_BOX(hbox_roms), roms_buttons, TRUE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(vbox), hbox_roms, TRUE, TRUE, 0);
	
	gtk_box_pack_start(GTK_BOX(vbox), hbox_emulators, TRUE, TRUE, 0);
	

	//g_signal_connect(selection, "changed", G_CALLBACK(on_changed), statusbar);
	  
	g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK(close_configuration), NULL);

  gtk_widget_show_all(window);
}