/**************************************************************************/
/*  webview2_wrapper.h                                                    */
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

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle for WebView2 instance
typedef void* WebView2Handle;

// Callback function types
typedef void (*WebView2NavigationStartingCallback)(const char* url, void* user_data);
typedef void (*WebView2NavigationCompletedCallback)(const char* url, int success, void* user_data);
typedef void (*WebView2SourceChangedCallback)(const char* url, void* user_data);
typedef void (*WebView2HistoryChangedCallback)(int can_go_back, int can_go_forward, void* user_data);

// Create and destroy WebView2
WebView2Handle webview2_create(void* parent_hwnd, void* user_data);
void webview2_destroy(WebView2Handle handle);

// Navigation
void webview2_navigate(WebView2Handle handle, const char* url);
void webview2_go_back(WebView2Handle handle);
void webview2_go_forward(WebView2Handle handle);
void webview2_reload(WebView2Handle handle);
void webview2_stop(WebView2Handle handle);

// Getters
int webview2_can_go_back(WebView2Handle handle);
int webview2_can_go_forward(WebView2Handle handle);
void webview2_get_url(WebView2Handle handle, char* buffer, int buffer_size);

// Setters
void webview2_set_visible(WebView2Handle handle, int visible);
void webview2_set_bounds(WebView2Handle handle, int x, int y, int width, int height);

// Callbacks
void webview2_set_navigation_starting_callback(WebView2Handle handle, WebView2NavigationStartingCallback callback);
void webview2_set_navigation_completed_callback(WebView2Handle handle, WebView2NavigationCompletedCallback callback);
void webview2_set_source_changed_callback(WebView2Handle handle, WebView2SourceChangedCallback callback);
void webview2_set_history_changed_callback(WebView2Handle handle, WebView2HistoryChangedCallback callback);

// Execute JavaScript
void webview2_execute_javascript(WebView2Handle handle, const char* script);

// Process messages (should be called regularly)
void webview2_process_messages(WebView2Handle handle);

// Release focus back to parent window
void webview2_release_focus(WebView2Handle handle);

#ifdef __cplusplus
}
#endif
