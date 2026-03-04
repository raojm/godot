/**************************************************************************/
/*  webview_editor.h                                                      */
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

#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/control.h"

// Forward declaration for platform-specific implementation
class WebViewImpl;

/**
 * WebViewEditor - A web browser panel for Godot Editor
 * 
 * This class provides a web browsing interface within the Godot Editor.
 * It uses the native WebView2 component on Windows to render web content.
 * 
 * Key features:
 * - Navigation controls (back, forward, refresh, home)
 * - URL address bar with input
 * - Status display
 * - Focus management to prevent webview from capturing all input
 */
class WebViewEditor : public PanelContainer {
    GDCLASS(WebViewEditor, PanelContainer)

public:
    // Navigation methods
    void navigate(const String &p_url);
    void _navigate_back();
    void _navigate_forward();
    void _refresh_page();
    void _navigate_home();
    void _navigate_to_url(const String &p_url);
    
    // Event handlers
    void _on_url_submitted(const String &p_url);
    void _on_address_bar_focus_entered();
    void _on_address_bar_focus_exited();

    // Callbacks from WebView implementation
    void on_page_loaded(const String &p_url);
    void on_page_load_failed(const String &p_url, const String &p_error);
    void on_title_changed(const String &p_title);
    void on_url_changed(const String &p_url);
    void on_loading_state_changed(bool p_loading);

    // Getters / Setters
    String get_current_url() const;
    bool can_go_back() const;
    bool can_go_forward() const;
    void set_webview_visible(bool p_visible);
    bool is_webview_visible() const;

    // Godot overrides
    virtual void _notification(int p_what);
    virtual void gui_input(const Ref<InputEvent> &p_gui_input) override;

    WebViewEditor();
    ~WebViewEditor();

protected:
    static void _bind_methods() {}

private:
    // UI Components
    VBoxContainer *main_vbox = nullptr;
    HBoxContainer *toolbar_hb = nullptr;
    Button *back_button = nullptr;
    Button *forward_button = nullptr;
    Button *refresh_button = nullptr;
    Button *home_button = nullptr;
    LineEdit *address_bar = nullptr;
    Label *status_label = nullptr;
    Control *webview_container = nullptr;
    
    // Platform-specific WebView implementation
    WebViewImpl *webview_impl = nullptr;
    
    // State
    String current_url;
    bool webview_visible = false;
    bool is_address_bar_focused = false;
    bool webview_initialized = false;
    
    // UI Creation
    void _create_toolbar();
    
    // WebView lifecycle
    void _initialize_webview();
    void _delayed_initialize_webview();
    void _destroy_webview();
    
    // Updates
    void _update_webview_bounds();
    void _update_navigation_buttons();
    void _update_status(const String &p_status);
    void _handle_visibility_changed();
};
