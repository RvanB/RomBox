/* RomBox by Raiden van Bronkhorst
 * Created September 2020
 */

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>

/* Structures */
struct widgets {
  GtkWidget *program_name;
  GtkWidget *input_name;
  GtkWidget *program_combo;
  GtkWidget *input_combo;
  GtkWidget *playtime;
};

/* Prototypes */

int load_data(GtkWidget *program_combo, GtkWidget *input_combo);
char * display_time(int minutes);

/* Global variables */

static char ** emulator_paths;
static char ** game_paths;
static int * playtimes;
static pthread_t id;
static pthread_t timer_id;
static bool launched = false;
static int scale = 3;
static bool window_open = false;
static bool loaded = false;
static int nEmulators = 0;
static int nGames = 0;
static bool playing = false;
static pthread_cond_t increment_time = PTHREAD_COND_INITIALIZER;

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

void update_file_playtime(struct widgets * widgets, int game_index, int minutes) {

  FILE* games;
  if (games = fopen("data/roms.txt", "rb+")) {
    
    int line_counter = 0;
    char buffer[200] = {0};
    while (line_counter < game_index + 1) {
      fgets(buffer, 200, games);
      line_counter++;
    }
    
    char c;
    for (int i = 0; i < 3; i++) {
      while (c != '\t')
        c = fgetc(games);
      c = fgetc(games);
    }
    int playtime_location = ftell(games) - 1;
    
    while (c != '\n' && c != EOF)
      c = fgetc(games);
    c = fgetc(games);
    int read_start = ftell(games) - 1;
    
    int digits;
    if (minutes == 0)
      digits = 1;
    else
      digits = 1 + (int)(log10((double)minutes));
    char new_minutes[digits + 2];
    snprintf(new_minutes, digits + 2, "%d\n", minutes);
    
    if (game_index < nGames - 1) {
      fseek(games, 0, SEEK_END);
      long size = ftell(games) - read_start;
      fseek(games, read_start, SEEK_SET);
      
      char *temp = (char *)malloc(size + 1);
      fread(temp, 1, size, games);
      
      temp[size] = 0;
      
      fseek(games, playtime_location, SEEK_SET);
      if (playing) {
        fwrite(new_minutes, sizeof(char), digits + 1, games);
        fwrite(temp, 1, size, games);
      }
      free(temp);
    } else {
      fseek(games, playtime_location, SEEK_SET);
      if (playing) {
        fwrite(new_minutes, sizeof(char), digits + 1, games);
      }
    }
    
    
    fclose(games);
  }
}

int custom_wait(int seconds) {
  pthread_mutex_t fakeMutex = PTHREAD_MUTEX_INITIALIZER;
  
  struct timespec timeToWait;
  struct timeval now;
  int rt;
  
  mingw_gettimeofday(&now, NULL);
  
  timeToWait.tv_sec = now.tv_sec + seconds;
  
  pthread_mutex_lock(&fakeMutex);
  rt = pthread_cond_timedwait(&increment_time, &fakeMutex, &timeToWait);
  pthread_mutex_unlock(&fakeMutex);
  return rt;
}

void * timer_body(void *arg) {
  
  struct widgets * widgets = (struct widgets *)arg;
  
  int game_index = gtk_combo_box_get_active(GTK_COMBO_BOX(widgets->input_combo));
  
  int loaded_minutes = playtimes[game_index + 1];
  
  char *time_str = display_time(loaded_minutes);
  gtk_label_set_text(GTK_LABEL(widgets->playtime), time_str);
  free(time_str);
  
  int minutes = loaded_minutes;
  time_t start, end;
  time(&start);
  
  while (playing) {
    
    custom_wait(60);
    
    if (playing) {
      minutes++;
      
      time_str = display_time(minutes);
      
      gtk_label_set_text(GTK_LABEL(widgets->playtime), time_str);

      playtimes[game_index + 1] = minutes;
        
      free(time_str);
      
      update_file_playtime(widgets, game_index, minutes);
    }
    
  }
  
  time(&end);
  int elapsed = difftime(end, start) / 60;
  update_file_playtime(widgets, game_index, loaded_minutes + elapsed);
  pthread_exit(NULL);
  
}

void quit() {
  pthread_cond_signal(&increment_time);
  gtk_main_quit();
}

void * thread_body(void *arg) {
  char * args = (char *)arg;
  launched = true;
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  playing = true;
  
  // Start the child process.
  if(!CreateProcess(NULL, args, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
    fprintf(stderr, "Couldn't create process");
  }
  
  // Wait until child process exits.
  WaitForSingleObject( pi.hProcess, INFINITE );

  playing = false;
  
  pthread_join(timer_id, NULL);
  
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
    
  struct widgets * widgets = (struct widgets *)data;  
    
  // Construct args string from program name and input paths

  char *launch_args = (char *)malloc(sizeof(char) * 1024);
  int program_index = gtk_combo_box_get_active(GTK_COMBO_BOX(widgets->program_combo));
  int input_index = gtk_combo_box_get_active(GTK_COMBO_BOX(widgets->input_combo));

  if (program_index > -1 && input_index > -1) {
    
    char * program_str = emulator_paths[program_index + 1];
    
    char * input_str = game_paths[input_index + 1];

    strcpy(launch_args, program_str);
    strcat(launch_args, " ");
    strcat(launch_args, input_str);
    
    int err = pthread_create(&id, NULL, thread_body, (void *)launch_args);
    
    if (err) {
      fprintf(stderr, "Can't create thread with id %ld\n", id);
      exit(1);
    }
    
    err = pthread_create(&timer_id, NULL, timer_body, (void *)widgets);
      
    if (err) {
      fprintf(stderr, "Can't create thread with id %ld\n", timer_id);
      exit(1);
    }
    
  } else {
    g_print("No program or emulator selected\n");
  }
    
  free(launch_args);

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
    int nDisabled = 0;
    emulator_paths = (char **)malloc(sizeof(char *) * (nEmulators + 1));
    for (int i = 0; i < nEmulators + 1; i++) {
      emulator_paths[i] = (char *)malloc(200 * sizeof(char));
      char buffer[200] = {0};
      
      if (fgets(buffer, 200, emulators) != buffer)
        return 1;
      
      // Skip line if it's the header row or if item is hidden
      if (i == 0)
        continue;
      
      if (buffer[0] == 'N') {
        nDisabled++;
        continue;
      }

      int r = 2;
      while (buffer[r] != '\t')
        r++;
      buffer[r] = 0;
      char * name = buffer + 2;

      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(program_combo), NULL, name);

      char * path = buffer + r + 1;
      
      int path_length = strlen(path);
      if (path[path_length - 1] == '\n')
        path[path_length - 1] = 0;
      
      strcpy(emulator_paths[i - nDisabled], path);
      
    }
    fclose(emulators);
  } else {
    fopen("data/emulators.txt", "w");
  }
  

  // Load games and playtime
  FILE *games;
  if (games = fopen("data/roms.txt", "r")) {
    nGames = count_lines(games) - 1;
    rewind(games);
    int nDisabled = 0;
    game_paths = (char **)malloc(sizeof(char *) * (nGames + 1));
    playtimes = (int *)malloc(sizeof(int) * (nGames + 1));
    for (int i = 0; i < nGames + 1; i++) {
      game_paths[i] = (char *)malloc(200 * sizeof(char));
      char buffer[200] = {0};
      
      if (fgets(buffer, 200, games) != buffer)
        return 1;

      if (i == 0)
        continue;
      
      if (buffer[0] == 'N') {
        nDisabled++;
        continue;
      }

      int r = 2;
      while (buffer[r] != '\t')
        r++;
      buffer[r] = 0;
      char * name = buffer + 2;
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(input_combo), NULL, name);

      char * path = buffer + r + 1;
      
      int path_length = strlen(path);
      if (path[path_length - 1] == '\n')
        path[path_length - 1] = 0;
      
      strcpy(game_paths[i - nDisabled], path);

      int r1 = r + 1;
      while (buffer[r1] != '\t')
        r1++;
      buffer[r1] = 0;
      char * playtime = buffer + r1 + 1;
      playtimes[i - nDisabled] = atoi(playtime);
      
    }
    fclose(games);
  } else {
    fopen("data/games.txt", "w");
  }
  loaded = true;
  
  return 0;
}

char * display_time(int minutes) {
  int hours = minutes / 60;
  int days = hours / 24;
  hours -= days * 24;
  minutes = minutes - days * 24 * 60 - hours * 60;
  char * output = (char *)malloc(sizeof(char) * 9);
  snprintf(output, 9, "%0*d:%0*d:%0*d", 2, days, 2, hours, 2, minutes);
  return output;
}

void update_combo(GtkWidget *combo, gpointer data) {
  struct widgets * widgets = (struct widgets *)data;
  
  char * str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
  
  GtkWidget *label;
  if (combo == widgets->program_combo) {
    label = widgets->program_name;
  } else {
    label = widgets->input_name;
    int i = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    
    char *time;
    if (str == NULL)
       time = display_time(0);
    else 
       time = display_time(playtimes[i + 1]);
    gtk_label_set_text(GTK_LABEL(widgets->playtime), time);
    free(time);
    
  }
  gtk_label_set_text(GTK_LABEL(label), str);
  
  
  
  g_free(str);
}

int create_window(struct widgets *widgets) {
    
  int status;
  
  // Create Window
  GtkWidget *window;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "RomBox");
  gtk_window_set_default_size (GTK_WINDOW (window), 224 * scale, 160 * scale);
  
  GtkWidget *playtime = gtk_label_new(NULL);
  
  widgets->playtime = playtime;
  
  GtkWidget *program_name = gtk_label_new(NULL);
  GtkWidget *input_name = gtk_label_new(NULL);
  
  widgets->program_name = program_name;
  widgets->input_name = input_name;

  GtkWidget *program_combo = gtk_combo_box_text_new();
  gtk_widget_set_size_request(program_combo, 88 * scale, 24 * scale);
  gtk_widget_set_opacity(program_combo, 0);
  
  widgets->program_combo = program_combo;

  GtkWidget *input_combo = gtk_combo_box_text_new();
  gtk_widget_set_size_request(input_combo, 88 * scale, 24 * scale);
  gtk_widget_set_opacity(input_combo, 0);
  
  widgets->input_combo = input_combo;
  
  g_signal_connect(program_combo, "changed", G_CALLBACK(update_combo), (gpointer)widgets);
  g_signal_connect(input_combo, "changed", G_CALLBACK(update_combo), (gpointer)widgets);
  
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
  
  if (nEmulators > 0)
    gtk_combo_box_set_active(GTK_COMBO_BOX(program_combo), 0);
  
  if (nGames > 0)
    gtk_combo_box_set_active(GTK_COMBO_BOX(input_combo), 0);
 
  g_signal_connect (button, "clicked", G_CALLBACK (launch), (gpointer)widgets);

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
  gtk_layout_put(GTK_LAYOUT(layout), playtime, 80 * scale, 112 * scale);

  g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(quit), NULL);
  gtk_widget_show_all (window);

  if (error) {
    g_warning("%s", error->message);
    g_clear_error(&error);
  }
}

int main(int argc, char** argv) {
  
  time_t start, end;
  int elapsed;
  
  struct widgets * widgets = (struct widgets *)malloc(sizeof(struct widgets));

  gtk_init (&argc, &argv);
  int status = create_window(widgets);
  gtk_main();

  if (launched)
    pthread_join(id, NULL);
  
  free(widgets);
  free(playtimes);
  free_paths();
  return status;
}
