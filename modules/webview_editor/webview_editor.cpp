/**************************************************************************/
/*  webview_editor.cpp                                                    */
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

#include "webview_editor.h"

#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/control.h"
#include "scene/gui/reference_rect.h"
#include "scene/main/window.h"
#include "servers/display/display_server.h"

#include <stdio.h>

#if defined(WEBVIEW_WINDOWS)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// ============================================================================
// Debug Utilities
// ============================================================================

// Set to 1 to enable debug output, 0 to disable
#define ENABLE_DEBUG_LOG 0

static void DebugLog(const char* format, ...) {
#if ENABLE_DEBUG_LOG
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    printf("[WebViewEditor] %s\n", buffer);
#endif
}

// Include platform-specific implementation
#if defined(WEBVIEW_WINDOWS)
#include "webview_impl_windows.h"
#elif defined(WEBVIEW_MACOS)
#include "webview_impl_macos.h"
#elif defined(WEBVIEW_LINUX)
#include "webview_impl_linux.h"
#endif

// ============================================================================
// Constructor / Destructor
// ============================================================================

WebViewEditor::WebViewEditor() {
    DebugLog("Constructor");
    
    // Create main vertical layout
    main_vbox = memnew(VBoxContainer);
    main_vbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
    add_child(main_vbox);

    // Create toolbar with navigation controls
    _create_toolbar();

    // Create webview container (visual placeholder for native webview)
    webview_container = memnew(ReferenceRect);
    webview_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    webview_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    static_cast<ReferenceRect*>(webview_container)->set_border_color(Color(1, 0, 0, 1));
    main_vbox->add_child(webview_container);

    // Create platform-specific WebView implementation
#if defined(WEBVIEW_WINDOWS)
    webview_impl = memnew(WebViewImpl(this));
#elif defined(WEBVIEW_MACOS)
    webview_impl = memnew(WebViewImpl(this));
#elif defined(WEBVIEW_LINUX)
    webview_impl = memnew(WebViewImpl(this));
#endif

    // Set default URL
    current_url = "about:blank";
    address_bar->set_text(current_url);
    
    DebugLog("Constructor done");
}

WebViewEditor::~WebViewEditor() {
    _destroy_webview();
}

// ============================================================================
// UI Creation
// ============================================================================

void WebViewEditor::_create_toolbar() {
    toolbar_hb = memnew(HBoxContainer);
    toolbar_hb->set_custom_minimum_size(Size2(0, 36));
    main_vbox->add_child(toolbar_hb);

    // Navigation buttons
    back_button = memnew(Button);
    back_button->set_text("Back");
    back_button->set_disabled(true);
    back_button->connect("pressed", callable_mp(this, &WebViewEditor::_navigate_back));
    toolbar_hb->add_child(back_button);

    forward_button = memnew(Button);
    forward_button->set_text("Forward");
    forward_button->set_disabled(true);
    forward_button->connect("pressed", callable_mp(this, &WebViewEditor::_navigate_forward));
    toolbar_hb->add_child(forward_button);

    refresh_button = memnew(Button);
    refresh_button->set_text("Refresh");
    refresh_button->connect("pressed", callable_mp(this, &WebViewEditor::_refresh_page));
    toolbar_hb->add_child(refresh_button);

    home_button = memnew(Button);
    home_button->set_text("Home");
    home_button->connect("pressed", callable_mp(this, &WebViewEditor::_navigate_home));
    toolbar_hb->add_child(home_button);

    // Address bar
    address_bar = memnew(LineEdit);
    address_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    address_bar->set_placeholder("Enter URL...");
    address_bar->connect("text_submitted", callable_mp(this, &WebViewEditor::_on_url_submitted));
    address_bar->connect("focus_entered", callable_mp(this, &WebViewEditor::_on_address_bar_focus_entered));
    address_bar->connect("focus_exited", callable_mp(this, &WebViewEditor::_on_address_bar_focus_exited));
    toolbar_hb->add_child(address_bar);

    // Status label
    status_label = memnew(Label);
    status_label->set_custom_minimum_size(Size2(100, 0));
    status_label->set_text("Ready");
    toolbar_hb->add_child(status_label);
}

// ============================================================================
// Godot Notifications
// ============================================================================

void WebViewEditor::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_ENTER_TREE:
            _initialize_webview();
            set_process(true);
            break;
            
        case NOTIFICATION_VISIBILITY_CHANGED:
            _handle_visibility_changed();
            break;
            
        case NOTIFICATION_RESIZED:
        case NOTIFICATION_DRAW:
            _update_webview_bounds();
            break;
            
        case NOTIFICATION_EXIT_TREE:
            _destroy_webview();
            break;
            
        case NOTIFICATION_PROCESS:
            _update_webview_bounds();
            if (webview_impl) {
                webview_impl->process_messages();
            }
            break;
    }
}

void WebViewEditor::_handle_visibility_changed() {
    if (is_visible()) {
        _update_navigation_buttons();
    } else {
        // Release focus when hidden
        if (webview_impl) {
            webview_impl->release_focus();
        }
    }
    
    if (webview_impl) {
        webview_impl->set_visible(is_visible());
    }
}

// ============================================================================
// WebView Initialization
// ============================================================================

void WebViewEditor::_initialize_webview() {
    if (webview_initialized || !webview_impl) {
        return;
    }

#if defined(WEBVIEW_WINDOWS)
    // Delay initialization to ensure window is ready
    callable_mp(this, &WebViewEditor::_delayed_initialize_webview).call_deferred();
#endif
}

void WebViewEditor::_delayed_initialize_webview() {
    if (webview_initialized || !webview_impl || !webview_container) {
        return;
    }

#if defined(WEBVIEW_WINDOWS)
    // Get the native window handle from DisplayServer
    void* native_handle = (void*)DisplayServer::get_singleton()->window_get_native_handle(
        DisplayServer::WINDOW_HANDLE, 
        DisplayServer::MAIN_WINDOW_ID
    );
    
    if (native_handle && webview_impl->initialize(native_handle)) {
        webview_initialized = true;
        callable_mp(this, &WebViewEditor::_update_webview_bounds).call_deferred();
        navigate("https://www.baidu.com");
    }
#endif
}

void WebViewEditor::_destroy_webview() {
    if (webview_impl) {
        webview_impl->destroy();
        memdelete(webview_impl);
        webview_impl = nullptr;
    }
    webview_initialized = false;
}

// ============================================================================
// Bounds Update
// ============================================================================

void WebViewEditor::_update_webview_bounds() {
    if (!webview_impl || !webview_container) {
        return;
    }

    Vector2 global_pos = webview_container->get_global_position();
    Vector2 size = webview_container->get_size();
    
    if (size.x <= 0 || size.y <= 0) {
        return;
    }
    
    Window* root_window = get_tree()->get_root();
    if (!root_window) return;
    
#if defined(WEBVIEW_WINDOWS)
    HWND hwnd = (HWND)DisplayServer::get_singleton()->window_get_native_handle(
        DisplayServer::WINDOW_HANDLE, 
        DisplayServer::MAIN_WINDOW_ID
    );
    
    if (hwnd) {
        // Get client rect and convert to screen coordinates
        RECT client_rect;
        GetClientRect(hwnd, &client_rect);
        
        POINT client_top_left = {client_rect.left, client_rect.top};
        ClientToScreen(hwnd, &client_top_left);
        
        int screen_x = client_top_left.x + (int)global_pos.x;
        int screen_y = client_top_left.y + (int)global_pos.y;
        
        webview_impl->set_bounds(screen_x, screen_y, (int)size.x, (int)size.y);
    }
#else
    Vector2 window_pos = root_window->get_position();
    int screen_x = (int)(window_pos.x + global_pos.x);
    int screen_y = (int)(window_pos.y + global_pos.y);
    webview_impl->set_bounds(screen_x, screen_y, (int)size.x, (int)size.y);
#endif
}

// ============================================================================
// Navigation
// ============================================================================

void WebViewEditor::navigate(const String& p_url) {
    _navigate_to_url(p_url);
}

void WebViewEditor::_navigate_to_url(const String& p_url) {
    String url = p_url.strip_edges();
    
    // Add https:// if no protocol specified
    if (!url.contains("://") && !url.begins_with("about:")) {
        url = "https://" + url;
    }
    
    current_url = url;
    
    if (address_bar) {
        address_bar->set_text(current_url);
    }
    
    _update_status("Loading...");
    
    if (webview_impl) {
        webview_impl->navigate(current_url);
    }
    
    _update_navigation_buttons();
}

void WebViewEditor::_navigate_back() {
    if (webview_impl) {
        webview_impl->go_back();
    }
}

void WebViewEditor::_navigate_forward() {
    if (webview_impl) {
        webview_impl->go_forward();
    }
}

void WebViewEditor::_refresh_page() {
    if (webview_impl) {
        webview_impl->reload();
    }
}

void WebViewEditor::_navigate_home() {
    _navigate_to_url("https://www.baidu.com");
}

// ============================================================================
// Event Handlers
// ============================================================================

void WebViewEditor::_on_url_submitted(const String& p_url) {
    _navigate_to_url(p_url);
}

void WebViewEditor::_on_address_bar_focus_entered() {
    is_address_bar_focused = true;
    if (address_bar) {
        address_bar->select_all();
    }
    // Release webview focus when address bar gets focus
    if (webview_impl) {
        webview_impl->release_focus();
    }
}

void WebViewEditor::_on_address_bar_focus_exited() {
    is_address_bar_focused = false;
    if (address_bar) {
        address_bar->deselect();
    }
}

// ============================================================================
// Input Handling
// ============================================================================

void WebViewEditor::gui_input(const Ref<InputEvent> &p_gui_input) {
    // Handle GUI input events if needed
    // Currently empty as focus management is handled through other mechanisms
}

// ============================================================================
// Callbacks from WebView
// ============================================================================

void WebViewEditor::on_page_loaded(const String& p_url) {
    if (status_label) {
        status_label->set_text("Done");
    }
    _update_navigation_buttons();
}

void WebViewEditor::on_page_load_failed(const String& p_url, const String& p_error) {
    if (status_label) {
        status_label->set_text("Error");
    }
}

void WebViewEditor::on_title_changed(const String& p_title) {
    // Could update window title or tab label here
}

void WebViewEditor::on_url_changed(const String& p_url) {
    current_url = p_url;
    if (address_bar && !is_address_bar_focused) {
        address_bar->set_text(p_url);
    }
}

void WebViewEditor::on_loading_state_changed(bool p_loading) {
    if (status_label) {
        status_label->set_text(p_loading ? "Loading..." : "Done");
    }
}

// ============================================================================
// Getters / Setters
// ============================================================================

String WebViewEditor::get_current_url() const {
    return current_url;
}

bool WebViewEditor::can_go_back() const {
    return webview_impl ? webview_impl->can_go_back() : false;
}

bool WebViewEditor::can_go_forward() const {
    return webview_impl ? webview_impl->can_go_forward() : false;
}

void WebViewEditor::set_webview_visible(bool p_visible) {
    webview_visible = p_visible;
    if (webview_impl) {
        webview_impl->set_visible(p_visible);
        if (!p_visible) {
            webview_impl->release_focus();
        }
    }
}

bool WebViewEditor::is_webview_visible() const {
    return webview_visible;
}

// ============================================================================
// UI Updates
// ============================================================================

void WebViewEditor::_update_navigation_buttons() {
    if (back_button) {
        back_button->set_disabled(!can_go_back());
    }
    if (forward_button) {
        forward_button->set_disabled(!can_go_forward());
    }
}

void WebViewEditor::_update_status(const String& p_status) {
    if (status_label) {
        status_label->set_text(p_status);
    }
}
