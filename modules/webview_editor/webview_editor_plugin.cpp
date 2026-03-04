/**************************************************************************/
/*  webview_editor_plugin.cpp                                             */
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

#include "webview_editor_plugin.h"

#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"

WebViewEditorPlugin::WebViewEditorPlugin() {
	webview_editor = nullptr;
}

WebViewEditorPlugin::~WebViewEditorPlugin() {
	if (webview_editor) {
		webview_editor->queue_free();
	}
}

void WebViewEditorPlugin::make_visible(bool p_visible) {
	if (!webview_editor) {
		webview_editor = memnew(WebViewEditor);
		webview_editor->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		webview_editor->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		EditorNode::get_singleton()->get_editor_main_screen()->get_control()->add_child(webview_editor);
	}
	
	if (webview_editor) {
		if (p_visible) {
			webview_editor->show();
			webview_editor->set_webview_visible(true);
		} else {
			webview_editor->hide();
			webview_editor->set_webview_visible(false);
		}
	}
}

const Ref<Texture2D> WebViewEditorPlugin::get_plugin_icon() const {
	// Return null - Godot will use the plugin name to find an icon
	return Ref<Texture2D>();
}
