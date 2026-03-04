/**************************************************************************/
/*  webview_impl_windows.h                                                */
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

#ifdef WEBVIEW_WINDOWS

#include "core/string/ustring.h"
#include "core/os/mutex.h"

// Forward declaration for WebView2 wrapper
typedef void* WebView2Handle;
class WebViewEditor;

// WebView implementation for Windows using WebView2
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
	void release_focus();
	void process_messages();

private:
	WebViewEditor *editor = nullptr;
	WebView2Handle webview_handle = nullptr;
	
	// State
	bool initialized = false;
	bool visible = false;
	int bounds_left = 0, bounds_top = 0, bounds_right = 800, bounds_bottom = 600;
	
	String current_url;
	
	// Static callback functions
	static void on_navigation_starting(const char *url, void *user_data);
	static void on_navigation_completed(const char *url, int success, void *user_data);
	static void on_source_changed(const char *url, void *user_data);
	static void on_history_changed(int can_go_back, int can_go_forward, void *user_data);
};

#endif // WEBVIEW_WINDOWS
