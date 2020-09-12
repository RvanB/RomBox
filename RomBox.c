/* RomBox by Raiden van Bronkhorst
 * Created September 2020
 */

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include <stdbool.h>

/* Prototypes */

int load_data(GtkWidget *program_combo, GtkWidget *input_combo);

/* Global variables */

static char ** emulator_paths;
static char ** game_paths;
static pthread_t id;
static bool launched = false;
static int scale = 3;
static bool window_open = false;
static bool loaded = false;
static int playtime = 0;
static int nEmulators = 0;
static int nGames = 0;

void free_paths() {
  for (int i = 0; i < nEmulators + 1; i++) {
    free(emulator_paths[i]);
  }
  for (int i = 0; i < nGames + 1; i++) {
    free(game_paths[i]);
  }

  free(emulator_paths);
  free(game_paths);

}

void toggle_open(GtkWidget *widget, gpointer data) {
	window_open = !window_open;
}

void * thread_body(void *arg) {
    char * arg_str = (char *)arg;
    launched = true;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Start the child process.
    if(!CreateProcess(NULL, arg_str, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
      fprintf(stderr, "Couldn't create process");
    }
    // Wait until child process exits.
    WaitForSingleObject( pi.hProcess, INFINITE );

    // Close process and thread handles.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    pthread_exit(NULL);
}

char * select_file(GtkWindow *parent) {
  GtkFileChooserNative *native = gtk_file_chooser_native_new("Select emulator executable", parent, GTK_FILE_CHOOSER_ACTION_OPEN, "Open", "Cancel");
	gint res = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
	
	if (res == GTK_RESPONSE_ACCEPT)
	{
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
		return gtk_file_chooser_get_filename(chooser);
	}
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
	gtk_window_set_default_size(GTK_WINDOW(window), 400, 250);
	
	GtkWidget *layout = gtk_layout_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(window), layout);
	gtk_widget_show(layout);

  GtkWidget *path = gtk_entry_new();
  gtk_widget_set_size_request(path, 200, 30);
  gtk_layout_put(GTK_LAYOUT(layout), path, 20, 20);

  GtkWidget *browse = gtk_button_new_with_label("Browse");
  gtk_widget_set_size_request(browse, 50, 30);
  gtk_layout_put(GTK_LAYOUT(layout), browse, 225, 20);
  //g_signal_connect (browse, "clicked", G_CALLBACK (launch), NULL);

	
	//gtk_widget_set_size_request(file_chooser, 100, 30);
	
	//gtk_layout_put(GTK_LAYOUT(layout), file_chooser, 20, 20);
	
	g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(toggle_open), NULL);
	gtk_widget_show_all (window);
	toggle_open(NULL, NULL);
}

void delete_item(GtkWidget *widget, gpointer data) {

}

void launch (GtkWidget *widget, gpointer data) {
  
   int err = pthread_create(&id, NULL, thread_body, (void *)data);

  if (err) {
    fprintf(stderr, "Can't create thread with id %ld\n", id);
    exit(1);
  }
}

int count_lines(FILE* open_file) {
    // Count lines
    int lines = 0;
    char c;
    while (!feof(open_file)) {
      c = fgetc(open_file);
      if (c == '\n')
        lines++;
    }
    return lines + 1;
}

int refresh_data(GtkWidget *program_combo, GtkWidget *input_combo) {
  if (!loaded)
    return 1;
  free_paths();
  gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(program_combo));
  gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(input_combo));
  load_data(program_combo, input_combo);
  return 0;
}

int load_data(GtkWidget *program_combo, GtkWidget *input_combo) {
  // Load emulators
  FILE *emulators;
  if (emulators = fopen("data/emulators.txt", "r")) {
    nEmulators = count_lines(emulators) - 1;
    rewind(emulators);
    emulator_paths = (char **)malloc(sizeof(char *) * (nEmulators + 1));
    for (int i = 0; i < nEmulators + 1; i++) {
			emulator_paths[i] = (char *)malloc(200 * sizeof(char));
      char buffer[200] = {0};
			
      if (fgets(buffer, 200, emulators) != buffer)
        return 1;
      
      // Skip line if it's the header row or if item is hidden
      if (i == 0 || buffer[0] == 'N')
        continue;

      int r = 2;
      while (buffer[r] != '\t')
        r++;
      buffer[r] = 0;
      char * name = buffer + 2;

      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(program_combo), NULL, name);

      char * path = buffer + r + 1;
      
      strcpy(emulator_paths[i], path);
    }
  } else {
    fopen("data/emulators.txt", "w");
  }
  fclose(emulators);

  // Load games and playtime
  FILE *games;
  if (games = fopen("data/roms.txt", "r")) {
    nGames = count_lines(games) - 1;
    rewind(games);
    game_paths = (char **)malloc(sizeof(char *) * (nGames + 1));

    for (int i = 0; i < nGames + 1; i++) {
			game_paths[i] = (char *)malloc(200 * sizeof(char));
      char buffer[200] = {0};
			
      if (fgets(buffer, 200, games) != buffer)
        return 1;

      if (i == 0 || buffer[0] == 'N')
        continue;

      int r = 2;
      while (buffer[r] != '\t')
        r++;
      buffer[r] = 0;
      char * name = buffer + 2;
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(input_combo), NULL, name);

      char * path = buffer + r + 1;
      
      strcpy(game_paths[i], path);

      int r1 = r + 1;
      while (buffer[r1] != '\t')
        r1++;
      buffer[r1] = 0;
      char * playtime = buffer + r1 + 1;
			
    }
  } else {
    fopen("data/games.txt", "w");
  }
  fclose(games);

  loaded = true;
	
	if (nEmulators > 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(program_combo), 0);
  
	if (nGames > 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(input_combo), 0);
	
  return 0;
	
	
}

void update_combo(GtkWidget *combo, gpointer data) {
	char * str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
  
	GtkWidget *label = (GtkWidget *)data;
	gtk_label_set_text(GTK_LABEL(label), str);
	
	g_free(str);
}

int create_window(int *argc, char **argv) {
	gtk_init (argc, &argv);
		
	int status;
	
	// Create Window
  GtkWidget *window;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "RomBox");
  gtk_window_set_default_size (GTK_WINDOW (window), 224 * scale, 160 * scale);

	
  GtkWidget *program_name = gtk_label_new(NULL);
  GtkWidget *input_name = gtk_label_new(NULL);

  GtkWidget *program_combo = gtk_combo_box_text_new();
  gtk_widget_set_size_request(program_combo, 88 * scale, 24 * scale);
  gtk_widget_set_opacity(program_combo, 0);
	g_signal_connect(program_combo, "changed", G_CALLBACK(update_combo), (gpointer)program_name);

  GtkWidget *input_combo = gtk_combo_box_text_new();
  gtk_widget_set_size_request(input_combo, 88 * scale, 24 * scale);
  gtk_widget_set_opacity(input_combo, 0);
	g_signal_connect(input_combo, "changed", G_CALLBACK(update_combo), (gpointer)input_name);
	
  GError *error = NULL;

  const gchar *css_relpath = "styles.css";
  GFile *css_file = g_file_new_for_path(css_relpath);

  GtkCssProvider *cssProvider = gtk_css_provider_new();
  gtk_css_provider_load_from_file(cssProvider, css_file, &error);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(cssProvider), GTK_STYLE_PROVIDER_PRIORITY_USER);


  g_object_unref(css_file);

  GtkWidget *layout = gtk_layout_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(window), layout);
  gtk_widget_show(layout);

  GtkWidget *image = gtk_image_new_from_file("resources/bg.png");
  gtk_widget_set_size_request(image, 224 * scale, 160 * scale);

  GtkWidget *button = gtk_button_new_with_label ("play");
  gtk_widget_set_size_request(button, 112 * scale, 24 * scale);
  gtk_widget_set_opacity(button, 0);

	// Load data from files
	load_data(program_combo, input_combo);

  // Construct args string from program name and input paths
  char args[1024] = {0};
	
  char *program_name_str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(program_combo));
  char *input_name_str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(input_combo));
	
	if (program_name_str != NULL && input_name_str != NULL)
 
	g_free(program_name_str);
  g_free(input_name_str);
 
 /*
  strcpy(args, program_name_str);
  strcat(args, " ");
  strcat(args, input_name_str);



  
  
  */

  g_signal_connect (button, "clicked", G_CALLBACK (launch), NULL);


  GtkWidget *play = gtk_label_new("PLAY");

  GtkWidget *add_emulator = gtk_event_box_new();
  gtk_widget_set_size_request(add_emulator, 8 * scale, 8 * scale);
  gtk_widget_set_opacity(add_emulator, 1);
  g_signal_connect(GTK_CONTAINER(add_emulator), "button_press_event", G_CALLBACK (add_item), (gpointer)"emulator");

  GtkWidget *remove_emulator = gtk_event_box_new();
  gtk_widget_set_size_request(remove_emulator, 8 * scale, 8 * scale);
  gtk_widget_set_opacity(remove_emulator, 1);
  g_signal_connect(GTK_CONTAINER(remove_emulator), "button_press_event", G_CALLBACK (delete_item), (gpointer)"emulator");
	
  GtkWidget *add_game = gtk_event_box_new();
  gtk_widget_set_size_request(add_game, 8 * scale, 8 * scale);
  gtk_widget_set_opacity(add_game, 1);
  g_signal_connect(GTK_CONTAINER(add_game), "button_press_event", G_CALLBACK (add_item), (gpointer)"game");

  GtkWidget *remove_game = gtk_event_box_new();
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

  if (error) {
    g_warning("%s", error->message);
    g_clear_error(&error);
  }
}

int main(int argc, char** argv) {

  time_t start, end;
  int elapsed;

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
  free_paths();
  
  return status;
}
