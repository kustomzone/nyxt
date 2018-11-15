/*
Copyright © 2018 Atlas Engineer LLC.
Use of this file is governed by the license that can be found in LICENSE.
*/
#define APPNAME "Next"

#include <webkit2/webkit2.h>
#include "buffer.h"
#include "minibuffer.h"

#include "server-state.h"

typedef struct {
	GtkWidget *base;
	Buffer *buffer;
	const char *identifier;
	Minibuffer *minibuffer;
	int minibuffer_height;
} Window;

void window_destroy_callback(GtkWidget *_widget, Window *window) {
	// TODO: We need to communicate the kill to the Lisp core.
	g_debug("Delete window %s", window->identifier);
	akd_remove_object_for_key(state.windows, window->identifier);
	// We don't kill the buffer since it may be used by other windows.
	minibuffer_delete(window->minibuffer);

	if (g_hash_table_size(state.windows->dict) >= 1) {
		return;
	}

	// No more windows, let's quit.
	// TODO: This is dirty, since it could interupt the request response of
	// server_window_delete.  We probably need add an "quit" request to the API.
	g_debug("No more windows, quitting");
	gtk_main_quit();
}

void window_delete(Window *window) {
	// We need to destroy the widget here, then clean up in
	// window_destroy_callback.  We don't destroy in window_destroy_callback since
	// it's already done.
	g_debug("Destroy window widget %s", window->identifier);
	gtk_widget_destroy(window->base);
	window_destroy_callback(NULL, window);
}

// TODO: Not needed?
void window_close_web_view_callback(WebKitWebView *_web_view, Window *window) {
	g_debug("Closing web view");
	window_delete(window);
}

Window *window_init() {
	Buffer *buffer = buffer_init();
	// Make sure that when the browser area becomes visible, it will get
	// mouse and keyboard events
	gtk_widget_grab_focus(GTK_WIDGET(buffer->web_view));

	Minibuffer *minibuffer = minibuffer_init();

	GtkWidget *mainbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_end(GTK_BOX(mainbox), GTK_WIDGET(buffer->web_view), TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(mainbox), GTK_WIDGET(minibuffer->web_view), FALSE, FALSE, 0);

	Window *window = calloc(1, sizeof (Window));
	// Create an 800x600 window that will contain the browser instance
	window->base = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window->base), 800, 600);
	// TODO: Make title customizable from Lisp.
	gtk_window_set_title(GTK_WINDOW(window->base), APPNAME);
	// TODO: Deprecated?
	/* gtk_window_set_wmclass(GTK_WINDOW(main_window), APPNAME, APPNAME); */

	// Put the browser area into the main window
	gtk_container_add(GTK_CONTAINER(window->base), mainbox);

	// Set up callbacks so that if either the main window or the browser
	// instance is closed, it is handled properly.
	g_signal_connect(window->base, "destroy", G_CALLBACK(window_destroy_callback), window);
	g_signal_connect(buffer->web_view, "close", G_CALLBACK(window_close_web_view_callback), window);

	// Make sure the main window and all its contents are visible
	gtk_widget_show_all(window->base);

	window->buffer = buffer;
	window->minibuffer = minibuffer;
	return window;
}

void window_set_active_buffer(Window *window, Buffer *buffer) {
	window->buffer = buffer;
	gtk_container_add(GTK_CONTAINER(window->base), GTK_WIDGET(buffer->web_view));
}

gint64 window_set_minibuffer_height(Window *window, gint64 height) {
	gtk_widget_set_size_request(GTK_WIDGET(window->minibuffer), -1, height);
	window->minibuffer_height = height;

	gint natural_height;
	gtk_widget_get_preferred_height(GTK_WIDGET(window->minibuffer), NULL, &natural_height);
	return natural_height;
}
