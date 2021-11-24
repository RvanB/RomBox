#pragma once

#ifdef DEFINE_GLOBALS

#define GLOBAL_VAR(type, name, init) extern type name ; type name = init

#else

#define GLOBAL_VAR(type, name, init) extern type name

#endif

struct widgets {
	GtkWidget *layout;
	GtkWidget *play;
	GtkWidget *button;
	GtkWidget *program_name;
	GtkWidget *input_name;
	GtkWidget *program_combo;
	GtkWidget *input_combo;
	GtkWidget *playtime;
	GtkWidget *config_button;
	GtkWidget *config_label;
};
struct configuration {
	double scale;
	cJSON *systems;
	cJSON *emulators;
};

GLOBAL_VAR(struct widgets *, widgets, NULL);
GLOBAL_VAR(struct configuration *, config, NULL);
GLOBAL_VAR(cJSON *, json, NULL);