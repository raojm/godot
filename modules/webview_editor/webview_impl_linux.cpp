/**************************************************************************/
/*  webview_impl_linux.cpp                                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifdef WEBVIEW_LINUX

#include "webview_impl_linux.h"
#include "webview_editor.h"

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <gdk/gdkx.h>

WebViewImpl::WebViewImpl(WebViewEditor *p_editor) :
		editor(p_editor) {
}

WebViewImpl::~WebViewImpl() {
	destroy();
}

bool WebViewImpl::initialize(void *p_parent_handle) {
	MutexLock lock(mutex);

	if (initialized) {
		return true;
	}

	// Initialize GTK if not already done
	if (!gtk_init_check(nullptr, nullptr)) {
		ERR_PRINT("WebViewImpl: Failed to initialize GTK");
		return false;
	}

	// Get parent window handle
	GdkWindow *gdk_window = (GdkWindow *)p_parent_handle;
	if (!gdk_window) {
		ERR_PRINT("WebViewImpl: Invalid parent window");
		return false;
	}

	// Create container widget
	container_widget = gtk_event_box_new();
	if (!container_widget) {
		ERR_PRINT("WebViewImpl: Failed to create container widget");
		return false;
	}

	// Create WebKitWebView
	WebKitSettings *settings = webkit_settings_new();
	webkit_settings_set_enable_javascript(settings, TRUE);
	webkit_settings_set_enable_webgl(settings, TRUE);
	webkit_settings_set_enable_media_stream(settings, TRUE);
	webkit_settings_set_enable_developer_extras(settings, TRUE);

	webview = WEBKIT_WEB_VIEW(webkit_web_view_new_with_settings(settings));
	if (!webview) {
		ERR_PRINT("WebViewImpl: Failed to create WebKitWebView");
		g_object_unref(settings);
		return false;
	}

	g_object_unref(settings);

	// Add webview to container
	gtk_container_add(GTK_CONTAINER(container_widget), GTK_WIDGET(webview));

	// Connect signals
	g_signal_connect(webview, "load-changed", G_CALLBACK(on_load_changed), this);
	g_signal_connect(webview, "notify::title", G_CALLBACK(on_title_changed), this);
	g_signal_connect(webview, "notify::uri", G_CALLBACK(on_uri_changed), this);

	// Get parent GTK window
	parent_window = GTK_WINDOW(gdk_window_get_user_data(gdk_window));
	if (!parent_window) {
		ERR_PRINT("WebViewImpl: Failed to get parent window");
		return false;
	}

	// Add container to parent window
	gtk_container_add(GTK_CONTAINER(parent_window), container_widget);

	// Set initial bounds
	update_bounds();

	// Show widgets
	gtk_widget_show_all(container_widget);
	gtk_widget_set_visible(container_widget, visible);

	initialized = true;

	// Navigate to pending URL if any
	if (!pending_url.is_empty()) {
		String url = pending_url;
		pending_url.clear();
		navigate(url);
	}

	return true;
}

void WebViewImpl::destroy() {
	MutexLock lock(mutex);

	if (!initialized) {
		return;
	}

	// Remove from parent
	if (container_widget && parent_window) {
		gtk_container_remove(GTK_CONTAINER(parent_window), container_widget);
	}

	// Release objects
	if (webview) {
		g_object_unref(webview);
		webview = nullptr;
	}

	if (container_widget) {
		g_object_unref(container_widget);
		container_widget = nullptr;
	}

	parent_window = nullptr;
	initialized = false;
}

void WebViewImpl::navigate(const String &p_url) {
	MutexLock lock(mutex);

	if (!initialized || !webview) {
		pending_url = p_url;
		return;
	}

	const char *url = p_url.utf8().get_data();
	webkit_web_view_load_uri(webview, url);
}

void WebViewImpl::go_back() {
	MutexLock lock(mutex);

	if (!initialized || !webview) {
		return;
	}

	webkit_web_view_go_back(webview);
}

void WebViewImpl::go_forward() {
	MutexLock lock(mutex);

	if (!initialized || !webview) {
		return;
	}

	webkit_web_view_go_forward(webview);
}

void WebViewImpl::reload() {
	MutexLock lock(mutex);

	if (!initialized || !webview) {
		return;
	}

	webkit_web_view_reload(webview);
}

void WebViewImpl::stop() {
	MutexLock lock(mutex);

	if (!initialized || !webview) {
		return;
	}

	webkit_web_view_stop_loading(webview);
}

void WebViewImpl::set_visible(bool p_visible) {
	MutexLock lock(mutex);

	visible = p_visible;

	if (container_widget) {
		gtk_widget_set_visible(container_widget, p_visible);
	}
}

void WebViewImpl::set_bounds(int x, int y, int width, int height) {
	MutexLock lock(mutex);

	bounds_x = x;
	bounds_y = y;
	bounds_width = width;
	bounds_height = height;

	update_bounds();
}

void WebViewImpl::update_bounds() {
	if (container_widget) {
		GtkAllocation allocation;
		allocation.x = bounds_x;
		allocation.y = bounds_y;
		allocation.width = bounds_width;
		allocation.height = bounds_height;
		gtk_widget_size_allocate(container_widget, &allocation);
	}
}

bool WebViewImpl::can_go_back() {
	MutexLock lock(mutex);

	if (!initialized || !webview) {
		return false;
	}

	return webkit_web_view_can_go_back(webview);
}

bool WebViewImpl::can_go_forward() {
	MutexLock lock(mutex);

	if (!initialized || !webview) {
		return false;
	}

	return webkit_web_view_can_go_forward(webview);
}

String WebViewImpl::get_url() {
	MutexLock lock(mutex);

	if (!initialized || !webview) {
		return String();
	}

	const char *uri = webkit_web_view_get_uri(webview);
	if (uri) {
		return String::utf8(uri);
	}

	return String();
}

void WebViewImpl::execute_javascript(const String &p_script) {
	MutexLock lock(mutex);

	if (!initialized || !webview) {
		return;
	}

	const char *script = p_script.utf8().get_data();
	webkit_web_view_run_javascript(webview, script, nullptr, nullptr, nullptr);
}

void WebViewImpl::on_load_changed(WebKitWebView *web_view, int load_event, gpointer user_data) {
	WebViewImpl *self = static_cast<WebViewImpl *>(user_data);
	if (!self) {
		return;
	}

	switch (load_event) {
		case WEBKIT_LOAD_STARTED:
			self->on_loading_state_changed(true);
			break;
		case WEBKIT_LOAD_FINISHED: {
			const char *uri = webkit_web_view_get_uri(web_view);
			if (uri) {
				self->on_page_loaded(String::utf8(uri));
			}
			break;
		}
		case WEBKIT_LOAD_FAILED: {
			const char *uri = webkit_web_view_get_uri(web_view);
			if (uri) {
				self->on_page_load_failed(String::utf8(uri), "Load failed");
			}
			break;
		}
	}
}

void WebViewImpl::on_title_changed(WebKitWebView *web_view, GParamSpec *pspec, gpointer user_data) {
	WebViewImpl *self = static_cast<WebViewImpl *>(user_data);
	if (!self || !self->editor) {
		return;
	}

	const char *title = webkit_web_view_get_title(web_view);
	if (title) {
		self->on_title_changed(String::utf8(title));
	}
}

void WebViewImpl::on_uri_changed(WebKitWebView *web_view, GParamSpec *pspec, gpointer user_data) {
	WebViewImpl *self = static_cast<WebViewImpl *>(user_data);
	if (!self || !self->editor) {
		return;
	}

	const char *uri = webkit_web_view_get_uri(web_view);
	if (uri) {
		self->on_url_changed(String::utf8(uri));
	}
}

void WebViewImpl::on_page_loaded(const String &p_url) {
	if (editor) {
		editor->on_page_loaded(p_url);
	}
}

void WebViewImpl::on_page_load_failed(const String &p_url, const String &p_error) {
	if (editor) {
		editor->on_page_load_failed(p_url, p_error);
	}
}

void WebViewImpl::on_url_changed(const String &p_url) {
	if (editor) {
		editor->on_url_changed(p_url);
	}
}

void WebViewImpl::on_title_changed(const String &p_title) {
	if (editor) {
		editor->on_title_changed(p_title);
	}
}

void WebViewImpl::on_loading_state_changed(bool p_loading) {
	if (editor) {
		editor->on_loading_state_changed(p_loading);
	}
}

#endif // WEBVIEW_LINUX
