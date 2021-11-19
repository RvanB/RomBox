/* RomBox by Raiden van Bronkhorst
 * Created September 2020
 */

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <pango/pango.h>
#include <glib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>

#include "configure.h"
#include "cJSON.h"

#define DEFINE_GLOBALS
#include "globals.h"

/* Prototypes */
int load_data();
char * display_time(int minutes);
void toggle_sensitive(GtkWidget *interactive_widget, GtkWidget *label);

/* Global variables */
static pthread_t id;
static pthread_t timer_id;
static pthread_cond_t increment_time = PTHREAD_COND_INITIALIZER;
static char * exe_dir;
static cJSON *json;
static char * css_format;

/* Structures */


typedef struct Configuration {
	double scale;
	cJSON *systems;
	cJSON *emulators;
} Configuration;
Configuration config;

typedef struct State {
	bool playing;
	bool launched;
	int system_id;
	cJSON *selected_emulator;
	cJSON *selected_rom;
} State;
State state = { .playing = false, .launched = false, .system_id = -1 };

void cleanup() {
	if (state.playing) {	
		pthread_cancel(id);
		pthread_cancel(timer_id);
	}
	
	if (widgets != NULL)
		free(widgets);
	
	cJSON_Delete(json);
	
	if (exe_dir != NULL)
		free(exe_dir);
}

char * path_rel_to_abs(char * relative) {
	char * abs = (char *) malloc(sizeof(char) * 1024);

	strcpy(abs, exe_dir);
	strcat(abs, "/");
	strcat(abs, relative);
	return abs;
}

void store_data(char * data) {
	char * path = path_rel_to_abs("config.json");
	FILE *f = fopen(path, "wb");
	
	if (f != NULL) {
		fputs(data, f);
		fclose(f);
	}
	free(path);
}

void * timer_body(void *arg) {
	
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(widgets->input_combo));
	
	cJSON *rom = state.selected_rom;
	cJSON *playtime_obj = cJSON_GetObjectItemCaseSensitive(rom, "playtime");
	int loaded_minutes = cJSON_GetNumberValue(playtime_obj);
	
	char *time_str = display_time(loaded_minutes);
	gtk_label_set_text(GTK_LABEL(widgets->playtime), time_str);
	free(time_str);
	
	int minutes = loaded_minutes;
	time_t start, end;
	time(&start);
	
	while (state.playing) {
		
		Sleep(1000 * 60);

		if (state.playing) {
			minutes++;
			
			time_str = display_time(minutes);
			
			gtk_label_set_text(GTK_LABEL(widgets->playtime), time_str);
			
			free(time_str);
			
			cJSON_SetNumberValue(playtime_obj, minutes);
			char * data = cJSON_Print(json);
			if (data != NULL) {
				store_data(data);
			}
	
			
			time(&end);
		}
	}
	
	int elapsed = difftime(end, start) / 60;
	
	cJSON_SetNumberValue(playtime_obj, loaded_minutes + elapsed);
	char * data = cJSON_Print(json);
	if (data != NULL) {
		store_data(data);
	}
	pthread_exit(NULL);
	
}

void quit() {
	
	pthread_cond_signal(&increment_time);
	
	gtk_main_quit();
}

void * thread_body(void *arg) {
	struct thread_info {
		char * args;
	};
	struct thread_info * info = (struct thread_info *)arg;
	char * args = info->args;

	state.launched = true;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	gtk_label_set_text(GTK_LABEL(widgets->play), "PLAYING");

	gtk_layout_move(GTK_LAYOUT(widgets->layout), widgets->play, 88 * config.scale, 56 * config.scale);

	// Start the child process.
	if(!CreateProcess(NULL, args, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		fprintf(stderr, "Couldn't create process");
	}
	free(info);
	
	// Wait until child process exits.
	WaitForSingleObject( pi.hProcess, INFINITE );

	state.playing = false;
	
	toggle_sensitive(widgets->program_combo, widgets->program_name);
	toggle_sensitive(widgets->input_combo, widgets->input_name);
	toggle_sensitive(widgets->config_button, widgets->config_label);
	
	gtk_label_set_text(GTK_LABEL(widgets->play), "PLAY");

	gtk_layout_move(GTK_LAYOUT(widgets->layout), widgets->play, 96 * config.scale, 56 * config.scale);
	
	
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


/*
static void edit(GtkWidget *widget, GdkEvent *event, gpointer data) {
	//if (window_open)
	//	return;
	
	char * type = (char *)data;
	GtkWidget *window;
	char title[14];
	
	strcpy(title, "Edit ");
	strcat(title, type);
	strcat(title, "s");
	
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
*/
void delete_item(GtkWidget *widget, gpointer data) {

}

void toggle_sensitive(GtkWidget *interactive_widget, GtkWidget *label) {
	gboolean sensitivity = gtk_widget_get_sensitive(interactive_widget);
	if (sensitivity == TRUE) {
		gtk_widget_set_sensitive(interactive_widget, FALSE);
		gtk_widget_set_name(label, "disabledlabel");
	} else {
		gtk_widget_set_sensitive(interactive_widget, TRUE);
		gtk_widget_set_name(label, "whitelabel");
	}
}


void launch (GtkWidget *widget, gpointer data) {
	if (state.playing) {
		return;
	}

	char *launch_args = (char *)malloc(sizeof(char) * 1024);
	
	cJSON *emulator = state.selected_emulator;
	cJSON *rom = state.selected_rom;
	
	if (emulator != NULL && rom != NULL) {
		cJSON *emulator_path_obj = cJSON_GetObjectItemCaseSensitive(emulator, "path");
		char * emulator_path = cJSON_GetStringValue(emulator_path_obj);
		
		cJSON *emulator_flags_obj = cJSON_GetObjectItemCaseSensitive(emulator, "flags");
		char * emulator_flags = cJSON_GetStringValue(emulator_flags_obj);
		
		cJSON *rom_path_obj = cJSON_GetObjectItemCaseSensitive(rom, "path");
		char * rom_path = cJSON_GetStringValue(rom_path_obj);
		
		
		
		strcpy(launch_args, "\"");
		strcat(launch_args, emulator_path);
		strcat(launch_args, "\" ");
		strcat(launch_args, emulator_flags);
		strcat(launch_args, " \"");
		strcat(launch_args, rom_path);
		strcat(launch_args, "\"");
		
		/* Launch emulator with rom */
		
		struct thread_info {
			char * args;
		};
		struct thread_info * info = (struct thread_info *)malloc(sizeof(struct thread_info));
		info->args = launch_args;
		
		toggle_sensitive(widgets->program_combo, widgets->program_name);
		toggle_sensitive(widgets->input_combo, widgets->input_name);
		toggle_sensitive(widgets->config_button, widgets->config_label);
		
		
		state.playing = true;
		
		int err = pthread_create(&id, NULL, thread_body, (void *)info);
		
		if (err) {
			fprintf(stderr, "Can't create thread with id %ld\n", id);
			exit(1);
		}
		
		/* Start timer thread */
		err = pthread_create(&timer_id, NULL, timer_body, NULL);
		
		if (err) {
			fprintf(stderr, "Can't create thread with id %ld\n", timer_id);
			exit(1);
		}
	} else {
		g_print("No program or emulator selected\n");
	}
		
	free(launch_args);
}


int load_data() {
	gchar *contents;
	GError *err = NULL;
	
	char * path = path_rel_to_abs("config.json");
	if (g_file_get_contents(path, &contents, NULL, &err)) { 
		json = cJSON_Parse(contents);
		
		if (json == NULL) {
			const char *error_ptr = cJSON_GetErrorPtr();
			if (error_ptr != NULL)
				g_printerr("Error before: %s\n", error_ptr);
		}
		
		cJSON *scaleObj = cJSON_GetObjectItemCaseSensitive(json, "scale");
		config.scale = cJSON_GetNumberValue(scaleObj);
		
		config.systems = cJSON_GetObjectItemCaseSensitive(json, "systems");
		const cJSON *system = NULL;
		
		config.emulators = cJSON_GetObjectItemCaseSensitive(json, "emulators");
	}
	free(path);
}

char * display_time(int minutes) {
	int hours = minutes / 60;
	int days = hours / 24;
	hours -= days * 24;
	minutes = minutes - days * 24 * 60 - hours * 60;
	char * output = (char *)malloc(sizeof(char) * 9);
	if (output == NULL) {
		perror("malloc");
		exit(0);
	}
	snprintf(output, 9, "%0*d:%0*d:%0*d", 2, days, 2, hours, 2, minutes);
	return output;
}


void update_program_combo(GtkWidget *program_combo) {
	cJSON *emulator = NULL;
	
	int emulator_count = 0;
	
	cJSON_ArrayForEach(emulator, config.emulators) {
		cJSON *nameObj = cJSON_GetObjectItemCaseSensitive(emulator, "name");
		char * name = cJSON_GetStringValue(nameObj);
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(program_combo), NULL, name);
		
		emulator_count ++;
	}
	if (emulator_count > 0) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(program_combo), 0);
	}
}

void update_selected(GtkWidget *combo, gpointer data) {
	
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
	
	GtkWidget *label;
	
	char * time = display_time(0);
	
	if (combo == widgets->program_combo) {
		/* Program combo */
		
		label = widgets->program_name;
		
		/* Change options in input combo */
		
		cJSON *emulator = cJSON_GetArrayItem(config.emulators, index);
		state.selected_emulator = emulator;
		
		cJSON *system_id_obj = cJSON_GetObjectItemCaseSensitive(emulator, "system");
		int system_id = cJSON_GetNumberValue(system_id_obj);
		
		if (system_id != state.system_id) {
			
			/* Clear the combo box */
			gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(widgets->input_combo));
			
			cJSON *system = cJSON_GetArrayItem(config.systems, system_id);
			cJSON *roms = cJSON_GetObjectItemCaseSensitive(system, "roms");
			
			cJSON *rom = NULL;
			
			int rom_count = 0;
			
			cJSON_ArrayForEach(rom, roms) {
				cJSON *nameObj = cJSON_GetObjectItemCaseSensitive(rom, "name");
				char * name = cJSON_GetStringValue(nameObj);
				gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widgets->input_combo), NULL, name);
				
				rom_count ++;
			}
			
			gtk_label_set_text(GTK_LABEL(widgets->playtime), time);
			free(time);
			
			state.system_id = system_id;
			
			if (rom_count > 0) {
				gtk_combo_box_set_active(GTK_COMBO_BOX(widgets->input_combo), 0);
				
				update_selected(widgets->input_combo, data);
			}
		}
	} else {
		/* Input combo */
		
		label = widgets->input_name;
		
		/* Load playtime for this rom */
		cJSON *system = cJSON_GetArrayItem(config.systems, state.system_id);
		cJSON *roms = cJSON_GetObjectItemCaseSensitive(system, "roms");
		
		cJSON *rom = cJSON_GetArrayItem(roms, index);
		state.selected_rom = rom;
		
		cJSON *playtime_obj = cJSON_GetObjectItemCaseSensitive(rom, "playtime");
		int playtime = cJSON_GetNumberValue(playtime_obj);
			
		time = display_time(playtime);
		
		gtk_label_set_text(GTK_LABEL(widgets->playtime), time);
		free(time);
	}
	char * text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
	if (text != NULL) {
		if (strlen(text) > 9) {
			text[9] = 0;
		}
		gtk_label_set_text(GTK_LABEL(label), text);
	}
	
}

char * load_css_with_font_size(char * relpath, double font_size) {
	gchar *css_path = path_rel_to_abs(relpath);
	
	char * template;
	
	char * css = (char *) malloc(sizeof(char) * 1024);
	
	GError *err = NULL;
	if (g_file_get_contents(css_path, &template, NULL, &err)) {
		snprintf(css, 1024, template, font_size);
	} else {
		perror("g_file_get_contents");
	}
	return css;
}

int create_window() {
		
	int status;
	
	/* Create window */
	GtkWidget *window;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "RomBox");
	gtk_window_set_default_size (GTK_WINDOW (window), 224 * config.scale, 160 * config.scale);
	gtk_window_set_resizable(GTK_WINDOW(window), false);
	
	/* Create font descriptor */
	PangoAttrList *font_attr_list = pango_attr_list_new();
	PangoFontDescription *df = pango_font_description_new();
	pango_font_description_set_family(df, "Tetris");
	pango_font_description_set_absolute_size(df, 7 * config.scale * PANGO_SCALE);
	PangoAttribute *attr = pango_attr_font_desc_new(df);
	pango_attr_list_insert(font_attr_list, attr);
	
	/* Layout */
	GtkWidget *layout = gtk_layout_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(window), layout);
	
	gtk_widget_show(layout);

	widgets->layout = layout;
	
	/* Background image */
	char * path = path_rel_to_abs("resources/template.png");
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(path, NULL);
	GtkWidget *image = gtk_image_new();
	free(path);
	
	GdkPixbuf *pixbuf_scaled = gdk_pixbuf_scale_simple(pixbuf, 224 * config.scale, 160 * config.scale, GDK_INTERP_NEAREST);
	gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf_scaled);
	
	g_object_unref(pixbuf_scaled);
	
	/* Playtime label */
	GtkWidget *playtime = gtk_label_new(NULL);
	gtk_label_set_attributes(GTK_LABEL(playtime), font_attr_list);
	gtk_widget_set_name(playtime, "whitelabel");
	widgets->playtime = playtime;
	
	/* Program name label */
	GtkWidget *program_name = gtk_label_new(NULL);
	gtk_label_set_attributes(GTK_LABEL(program_name), font_attr_list);
	gtk_widget_set_name(program_name, "whitelabel");
	widgets->program_name = program_name;
	
	/* Input name label */
	GtkWidget *input_name = gtk_label_new(NULL);
	gtk_label_set_attributes(GTK_LABEL(input_name), font_attr_list);
	gtk_widget_set_name(input_name, "whitelabel");
	widgets->input_name = input_name;

	/* Program combo */
	GtkWidget *program_combo = gtk_combo_box_text_new();
	gtk_widget_set_size_request(program_combo, 72 * config.scale, 24 * config.scale);
	gtk_widget_set_opacity(program_combo, 0);
	gtk_combo_box_set_popup_fixed_width(GTK_COMBO_BOX(program_combo), TRUE);
	
	widgets->program_combo = program_combo;

	/* Input combo */
	GtkWidget *input_combo = gtk_combo_box_text_new();
	gtk_widget_set_size_request(input_combo, 72 * config.scale, 24 * config.scale);
	gtk_widget_set_opacity(input_combo, 0);
	gtk_combo_box_set_popup_fixed_width(GTK_COMBO_BOX(input_combo), TRUE);
	
	widgets->input_combo = input_combo;
	
	/* Update combo boxes */
	update_program_combo(program_combo);
	update_selected(program_combo, widgets);
	update_selected(input_combo, widgets);
	
	/* Event handlers for combo boxes */
	g_signal_connect(program_combo, "changed", G_CALLBACK(update_selected), NULL);
	g_signal_connect(input_combo, "changed", G_CALLBACK(update_selected), NULL);
	
	/* CSS */
	GError *error = NULL;
	
	char * css = load_css_with_font_size("styles.css", 7.0 * config.scale);

	GtkCssProvider *cssProvider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(cssProvider, css, -1, &error);
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(cssProvider), GTK_STYLE_PROVIDER_PRIORITY_USER);

	free(css);

	/* Play button */
	GtkWidget *play = gtk_label_new("PLAY");
	gtk_label_set_attributes(GTK_LABEL(play), font_attr_list);
	gtk_widget_set_name(play, "whitelabel");
	widgets->play = play;

	GtkWidget *button = gtk_button_new_with_label ("play");
	gtk_widget_set_size_request(button, 112 * config.scale, 24 * config.scale);
	gtk_widget_set_opacity(button, 0);
	widgets->button = button;
	
	g_signal_connect (button, "clicked", G_CALLBACK (launch), NULL);
	
	/* Options button */
	GtkWidget *config_label = gtk_label_new("CONF");
	gtk_label_set_attributes(GTK_LABEL(config_label), font_attr_list);
	gtk_widget_set_name(config_label, "whitelabel");
	widgets->config_label = config_label;
	
	GtkWidget *config_button = gtk_button_new_with_label("config");
	gtk_widget_set_size_request(config_button, 48 * config.scale, 24 * config.scale);
	gtk_widget_set_opacity(config_button, 0);
	widgets->config_button = config_button;

	g_signal_connect(GTK_CONTAINER(config_button), "clicked", G_CALLBACK (configure), NULL);
	
	/* Add widgets to layout */
	
	gtk_layout_put(GTK_LAYOUT(layout), image, 0, 0);
	
	gtk_layout_put(GTK_LAYOUT(layout), program_name, 24 * config.scale, 16 * config.scale);
	gtk_layout_put(GTK_LAYOUT(layout), program_combo, 16 * config.scale, 8 * config.scale);
	
	gtk_layout_put(GTK_LAYOUT(layout), input_name, 128 * config.scale, 16 * config.scale);
	gtk_layout_put(GTK_LAYOUT(layout), input_combo, 120 * config.scale, 8 * config.scale);
	
	gtk_layout_put(GTK_LAYOUT(layout), play, 96 * config.scale, 56 * config.scale);
	gtk_layout_put(GTK_LAYOUT(layout), button, 56 * config.scale, 48 * config.scale);
	
	gtk_layout_put(GTK_LAYOUT(layout), config_label, 16 * config.scale, 128 * config.scale);
	gtk_layout_put(GTK_LAYOUT(layout), config_button, 8 * config.scale, 120 * config.scale);
	
	gtk_layout_put(GTK_LAYOUT(layout), playtime, 80 * config.scale, 112 * config.scale);

	gtk_widget_show_all (window);

	g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(quit), NULL);
	
	/* Free font stuff */
	pango_font_description_free(df);
	pango_attr_list_unref(font_attr_list);

	if (error) {
		g_warning("%s", error->message);
		g_clear_error(&error);
	}
	
	return 0;
}

int main(int argc, char** argv) {
	
	exe_dir = (char *) calloc(sizeof(char), 1024);
	
	GetModuleFileName(NULL, exe_dir, 1024);
	int len = strlen(exe_dir);
	exe_dir[len - strlen("RomBox.exe") - 1] = 0;
	
	time_t start, end;
	int elapsed;
	
	widgets = (struct widgets *)malloc(sizeof(struct widgets));
	if (widgets == NULL) {
		perror("malloc");
		exit(0);
	}
	
	/* Load data from files */
	load_data();
	
	gtk_init (&argc, &argv);
	int status = create_window();
	gtk_main();
	
	cleanup();
	return status;
}
