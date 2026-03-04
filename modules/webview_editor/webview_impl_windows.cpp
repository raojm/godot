/**************************************************************************/
/*  webview_impl_windows.cpp                                              */
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

#ifdef WEBVIEW_WINDOWS

#include "webview_impl_windows.h"
#include "webview_editor.h"
#include "webview2_wrapper.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "core/error/error_macros.h"

// ============================================================================
// Constructor / Destructor
// ============================================================================

WebViewImpl::WebViewImpl(WebViewEditor* p_editor) : editor(p_editor) {
}

WebViewImpl::~WebViewImpl() {
    destroy();
}

// ============================================================================
// Initialization
// ============================================================================

bool WebViewImpl::initialize(void* p_parent_handle) {
    if (initialized) {
        return true;
    }

    if (!p_parent_handle) {
        ERR_PRINT("WebViewImpl: Invalid parent window handle");
        return false;
    }

    // Create WebView2 using the wrapper
    webview_handle = webview2_create(p_parent_handle, this);
    if (!webview_handle) {
        ERR_PRINT("WebViewImpl: Failed to create WebView2");
        return false;
    }

    // Set up callbacks for navigation events
    webview2_set_navigation_starting_callback(webview_handle, on_navigation_starting);
    webview2_set_navigation_completed_callback(webview_handle, on_navigation_completed);
    webview2_set_source_changed_callback(webview_handle, on_source_changed);
    webview2_set_history_changed_callback(webview_handle, on_history_changed);

    initialized = true;
    return true;
}

void WebViewImpl::destroy() {
    if (webview_handle) {
        webview2_destroy(webview_handle);
        webview_handle = nullptr;
    }
    initialized = false;
}

// ============================================================================
// Navigation
// ============================================================================

void WebViewImpl::navigate(const String& p_url) {
    if (!initialized || !webview_handle) {
        return;
    }

    current_url = p_url;
    CharString url_utf8 = p_url.utf8();
    webview2_navigate(webview_handle, url_utf8.get_data());
}

void WebViewImpl::go_back() {
    if (!initialized || !webview_handle) {
        return;
    }
    webview2_go_back(webview_handle);
}

void WebViewImpl::go_forward() {
    if (!initialized || !webview_handle) {
        return;
    }
    webview2_go_forward(webview_handle);
}

void WebViewImpl::reload() {
    if (!initialized || !webview_handle) {
        return;
    }
    webview2_reload(webview_handle);
}

void WebViewImpl::stop() {
    if (!initialized || !webview_handle) {
        return;
    }
    webview2_stop(webview_handle);
}

// ============================================================================
// Visibility and Bounds
// ============================================================================

void WebViewImpl::set_visible(bool p_visible) {
    visible = p_visible;
    if (webview_handle) {
        webview2_set_visible(webview_handle, p_visible ? 1 : 0);
    }
}

void WebViewImpl::set_bounds(int x, int y, int width, int height) {
    bounds_left = x;
    bounds_top = y;
    bounds_right = x + width;
    bounds_bottom = y + height;

    if (webview_handle) {
        webview2_set_bounds(webview_handle, x, y, width, height);
    }
}

// ============================================================================
// State Queries
// ============================================================================

bool WebViewImpl::can_go_back() {
    if (!initialized || !webview_handle) {
        return false;
    }
    return webview2_can_go_back(webview_handle) != 0;
}

bool WebViewImpl::can_go_forward() {
    if (!initialized || !webview_handle) {
        return false;
    }
    return webview2_can_go_forward(webview_handle) != 0;
}

String WebViewImpl::get_url() {
    if (!webview_handle) {
        return current_url;
    }

    char buffer[2048];
    webview2_get_url(webview_handle, buffer, sizeof(buffer));
    return String::utf8(buffer);
}

// ============================================================================
// JavaScript Execution
// ============================================================================

void WebViewImpl::execute_javascript(const String& p_script) {
    if (!initialized || !webview_handle) {
        return;
    }

    CharString script_utf8 = p_script.utf8();
    webview2_execute_javascript(webview_handle, script_utf8.get_data());
}

// ============================================================================
// Focus Management
// ============================================================================

void WebViewImpl::release_focus() {
    if (!initialized || !webview_handle) {
        return;
    }
    webview2_release_focus(webview_handle);
}

void WebViewImpl::process_messages() {
    if (!initialized || !webview_handle) {
        return;
    }
    webview2_process_messages(webview_handle);
}

// ============================================================================
// Callbacks from WebView2
// ============================================================================

void WebViewImpl::on_navigation_starting(const char* url, void* user_data) {
    WebViewImpl* impl = static_cast<WebViewImpl*>(user_data);
    if (impl && impl->editor) {
        impl->editor->on_url_changed(String::utf8(url));
        impl->editor->on_loading_state_changed(true);
    }
}

void WebViewImpl::on_navigation_completed(const char* url, int success, void* user_data) {
    WebViewImpl* impl = static_cast<WebViewImpl*>(user_data);
    if (impl && impl->editor) {
        if (success) {
            impl->editor->on_loading_state_changed(false);
        } else {
            impl->editor->on_page_load_failed(String::utf8(url), "Navigation failed");
        }
    }
}

void WebViewImpl::on_source_changed(const char* url, void* user_data) {
    WebViewImpl* impl = static_cast<WebViewImpl*>(user_data);
    if (impl) {
        impl->current_url = String::utf8(url);
        if (impl->editor) {
            impl->editor->on_url_changed(impl->current_url);
        }
    }
}

void WebViewImpl::on_history_changed(int can_go_back, int can_go_forward, void* user_data) {
    WebViewImpl* impl = static_cast<WebViewImpl*>(user_data);
    if (impl && impl->editor) {
        impl->editor->on_page_loaded(impl->current_url);
    }
}

#endif // WEBVIEW_WINDOWS
