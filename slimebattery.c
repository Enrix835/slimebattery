/*
 * slimebattery, an extremly lightweight GTK+ battery tray icon 
 * Copyright (C) 2011  Enrico "Enrix835" Trotta
 *
 * <enrico{DOT}trt{AT}gmail{DOT}com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
 
/* #define DEBUG */
#define ACPI_CMD "acpi"
#define DEFAULT_ARRAY_SIZE 3
#define DEFAULT_TIME_UPDATE 2

gint opt_text_mode = 0;
gint opt_text_size;
gint opt_colors = 0;
gchar * text_color = "white";
gint opt_verbose = 0; 
gint opt_time = DEFAULT_TIME_UPDATE;
gint opt_other_theme = 0;

typedef enum batteryState {
	CHARGING,
	DISCHARGING,
    FULL,
} BatteryState;
	
typedef struct batteryTray {
	GtkStatusIcon * tray_icon;
	gchar * tooltip;
	gchar * icon;
} BatteryTray;

typedef struct battery {
	gchar * status;
	gint percentage;
	gchar * extra;
	BatteryTray batteryTray;
	BatteryState batteryState;
} Battery;

static gint indexof(gchar * string, gchar c);
static void update_status_battery(Battery * battery);
static gboolean update_status_tray(Battery * battery);
static gchar * get_status_icon_name(Battery * battery);
static void draw_status_icon_pixbuf(Battery * battery);
static void create_tray_icon(Battery * battery);
static void create_tray_icon(Battery * battery);
static void parse_acpi_output(Battery * battery, gchar * acpi_output);
static char * get_acpi_output(const gchar * acpi_command);
static void print_usage(void);

static gint indexof(gchar * string, gchar c)
{
	gchar * p = strchr(string, c);
	return p ? strchr(string, c) - string : -1;
}

static void update_status_battery(Battery * battery)
{
	if(strcmp(battery->status, "Charging") == 0)
		battery->batteryState = CHARGING;
	else if(strcmp(battery->status, "Discharging") == 0)
		battery->batteryState = DISCHARGING;
	else
		battery->batteryState = FULL;
}

static gboolean update_status_tray(Battery * battery)
{
	gchar * icon_name;
	gchar * acpi_out = get_acpi_output(ACPI_CMD);
        
	if(opt_text_mode == 0) icon_name = get_status_icon_name(battery);

	parse_acpi_output(battery, acpi_out);
	
	#ifdef DEBUG
		g_debug("battery status: %s", battery->status);
	#endif
	
	update_status_battery(battery);
		
	battery->batteryTray.tooltip = g_strdup_printf("%s (%d%%) %s", 
		battery->status, 
		battery->percentage,
		opt_verbose == 1 ? battery->extra : " ");
		
	gtk_status_icon_set_tooltip_text(battery->batteryTray.tray_icon,
		battery->batteryTray.tooltip);
	
	if(opt_text_mode == 0)
		gtk_status_icon_set_from_icon_name(battery->batteryTray.tray_icon,
			icon_name);
	else
		draw_status_icon_pixbuf(battery);
	
	return TRUE; 
}

static gchar * get_status_icon_name(Battery * battery)
{
	GString * icon_name = g_string_new(opt_other_theme == 1 ? 
		"battery" : "notification-battery" );
	
	if (battery->percentage < 20)
		g_string_append(icon_name, opt_other_theme == 1 ? "-caution" 
			: "-000");
	else if (battery->percentage < 40)
		g_string_append(icon_name, opt_other_theme == 1 ? "-low"
			: "-020");
	else if (battery->percentage < 80)
		g_string_append(icon_name, opt_other_theme == 1 ? "-good" 
			: "-060");
	else
		g_string_append(icon_name, opt_other_theme == 1 ? "-full"
			: "-100");
	
	if(battery->batteryState == CHARGING) {
		g_string_append(icon_name, opt_other_theme == 1 ? "-charging" 
			: "-plugged");
	}
	
	return icon_name->str;
}

static void create_tray_icon(Battery * battery) 
{
	battery->batteryTray.tray_icon = gtk_status_icon_new();
	battery->batteryTray.tooltip = "slimebattery";
	
	gtk_status_icon_set_tooltip(battery->batteryTray.tray_icon, 
		battery->batteryTray.tooltip);
	gtk_status_icon_set_visible(battery->batteryTray.tray_icon, 
		TRUE);
                
	update_status_tray(battery);
	g_timeout_add_seconds(opt_time, (GSourceFunc) update_status_tray, battery);
}

static void parse_acpi_output(Battery * battery, gchar * acpi_output)
{
	gint i = 0; 
	gchar * t;
	gchar ** values_array;
	
	if(strcmp(acpi_output, "") != 0) {
		int pos = indexof(acpi_output, ':');
		t = strtok(acpi_output + pos + 1, ",");
		
		values_array = malloc(DEFAULT_ARRAY_SIZE * sizeof(gchar));
		
		while(t != NULL) {
			values_array[i++] = t[0] == ' ' ? t + 1 : t;
			t = strtok(NULL, ",");
		}
		
		if(values_array[2][strlen(values_array[2]) - 1] == '\n') {
			values_array[2][strlen(values_array[2]) - 1] = '\0';
		}
		
		battery->status = values_array[0];
		battery->percentage = atoi(values_array[1]);
		battery->extra = values_array[2];
		
		free(values_array);
	} else {
	  battery->status = " ";
	  battery->percentage = 0;
	  battery->extra = " ";
	}
}
	
static gchar * get_acpi_output(const gchar * acpi_command)
{
	gchar * output;
	GError * error = NULL;
	
	g_spawn_command_line_sync(acpi_command, &output, NULL, NULL, &error);
	gint first_newline_index = indexof(output, '\n');
	output[first_newline_index] = '\0';
	
	return error == NULL ? output : NULL;
} 

static void print_usage(void)
{
	printf("usage: slimebattery <option> [...]\n"
		"  --help\t\t\tprint this help message\n"
		"  --change-icon\t\t\tchange tray icon theme (chane if you miss default icon)\n"
		"  --verbose\t\t\ttray icon's tooltip will display extra info\n"
		"  --colors\t\t\t(only in text-mode) change text color (green when it's full, red when it's low)\n"
		"  --text-mode <font size>\tshow battery status in plain text mode\n"
		"  --interval <time>\t\tcheck battery status at every specified seconds\n");
	exit(EXIT_SUCCESS);
}

static void draw_status_icon_pixbuf(Battery * battery)
{
	GError * error = NULL;
	GdkPixbuf * pixbuf;
	GdkPixbufLoader * loader = gdk_pixbuf_loader_new_with_type("svg", &error);
	
	if(opt_colors == 1) {
		if(battery->percentage < 20) {
			text_color = "red";
		} else if(battery->percentage > 80) {
			text_color = "green";
		} 
	}
	
	gchar * svg_percentage_signed = g_strdup_printf(
	"<svg width=\"100px\" height=\"100px\" viewbox=\"0 0 200 200\">"
	"<text style=\"text-align:center;\" x=\"0\" y=\"60\" font-weight=\"bold\" fill=\"%s\" font-family=\"Sans\" font-size=\"%d\">%d%</text>"
	"</svg>"
	, text_color, opt_text_size, battery->percentage);
	const gint len_svg_percentage_signed = strlen(svg_percentage_signed);
	
	gdk_pixbuf_loader_write(loader, (const guchar *) svg_percentage_signed, len_svg_percentage_signed, &error);
	gdk_pixbuf_loader_close(loader, &error);

	g_free(svg_percentage_signed);
	
	pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
	gtk_status_icon_set_from_pixbuf(battery->batteryTray.tray_icon, pixbuf);
}

int main(int argc, char ** argv)
{
	Battery battery;
        
	gint opt, opt_index = 0;
	static struct option long_options[] = {
			{ "change-icon", 0, 0, 0 },
			{ "text-mode", 1, 0, 0 },
			{ "verbose", 0, 0, 0 },
			{ "colors", 0, 0, 0 },
			{ "interval", 1, 0, 0 },
			{ "help", 0, 0, 0 },
			{ 0, 0, 0, 0 }
	};

	while((opt = getopt_long(argc, argv, " ", long_options, &opt_index)) != -1) {
		switch(opt) {
			/* options don't have short args */
			case 0:
				if(strcmp("change-icon", long_options[opt_index].name) == 0) {
					opt_other_theme = 1;
				} else if(strcmp("text-mode", long_options[opt_index].name) == 0) {
					opt_text_mode = 1;
					opt_text_size = atoi(optarg);
				} else if(strcmp("colors", long_options[opt_index].name) == 0) {
					if(opt_text_mode == 1) {
						opt_colors = 1;
					} else {
						g_warning("you must run %s in text-mode to use --colors option!\n", argv[0]);
					}
				} else if(strcmp("verbose", long_options[opt_index].name) == 0) {
					opt_verbose = 1;
				} else if(strcmp("interval", long_options[opt_index].name) == 0) {
					opt_time = atoi(optarg);
				} else if(strcmp("help", long_options[opt_index].name) == 0) {
					print_usage();
				}
				break;
				
			default:
				print_usage();
		}
	}

	gtk_init(&argc, &argv);
	create_tray_icon(&battery);
	gtk_main();
	
	return 0;
}
