/**************************************************************************/
/*  webview_impl_macos.h                                                  */
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

#pragma once

#ifdef WEBVIEW_MACOS

#include "core/string/ustring.h"
#include "core/os/mutex.h"

// Forward declarations for macOS types
#ifdef __OBJC__
@class WKWebView;
@class NSView;
@class NSWindow;
#else
typedef void WKWebView;
typedef void NSView;
typedef void NSWindow;
#endif

class WebViewEditor;

class WebViewImpl {
public:
	WebViewImpl(WebViewEditor *p_editor);
	~WebViewImpl();

	bool initialize(void *p_parent_handle);
	void destroy();
	
	void navigate(const String &p_url);
	void go_back();
	void go_forward();
	void reload();
	void stop();
	
	void set_visible(bool p_visible);
	void set_bounds(int x, int y, int width, int height);
	
	bool can_go_back();
	bool can_go_forward();
	String get_url();
	
	void execute_javascript(const String &p_script);

private:
	WebViewEditor *editor = nullptr;
	
	// macOS objects
	NSWindow *parent_window = nullptr;
	NSView *container_view = nullptr;
	WKWebView *webview = nullptr;
	
	// State
	bool initialized = false;
	bool visible = false;
	int bounds_x = 0, bounds_y = 0, bounds_width = 800, bounds_height = 600;
	
	// Pending navigation
	String pending_url;
	
	// Thread safety
	Mutex mutex;
	
	// Helper methods
	void update_bounds();
	void on_page_loaded(const String &p_url);
	void on_page_load_failed(const String &p_url, const String &p_error);
	void on_url_changed(const String &p_url);
	void on_title_changed(const String &p_title);
	void on_loading_state_changed(bool p_loading);
};

#endif // WEBVIEW_MACOS
