#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include <stdbool.h>

#define PATHLENGTH 1024

static char program_name[PATHLENGTH];
static char program_args[2 * PATHLENGTH];
static pthread_t id;
static bool launched = false;
static int scale = 3;
static bool window_open = false;
static guint number = 11;

void toggle_open(GtkWidget *widget, gpointer data) {
	window_open = !window_open;
}

void * thread_body(void *arg) {
    launched = true;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Start the child process.
    if(!CreateProcess(program_name, program_args, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
      fprintf(stderr, "Couldn't create process");
    }
    // Wait until child process exits.
    WaitForSingleObject( pi.hProcess, INFINITE );

    // Close process and thread handles.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    pthread_exit(NULL);
}

static void add_item(GtkWidget *widget, GdkEvent *event, gpointer data) {
	if (window_open)
		return;
	
	char * type = (char *)data;
	GtkWidget *window;
	char title[13];
	
	strcpy(title, "Add ");
	strcat(title, type);
	
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), (const gchar *)title);
	gtk_window_set_default_size(GTK_WINDOW(window), 300, 100);
	
	GtkWidget *layout = gtk_layout_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(window), layout);
	gtk_widget_show(layout);
	
	GtkWindow *parent_window = GTK_WINDOW(window);
	GtkFileChooserNative *native = gtk_file_chooser_native_new("Select emulator executable", parent_window, GTK_FILE_CHOOSER_ACTION_OPEN, "Open", "Cancel");
	gint res = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
	
	if (res == GTK_RESPONSE_ACCEPT)
	{
		char *filename;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
		filename = gtk_file_chooser_get_filename(chooser);

		// Write to text file here.

		g_free(filename);
	}
	
	//gtk_widget_set_size_request(file_chooser, 100, 30);
	
	//gtk_layout_put(GTK_LAYOUT(layout), file_chooser, 20, 20);
	
	g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(toggle_open), NULL);
	gtk_widget_show_all (window);
	toggle_open(NULL, NULL);
}

void delete_item(GtkWidget *widget, gpointer data) {

}

void launch (GtkWidget *widget, gpointer *data) {
  int err = pthread_create(&id, NULL, thread_body, NULL);
  if (err) {
    fprintf(stderr, "Can't create thread with id %ld\n", id);
    exit(1);
  }
}

int create_window(int *argc, char **argv) {
	gtk_init (argc, &argv);
		
	int status;
	
	// Create Window
  GtkWidget *window;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Timed Launcher");
  gtk_window_set_default_size (GTK_WINDOW (window), 224 * scale, 160 * scale);

  // Interface
  GtkWidget *layout;
  GtkWidget *button;
  GtkWidget *program_combo;
  GtkWidget *input_combo;
  GtkWidget *image;
  GtkWidget *program_name;
  GtkWidget *input_name;
  GtkWidget *play;
  GtkWidget *add_emulator;
  GtkWidget *remove_emulator;
  GtkWidget *add_game;
  GtkWidget *remove_game;

  GError *error = NULL;

  const gchar *css_relpath = "styles.css";
  GFile *css_file = g_file_new_for_path(css_relpath);

  GtkCssProvider *cssProvider = gtk_css_provider_new();
  gtk_css_provider_load_from_file(cssProvider, css_file, &error);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(cssProvider), GTK_STYLE_PROVIDER_PRIORITY_USER);

  if (error) {
    g_warning("%s", error->message);
    g_clear_error(&error);
  }
  g_object_unref(css_file);

  layout = gtk_layout_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(window), layout);
  gtk_widget_show(layout);

  image = gtk_image_new_from_file("resources/bg.png");
  gtk_widget_set_size_request(image, 224 * scale, 160 * scale);

  program_combo = gtk_combo_box_text_new();
  gtk_widget_set_size_request(program_combo, 88 * scale, 24 * scale);
  gtk_widget_set_opacity(program_combo, 0);

  input_combo = gtk_combo_box_text_new();
  gtk_widget_set_size_request(input_combo, 88 * scale, 24 * scale);
  gtk_widget_set_opacity(input_combo, 0);

  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(program_combo), NULL, "NESTOPIA");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(program_combo), NULL, "RAIDEN VAN BRO");

  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(input_combo), NULL, "TETRIS");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(input_combo), NULL, "METROID");

  button = gtk_button_new_with_label ("play");
  gtk_widget_set_size_request(button, 112 * scale, 24 * scale);
  gtk_widget_set_opacity(button, 0);
  g_signal_connect (button, "clicked", G_CALLBACK (launch), NULL);

  program_name = gtk_label_new("NESTOPIA");

  input_name = gtk_label_new("TETRIS");

  play = gtk_label_new("PLAY");

  add_emulator = gtk_event_box_new();
  gtk_widget_set_size_request(add_emulator, 8 * scale, 8 * scale);
  gtk_widget_set_opacity(add_emulator, 1);
  g_signal_connect(GTK_CONTAINER(add_emulator), "button_press_event", G_CALLBACK (add_item), (gpointer)"emulator");

  remove_emulator = gtk_event_box_new();
  gtk_widget_set_size_request(remove_emulator, 8 * scale, 8 * scale);
  gtk_widget_set_opacity(remove_emulator, 1);
  g_signal_connect(GTK_CONTAINER(remove_emulator), "button_press_event", G_CALLBACK (delete_item), (gpointer)"emulator");
	
  add_game = gtk_event_box_new();
  gtk_widget_set_size_request(add_game, 8 * scale, 8 * scale);
  gtk_widget_set_opacity(add_game, 1);
  g_signal_connect(GTK_CONTAINER(add_game), "button_press_event", G_CALLBACK (add_item), (gpointer)"game");

  remove_game = gtk_event_box_new();
  gtk_widget_set_size_request(remove_game, 8 * scale, 8 * scale);
  gtk_widget_set_opacity(remove_game, 1);
  g_signal_connect(GTK_CONTAINER(add_game), "button_press_event", G_CALLBACK (delete_item), (gpointer)"game");

  gtk_layout_put(GTK_LAYOUT(layout), image, 0, 0);
  gtk_layout_put(GTK_LAYOUT(layout), program_combo, 16 * scale, 8 * scale);
  gtk_layout_put(GTK_LAYOUT(layout), input_combo, 120 * scale, 8 * scale);
  gtk_layout_put(GTK_LAYOUT(layout), button, 56 * scale, 48 * scale);
  gtk_layout_put(GTK_LAYOUT(layout), program_name, 24 * scale, 16 * scale);
  gtk_layout_put(GTK_LAYOUT(layout), input_name, 128 * scale, 16 * scale);
  gtk_layout_put(GTK_LAYOUT(layout), play, 96 * scale, 56 * scale);
  gtk_layout_put(GTK_LAYOUT(layout), add_emulator, 104 * scale, 8 * scale);
  gtk_layout_put(GTK_LAYOUT(layout), remove_emulator, 104 * scale, 24 * scale);
  gtk_layout_put(GTK_LAYOUT(layout), add_game, 208 * scale, 8 * scale);
  gtk_layout_put(GTK_LAYOUT(layout), remove_game, 208 * scale, 24 * scale);

  g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  gtk_widget_show_all (window);
}

int main(int argc, char** argv) {

    time_t start, end;
    int elapsed;

    /* Read the configuration file */

    char output[PATHLENGTH];

    FILE *config = fopen("tracker.cfg", "r");

    if (fgets(program_args, PATHLENGTH, config) != program_args)
        return 1;
    if (fgets(output, PATHLENGTH, config) != output)
        return 1;

    fclose(config);

    int length = strlen(program_args);
    if (program_args[length - 1] == '\n')
        program_args[length - 1] = 0;
    length = strlen(output);
    if (output[length - 1] == '\n')
        output[length - 1] = 0;


    /* Start program in another thread */
    
    strcpy(program_name, program_args);

    int r = 0;
    while (program_name[r] != ' ')
        r++;
    program_name[r] = 0;

		int status = create_window(&argc, argv);
		gtk_main();

    if (launched)
      pthread_join(id, NULL);

    /*
    time(&end);
    elapsed = difftime(end, start) / 60;

    int minutes = 0;
    FILE *out = fopen(output, "r");
    fscanf (out, "%d", &minutes);
    minutes += elapsed;

    freopen(output, "w", out);
    fprintf(out, "%d", minutes);

    fclose(out);
    */
    return status;
}
