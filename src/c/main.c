#include <pebble.h>

#define BATTERY_WIDTH 24
#define BATTERY_HEIGHT 14

static Window *s_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_battery_text_layer;
static GFont s_time_font;
static GFont s_date_font;
static int s_battery_level;
static Layer *s_battery_layer;
static BitmapLayer *s_bt_icon_layer;
static GBitmap *s_bt_icon_bitmap;

static BitmapLayer *s_qt_icon_layer;
static GBitmap *s_qt_icon_bitmap;

char *month[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
char *weekday[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};


//static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
//	text_layer_set_text(s_time_layer, "Center button");
//}
//
//static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
//	text_layer_set_text(s_time_layer, "^^");
//}
//
//static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
//	text_layer_set_text(s_time_layer, "VV");
//}

//static void prv_click_config_provider(void *context) {
//	window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
//	window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
//	window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
//}


static void prv_window_unload(Window *window) {
	text_layer_destroy(s_time_layer);
	text_layer_destroy(s_date_layer);
	gbitmap_destroy(s_bt_icon_bitmap);
	gbitmap_destroy(s_qt_icon_bitmap);
	bitmap_layer_destroy(s_bt_icon_layer);
	bitmap_layer_destroy(s_qt_icon_layer);
	fonts_unload_custom_font(s_time_font);
	fonts_unload_custom_font(s_date_font);
	layer_destroy(s_battery_layer);
}


static void update_time() {
	// Get a tm structure
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);

	// Write the current hours and minutes into a buffer
	static char s_buffer[8];
	strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
					     "%H:%M" : "%I:%M", tick_time);

	// Display this time on the TextLayer
	text_layer_set_text(s_time_layer, s_buffer);


	static char s_wd_buffer[11];
	snprintf(s_wd_buffer, sizeof(s_wd_buffer), "%s %d %s", weekday[tick_time->tm_wday], tick_time->tm_mday, month[tick_time->tm_mon]);

	text_layer_set_text(s_date_layer, s_wd_buffer);
}

static void update_quiet_time() {
	layer_set_hidden(bitmap_layer_get_layer(s_qt_icon_layer), !quiet_time_is_active());
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time();
	update_quiet_time();
}

static void battery_callback(BatteryChargeState state) {
	// Record the new battery level
	s_battery_level = state.charge_percent;
	// Update meter
	layer_mark_dirty(s_battery_layer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
	GRect bounds = layer_get_bounds(layer);

	// Find the width of the bar (total width = 114px)
	int width = (s_battery_level * (BATTERY_WIDTH - 4)) / 100;

//
//	static char s_buffer[8];
//	snprintf(s_buffer, sizeof(s_buffer), ">%d", width);
//	s_buffer[7] = '\0';
//
//	// Display this time on the TextLayer
//	text_layer_set_text(layer, s_buffer);
//
	// Draw frame
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, bounds, 0, GCornerNone);


	// Draw inner frame
	graphics_context_set_fill_color(ctx, GColorWhite);
	bounds.origin.x++;
	bounds.origin.y++;
	bounds.size.h = bounds.size.h - 2;
	bounds.size.w = bounds.size.w - 2;
	graphics_fill_rect(ctx, bounds, 0, GCornerNone);



	// Draw the bar
	bounds.origin.x++;
	bounds.origin.y++;
	bounds.size.h = bounds.size.h - 2;
//	bounds.size.w = bounds.size.w-2;
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, GRect(bounds.origin.x, bounds.origin.y, width, bounds.size.h), 0, GCornerNone);

	static char s_buffer[4] = {};
	snprintf(s_buffer, sizeof(s_buffer), "%d%%", s_battery_level);

	text_layer_set_text(s_battery_text_layer, s_buffer);
}

static void bluetooth_callback(bool connected) {
	// Show icon if connected
	layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), !connected);

	if (!connected) {
		// Issue a vibrating alert
		vibes_double_pulse();
	}
}

static void prv_window_load(Window *window) {
	// Get information about the Window
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
//	s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_ROBOTO_CONDENSED_52));
	s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_ROBOTO_BOLD_50));
	s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_ROBOTO_LIGHT_20));
//	s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DEJAVU_SANS_MONO_46));
//	s_small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DEJAVU_SANS_MONO_10));

	// Create the TextLayer with specific bounds
	s_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(58, 54), bounds.size.w, 54));
	s_date_layer = text_layer_create(GRect(0, 144, bounds.size.w, 54));
	s_battery_text_layer = text_layer_create(GRect(84, 1, 30, 18));

	// Improve the layout to be more like a watchface
	text_layer_set_background_color(s_time_layer, GColorClear);
	text_layer_set_text_color(s_time_layer, GColorBlack);
	text_layer_set_background_color(s_date_layer, GColorClear);
	text_layer_set_text_color(s_date_layer, GColorBlack);
	text_layer_set_background_color(s_battery_text_layer, GColorClear);
	text_layer_set_text_color(s_battery_text_layer, GColorBlack);


//	text_layer_set_text(s_date_layer, "FRI 15 SEP");
	text_layer_set_font(s_battery_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
//	text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
	text_layer_set_font(s_time_layer, s_time_font);
	text_layer_set_font(s_date_layer, s_date_font);
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

	// Add it as a child layer to the Window's root layer
	layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
	layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
	layer_add_child(window_layer, text_layer_get_layer(s_battery_text_layer));

	// Initial update time on screen
	update_time();

	// Create battery meter Layer
	s_battery_layer = layer_create(GRect(114, 6, BATTERY_WIDTH, BATTERY_HEIGHT));
//	s_battery_text_layer = layer_create(GRect(90, 4, 30, 12));
	layer_set_update_proc(s_battery_layer, battery_update_proc);

	// Create the Bluetooth icon GBitmap
	s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_ICON);

	// Create the Quiet Time icon
	s_qt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_QT_ICON);

	// Create the BitmapLayer to display the GBitmap
	s_bt_icon_layer = bitmap_layer_create(GRect(4, 4, 20, 20));
	bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));

	// Show the correct state of the BT connection from the start
	bluetooth_callback(connection_service_peek_pebble_app_connection());

	// Create the BitmapLayer to display the Qiuet time GBitmap
	s_qt_icon_layer = bitmap_layer_create(GRect(24, 4, 16, 16));
	bitmap_layer_set_bitmap(s_qt_icon_layer, s_qt_icon_bitmap);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_qt_icon_layer));

	// Update Quiet Time icon
	update_quiet_time();

	// Add to Window
	layer_add_child(window_get_root_layer(window), s_battery_layer);
}


static void prv_init(void) {
	s_window = window_create();
//	window_set_click_config_provider(s_window, prv_click_config_provider);
	window_set_window_handlers(s_window, (WindowHandlers) {
		.load = prv_window_load,
		.unload = prv_window_unload,
	});
	const bool animated = true;
	window_stack_push(s_window, animated);

	// Register with TickTimerService
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	battery_state_service_subscribe(battery_callback);
	battery_callback(battery_state_service_peek());
	connection_service_subscribe((ConnectionHandlers) {
		.pebble_app_connection_handler = bluetooth_callback
	});

}

static void prv_deinit(void) {
	window_destroy(s_window);
}

int main(void) {
	prv_init();
//	resource_init_current_app(&APP_RESOURCES);

//	setlocale(LC_ALL, "ru_RU");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

	app_event_loop();
	prv_deinit();
}
