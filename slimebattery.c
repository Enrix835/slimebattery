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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
 
/* #define DEBUG */
#define ACPI_CMD "acpi"
#define DEFAULT_ARRAY_SIZE 10
#define DEFAULT_TIME_UPDATE 2

gint opt_verbose = 0; 
gint opt_time = DEFAULT_TIME_UPDATE;
gint opt_other_theme = 0;
gint opt_popup = 0;

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

static void update_status_battery(Battery * battery);
static gboolean update_status_tray(Battery * battery);
static gchar * get_status_icon_name(Battery * battery);
static void create_tray_icon(Battery * battery);
static void create_tray_icon(Battery * battery);
static void parse_acpi_output(Battery * battery, gchar * acpi_output);
static char * get_acpi_output(const gchar * acpi_command);
static void print_usage(void);

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
	gchar * icon_name = get_status_icon_name(battery);
	gchar * acpi_out = get_acpi_output(ACPI_CMD);

	if(acpi_out == NULL) {
		g_error("unable to run '%s'.", ACPI_CMD);
	}
	
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
	
	gtk_status_icon_set_from_icon_name(battery->batteryTray.tray_icon,
		icon_name);
	
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
	gint i = 0, count = 0;
	gchar * t;
	gchar * values_array[DEFAULT_ARRAY_SIZE];
	
	int pos = strchr(acpi_output, ':') - acpi_output;
	t = strtok(acpi_output + pos + 1, ",");
	while(t != NULL) {
		values_array[i++] = t[0] == ' ' ? t + 1 : t;
		t = strtok(NULL, ",");
		count++;
	}
	for(i = 0; i < count; i++) {
		if ((values_array[i][strlen(values_array[i]) - 1]) == '\n') {
			values_array[i][strlen(values_array[i]) - 1] = '\0';
		}
	}
	
	battery->status = values_array[0];
	battery->percentage = atoi(values_array[1]);
	battery->extra = values_array[2];
}
	
static gchar * get_acpi_output(const gchar * acpi_command)
{
	gchar * output;
	GError * error;
	
	g_spawn_command_line_sync(acpi_command, &output, NULL, NULL, &error);
	return error != NULL ? output : NULL;
} 

static void print_usage(void)
{
	printf("usage: slimebattery <option> [...]\n"
		"  -h\t\tprint this help message\n"
		"  -c\t\tchange tray icon theme (chane if you miss default icon)\n"
		"  -v\t\ttray icon's tooltip will display extra info\n"
		"  -t <time>\tset default time interval in seconds\n");
	exit(EXIT_SUCCESS);
}

int main(int argc, char ** argv)
{
	Battery battery;
	gint c;
	
	while((c = getopt(argc, argv, ":t:cv:h")) != -1) {
		switch(c) {
			case 't':
				opt_time = atoi(optarg);
				break;
			case 'c':
				opt_other_theme = 1;
				break;
			case 'v':
				opt_verbose = 1;
				break;
			case 'h':
				print_usage();
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
