/**************************************************************************/
/*  webview_impl_macos.mm                                                 */
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

#ifdef WEBVIEW_MACOS

#include "webview_impl_macos.h"
#include "webview_editor.h"

#import <WebKit/WebKit.h>
#import <Cocoa/Cocoa.h>

// Delegate class for handling WKWebView events
@interface WebViewDelegate : NSObject <WKNavigationDelegate>
@property (nonatomic, assign) WebViewImpl *impl;
@end

@implementation WebViewDelegate

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation {
	if (_impl) {
		String url = String::utf8([webView.URL.absoluteString UTF8String]);
		_impl->on_page_loaded(url);
	}
}

- (void)webView:(WKWebView *)webView didFailNavigation:(WKNavigation *)navigation withError:(NSError *)error {
	if (_impl) {
		String url = String::utf8([webView.URL.absoluteString UTF8String]);
		String error_str = String::utf8([error.localizedDescription UTF8String]);
		_impl->on_page_load_failed(url, error_str);
	}
}

- (void)webView:(WKWebView *)webView didFailProvisionalNavigation:(WKNavigation *)navigation withError:(NSError *)error {
	if (_impl) {
		String url = String::utf8([webView.URL.absoluteString UTF8String]);
		String error_str = String::utf8([error.localizedDescription UTF8String]);
		_impl->on_page_load_failed(url, error_str);
	}
}

- (void)webView:(WKWebView *)webView didStartProvisionalNavigation:(WKNavigation *)navigation {
	if (_impl) {
		_impl->on_loading_state_changed(true);
	}
}

- (void)webView:(WKWebView *)webView didCommitNavigation:(WKNavigation *)navigation {
	if (_impl) {
		String url = String::utf8([webView.URL.absoluteString UTF8String]);
		_impl->on_url_changed(url);
	}
}

@end

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

	parent_window = (NSWindow *)p_parent_handle;
	if (!parent_window) {
		ERR_PRINT("WebViewImpl: Invalid parent window");
		return false;
	}

	// Create container view
	container_view = [[NSView alloc] initWithFrame:NSMakeRect(bounds_x, bounds_y, bounds_width, bounds_height)];
	if (!container_view) {
		ERR_PRINT("WebViewImpl: Failed to create container view");
		return false;
	}

	// Create WKWebView configuration
	WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];
	[config.preferences setJavaScriptEnabled:YES];
	[config setAllowsAirPlayForMediaPlayback:YES];

	// Create WKWebView
	webview = [[WKWebView alloc] initWithFrame:((NSView *)container_view).bounds configuration:config];
	if (!webview) {
		ERR_PRINT("WebViewImpl: Failed to create WKWebView");
		return false;
	}

	// Set delegate
	WebViewDelegate *delegate = [[WebViewDelegate alloc] init];
	delegate.impl = this;
	[((WKWebView *)webview) setNavigationDelegate:delegate];

	// Add webview to container
	[((NSView *)container_view) addSubview:((WKWebView *)webview)];

	// Add container to parent window
	[parent_window.contentView addSubview:((NSView *)container_view)];

	// Set initial visibility
	[((NSView *)container_view) setHidden:!visible];

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
	if (container_view) {
		[((NSView *)container_view) removeFromSuperview];
	}

	// Release objects
	webview = nullptr;
	container_view = nullptr;
	parent_window = nullptr;

	initialized = false;
}

void WebViewImpl::navigate(const String &p_url) {
	MutexLock lock(mutex);

	if (!initialized || !webview) {
		pending_url = p_url;
		return;
	}

	NSString *url_str = [NSString stringWithUTF8String:p_url.utf8().get_data()];
	NSURL *url = [NSURL URLWithString:url_str];
	NSURLRequest *request = [NSURLRequest requestWithURL:url];
	[((WKWebView *)webview) loadRequest:request];
}

void WebViewImpl::go_back() {
	MutexLock lock(mutex);

	if (!initialized || !webview) {
		return;
	}

	if ([((WKWebView *)webview) canGoBack]) {
		[((WKWebView *)webview) goBack];
	}
}

void WebViewImpl::go_forward() {
	MutexLock lock(mutex);

	if (!initialized || !webview) {
		return;
	}

	if ([((WKWebView *)webview) canGoForward]) {
		[((WKWebView *)webview) goForward];
	}
}

void WebViewImpl::reload() {
	MutexLock lock(mutex);

	if (!initialized || !webview) {
		return;
	}

	[((WKWebView *)webview) reload];
}

void WebViewImpl::stop() {
	MutexLock lock(mutex);

	if (!initialized || !webview) {
		return;
	}

	[((WKWebView *)webview) stopLoading];
}

void WebViewImpl::set_visible(bool p_visible) {
	MutexLock lock(mutex);

	visible = p_visible;

	if (container_view) {
		[((NSView *)container_view) setHidden:!p_visible];
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
	if (container_view) {
		NSRect frame = NSMakeRect(bounds_x, bounds_y, bounds_width, bounds_height);
		[((NSView *)container_view) setFrame:frame];
	}

	if (webview) {
		[((WKWebView *)webview) setFrame:((NSView *)container_view).bounds];
	}
}

bool WebViewImpl::can_go_back() {
	MutexLock lock(mutex);

	if (!initialized || !webview) {
		return false;
	}

	return [((WKWebView *)webview) canGoBack];
}

bool WebViewImpl::can_go_forward() {
	MutexLock lock(mutex);

	if (!initialized || !webview) {
		return false;
	}

	return [((WKWebView *)webview) canGoForward];
}

String WebViewImpl::get_url() {
	MutexLock lock(mutex);

	if (!initialized || !webview) {
		return String();
	}

	NSString *url_str = ((WKWebView *)webview).URL.absoluteString;
	if (url_str) {
		return String::utf8([url_str UTF8String]);
	}

	return String();
}

void WebViewImpl::execute_javascript(const String &p_script) {
	MutexLock lock(mutex);

	if (!initialized || !webview) {
		return;
	}

	NSString *script = [NSString stringWithUTF8String:p_script.utf8().get_data()];
	[((WKWebView *)webview) evaluateJavaScript:script completionHandler:nil];
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

#endif // WEBVIEW_MACOS
