/**************************************************************************/
/*  webview2_wrapper.cpp                                                  */
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

#include "webview2_wrapper.h"

#include <windows.h>
#include <stdio.h>
#include <string>
#include <mutex>
#include <atomic>

// WebView2 headers
#include "WebView2.h"

// Link WebView2Loader library
#pragma comment(lib, "E:\\Byteworld\\godot-github\\modules\\webview_editor\\thirdparty\\webview2\\build\\native\\x64\\WebView2LoaderStatic.lib")

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
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
    printf("[WebView2] %s\n", buffer);
#endif
}

// ============================================================================
// String Conversion Utilities
// ============================================================================

static std::string WideToUtf8(const wchar_t* wide_str) {
    if (!wide_str) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide_str, -1, nullptr, 0, nullptr, nullptr);
    std::string result(size_needed - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide_str, -1, &result[0], size_needed, nullptr, nullptr);
    return result;
}

static std::wstring Utf8ToWide(const char* utf8_str) {
    if (!utf8_str) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, nullptr, 0);
    std::wstring result(size_needed - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, &result[0], size_needed);
    return result;
}

// ============================================================================
// WebView2 State Structure
// ============================================================================

struct WebView2State {
    // Window handles
    HWND parent_hwnd = nullptr;      // Godot editor window handle
    HWND container_hwnd = nullptr;   // Container window for WebView2
    
    // WebView2 COM objects
    ICoreWebView2Environment* webview_env = nullptr;
    ICoreWebView2Controller* webview_controller = nullptr;
    ICoreWebView2* webview = nullptr;
    ICoreWebView2Settings* webview_settings = nullptr;
    
    // State flags
    bool initialized = false;
    bool visible = true;
    bool can_go_back = false;
    bool can_go_forward = false;
    bool mouse_in_window = false;    // Track if mouse is over the webview
    
    // Geometry
    RECT bounds = {0, 0, 800, 600};
    
    // Navigation
    std::string current_url;
    std::string pending_url;
    
    // Thread safety
    std::mutex mutex;
    
    // Callbacks
    void* user_data = nullptr;
    WebView2NavigationStartingCallback navigation_starting_cb = nullptr;
    WebView2NavigationCompletedCallback navigation_completed_cb = nullptr;
    WebView2SourceChangedCallback source_changed_cb = nullptr;
    WebView2HistoryChangedCallback history_changed_cb = nullptr;
    
    // Event registration tokens
    EventRegistrationToken navigation_starting_token = {};
    EventRegistrationToken navigation_completed_token = {};
    EventRegistrationToken source_changed_token = {};
    EventRegistrationToken history_changed_token = {};
};

// ============================================================================
// COM Event Handler Base Template
// ============================================================================

template<typename T, typename Interface>
class EventHandlerBase : public Interface {
public:
    explicit EventHandlerBase(WebView2State* state) : state_(state), ref_count_(1) {}
    
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
        if (riid == IID_IUnknown || riid == __uuidof(Interface)) {
            *ppvObject = static_cast<Interface*>(this);
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }
    
    ULONG STDMETHODCALLTYPE AddRef() override { return ++ref_count_; }
    
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = --ref_count_;
        if (count == 0) delete this;
        return count;
    }

protected:
    WebView2State* state_;
    std::atomic<ULONG> ref_count_;
};

// ============================================================================
// COM Event Handlers
// ============================================================================

class NavigationStartingEventHandler : public EventHandlerBase<NavigationStartingEventHandler, ICoreWebView2NavigationStartingEventHandler> {
public:
    using EventHandlerBase::EventHandlerBase;
    
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) override {
        if (state_->navigation_starting_cb) {
            LPWSTR uri = nullptr;
            args->get_Uri(&uri);
            if (uri) {
                std::string url = WideToUtf8(uri);
                CoTaskMemFree(uri);
                state_->navigation_starting_cb(url.c_str(), state_->user_data);
            }
        }
        return S_OK;
    }
};

class NavigationCompletedEventHandler : public EventHandlerBase<NavigationCompletedEventHandler, ICoreWebView2NavigationCompletedEventHandler> {
public:
    using EventHandlerBase::EventHandlerBase;
    
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) override {
        if (state_->navigation_completed_cb) {
            BOOL success;
            args->get_IsSuccess(&success);
            
            LPWSTR uri = nullptr;
            sender->get_Source(&uri);
            std::string url = uri ? WideToUtf8(uri) : "";
            if (uri) CoTaskMemFree(uri);
            
            state_->navigation_completed_cb(url.c_str(), success ? 1 : 0, state_->user_data);
        }
        return S_OK;
    }
};

class SourceChangedEventHandler : public EventHandlerBase<SourceChangedEventHandler, ICoreWebView2SourceChangedEventHandler> {
public:
    using EventHandlerBase::EventHandlerBase;
    
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2SourceChangedEventArgs* args) override {
        LPWSTR uri = nullptr;
        sender->get_Source(&uri);
        if (uri) {
            state_->current_url = WideToUtf8(uri);
            CoTaskMemFree(uri);
            
            if (state_->source_changed_cb) {
                state_->source_changed_cb(state_->current_url.c_str(), state_->user_data);
            }
        }
        return S_OK;
    }
};

class HistoryChangedEventHandler : public EventHandlerBase<HistoryChangedEventHandler, ICoreWebView2HistoryChangedEventHandler> {
public:
    using EventHandlerBase::EventHandlerBase;
    
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, IUnknown* args) override {
        BOOL can_go_back, can_go_forward;
        sender->get_CanGoBack(&can_go_back);
        sender->get_CanGoForward(&can_go_forward);
        
        state_->can_go_back = can_go_back;
        state_->can_go_forward = can_go_forward;
        
        if (state_->history_changed_cb) {
            state_->history_changed_cb(can_go_back ? 1 : 0, can_go_forward ? 1 : 0, state_->user_data);
        }
        return S_OK;
    }
};

// ============================================================================
// WebView2 Creation Handlers
// ============================================================================

// Forward declaration needed for EnvironmentCompletedHandler
class ControllerCompletedHandler;

class EnvironmentCompletedHandler : public EventHandlerBase<EnvironmentCompletedHandler, ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler> {
public:
    using EventHandlerBase::EventHandlerBase;
    
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Environment* env) override;
};

class ControllerCompletedHandler : public EventHandlerBase<ControllerCompletedHandler, ICoreWebView2CreateCoreWebView2ControllerCompletedHandler> {
public:
    using EventHandlerBase::EventHandlerBase;
    
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Controller* controller) override {
        if (FAILED(result) || !controller) {
            DebugLog("Failed to create WebView2 controller");
            return result;
        }
        
        std::lock_guard<std::mutex> lock(state_->mutex);
        
        state_->webview_controller = controller;
        controller->AddRef();
        controller->get_CoreWebView2(&state_->webview);
        
        if (state_->webview) {
            // Configure settings
            state_->webview->get_Settings(&state_->webview_settings);
            if (state_->webview_settings) {
                state_->webview_settings->put_IsScriptEnabled(TRUE);
                state_->webview_settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                state_->webview_settings->put_IsWebMessageEnabled(TRUE);
                state_->webview_settings->put_AreDevToolsEnabled(TRUE);
            }
            
            // Set initial bounds
            RECT bounds;
            GetClientRect(state_->container_hwnd, &bounds);
            controller->put_Bounds(bounds);
            controller->put_IsVisible(TRUE);
            
            // Register event handlers
            auto* nav_start = new NavigationStartingEventHandler(state_);
            auto* nav_complete = new NavigationCompletedEventHandler(state_);
            auto* source_changed = new SourceChangedEventHandler(state_);
            auto* history_changed = new HistoryChangedEventHandler(state_);
            
            state_->webview->add_NavigationStarting(nav_start, &state_->navigation_starting_token);
            state_->webview->add_NavigationCompleted(nav_complete, &state_->navigation_completed_token);
            state_->webview->add_SourceChanged(source_changed, &state_->source_changed_token);
            state_->webview->add_HistoryChanged(history_changed, &state_->history_changed_token);
            
            state_->initialized = true;
            
            // Navigate to pending URL if any
            if (!state_->pending_url.empty()) {
                std::wstring url = Utf8ToWide(state_->pending_url.c_str());
                state_->webview->Navigate(url.c_str());
                state_->pending_url.clear();
            }
        }
        
        return S_OK;
    }
};

// EnvironmentCompletedHandler implementation (must be after ControllerCompletedHandler)
HRESULT STDMETHODCALLTYPE EnvironmentCompletedHandler::Invoke(HRESULT result, ICoreWebView2Environment* env) {
    if (FAILED(result) || !env) {
        DebugLog("Failed to create WebView2 environment");
        return result;
    }
    
    state_->webview_env = env;
    env->AddRef();
    
    // Create controller
    auto* controller_handler = new ControllerCompletedHandler(state_);
    HRESULT hr = env->CreateCoreWebView2Controller(state_->container_hwnd, controller_handler);
    
    if (FAILED(hr)) {
        DebugLog("CreateCoreWebView2Controller failed: HRESULT=0x%08X", hr);
    }
    
    return S_OK;
}

// ============================================================================
// Window Procedure
// ============================================================================

static LRESULT CALLBACK ContainerWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    auto* state = reinterpret_cast<WebView2State*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    
    switch (msg) {
        case WM_SIZE:
            // Update WebView2 bounds when container is resized
            if (state && state->webview_controller) {
                RECT bounds;
                GetClientRect(hwnd, &bounds);
                state->webview_controller->put_Bounds(bounds);
            }
            return 0;
            
        case WM_DESTROY:
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

// ============================================================================
// Window Class Registration
// ============================================================================

static bool RegisterContainerClass() {
    static bool registered = false;
    if (registered) return true;
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = ContainerWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"WebView2Container";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    if (!RegisterClassExW(&wc)) {
        DebugLog("Failed to register window class");
        return false;
    }
    registered = true;
    return true;
}

// ============================================================================
// Public API Implementation
// ============================================================================

WebView2Handle webview2_create(void* parent_hwnd, void* user_data) {
    DebugLog("webview2_create called with parent_hwnd=%p", parent_hwnd);
    
    if (!parent_hwnd) {
        DebugLog("Error: parent_hwnd is null");
        return nullptr;
    }
    
    auto* state = new WebView2State();
    state->parent_hwnd = static_cast<HWND>(parent_hwnd);
    state->user_data = user_data;
    
    if (!RegisterContainerClass()) {
        delete state;
        return nullptr;
    }
    
    // Create container window with WS_EX_NOACTIVATE to prevent automatic focus
    state->container_hwnd = CreateWindowExW(
        WS_EX_NOACTIVATE,
        L"WebView2Container",
        L"WebView2 Container",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0, 0, 800, 600,
        state->parent_hwnd,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (!state->container_hwnd) {
        DebugLog("Failed to create container window, error=%lu", GetLastError());
        delete state;
        return nullptr;
    }
    
    SetWindowLongPtr(state->container_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    
    // Create WebView2 environment (async)
    auto* env_handler = new EnvironmentCompletedHandler(state);
    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr, env_handler);
    
    if (FAILED(hr)) {
        DebugLog("CreateCoreWebView2EnvironmentWithOptions failed: HRESULT=0x%08X", hr);
        DestroyWindow(state->container_hwnd);
        delete state;
        return nullptr;
    }
    
    return state;
}

void webview2_destroy(WebView2Handle handle) {
    if (!handle) return;
    
    auto* state = static_cast<WebView2State*>(handle);
    
    // Unregister event handlers
    if (state->webview) {
        state->webview->remove_NavigationStarting(state->navigation_starting_token);
        state->webview->remove_NavigationCompleted(state->navigation_completed_token);
        state->webview->remove_SourceChanged(state->source_changed_token);
        state->webview->remove_HistoryChanged(state->history_changed_token);
    }
    
    // Release COM objects
    if (state->webview_settings) state->webview_settings->Release();
    if (state->webview) state->webview->Release();
    if (state->webview_controller) {
        state->webview_controller->Close();
        state->webview_controller->Release();
    }
    if (state->webview_env) state->webview_env->Release();
    
    // Destroy window
    if (state->container_hwnd) {
        DestroyWindow(state->container_hwnd);
    }
    
    delete state;
}

void webview2_navigate(WebView2Handle handle, const char* url) {
    if (!handle || !url) return;
    
    auto* state = static_cast<WebView2State*>(handle);
    std::lock_guard<std::mutex> lock(state->mutex);
    
    if (state->webview && state->initialized) {
        std::wstring wide_url = Utf8ToWide(url);
        state->webview->Navigate(wide_url.c_str());
    } else {
        state->pending_url = url;
    }
}

void webview2_go_back(WebView2Handle handle) {
    if (!handle) return;
    auto* state = static_cast<WebView2State*>(handle);
    std::lock_guard<std::mutex> lock(state->mutex);
    if (state->webview) state->webview->GoBack();
}

void webview2_go_forward(WebView2Handle handle) {
    if (!handle) return;
    auto* state = static_cast<WebView2State*>(handle);
    std::lock_guard<std::mutex> lock(state->mutex);
    if (state->webview) state->webview->GoForward();
}

void webview2_reload(WebView2Handle handle) {
    if (!handle) return;
    auto* state = static_cast<WebView2State*>(handle);
    std::lock_guard<std::mutex> lock(state->mutex);
    if (state->webview) state->webview->Reload();
}

void webview2_stop(WebView2Handle handle) {
    if (!handle) return;
    auto* state = static_cast<WebView2State*>(handle);
    std::lock_guard<std::mutex> lock(state->mutex);
    if (state->webview) state->webview->Stop();
}

int webview2_can_go_back(WebView2Handle handle) {
    if (!handle) return 0;
    auto* state = static_cast<WebView2State*>(handle);
    std::lock_guard<std::mutex> lock(state->mutex);
    return state->can_go_back ? 1 : 0;
}

int webview2_can_go_forward(WebView2Handle handle) {
    if (!handle) return 0;
    auto* state = static_cast<WebView2State*>(handle);
    std::lock_guard<std::mutex> lock(state->mutex);
    return state->can_go_forward ? 1 : 0;
}

void webview2_get_url(WebView2Handle handle, char* buffer, int buffer_size) {
    if (!handle || !buffer || buffer_size <= 0) return;
    
    auto* state = static_cast<WebView2State*>(handle);
    std::lock_guard<std::mutex> lock(state->mutex);
    
    std::string url;
    if (state->webview) {
        LPWSTR uri = nullptr;
        state->webview->get_Source(&uri);
        if (uri) {
            url = WideToUtf8(uri);
            CoTaskMemFree(uri);
        }
    } else {
        url = state->current_url;
    }
    
    strncpy_s(buffer, buffer_size, url.c_str(), _TRUNCATE);
}

void webview2_set_visible(WebView2Handle handle, int visible) {
    if (!handle) return;
    
    auto* state = static_cast<WebView2State*>(handle);
    std::lock_guard<std::mutex> lock(state->mutex);
    
    state->visible = visible != 0;
    
    if (state->container_hwnd) {
        ShowWindow(state->container_hwnd, state->visible ? SW_SHOW : SW_HIDE);
    }
    if (state->webview_controller) {
        state->webview_controller->put_IsVisible(state->visible ? TRUE : FALSE);
    }
}

void webview2_set_bounds(WebView2Handle handle, int x, int y, int width, int height) {
    if (!handle) return;
    
    auto* state = static_cast<WebView2State*>(handle);
    
    // Convert screen coordinates to parent window client coordinates
    POINT pt = {x, y};
    if (state->parent_hwnd) {
        ScreenToClient(state->parent_hwnd, &pt);
    }
    
    state->bounds = {pt.x, pt.y, pt.x + width, pt.y + height};
    
    if (state->container_hwnd) {
        SetWindowPos(state->container_hwnd, nullptr, pt.x, pt.y, width, height, SWP_NOZORDER);
    }
    
    if (state->webview_controller) {
        RECT controller_bounds = {0, 0, width, height};
        state->webview_controller->put_Bounds(controller_bounds);
    }
}

void webview2_set_navigation_starting_callback(WebView2Handle handle, WebView2NavigationStartingCallback callback) {
    if (!handle) return;
    static_cast<WebView2State*>(handle)->navigation_starting_cb = callback;
}

void webview2_set_navigation_completed_callback(WebView2Handle handle, WebView2NavigationCompletedCallback callback) {
    if (!handle) return;
    static_cast<WebView2State*>(handle)->navigation_completed_cb = callback;
}

void webview2_set_source_changed_callback(WebView2Handle handle, WebView2SourceChangedCallback callback) {
    if (!handle) return;
    static_cast<WebView2State*>(handle)->source_changed_cb = callback;
}

void webview2_set_history_changed_callback(WebView2Handle handle, WebView2HistoryChangedCallback callback) {
    if (!handle) return;
    static_cast<WebView2State*>(handle)->history_changed_cb = callback;
}

void webview2_execute_javascript(WebView2Handle handle, const char* script) {
    if (!handle || !script) return;
    
    auto* state = static_cast<WebView2State*>(handle);
    std::lock_guard<std::mutex> lock(state->mutex);
    
    if (state->webview) {
        std::wstring wide_script = Utf8ToWide(script);
        state->webview->ExecuteScript(wide_script.c_str(), nullptr);
    }
}

void webview2_process_messages(WebView2Handle handle) {
    // Process Windows messages
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Poll mouse position to detect when mouse leaves the webview
    if (!handle) return;
    
    auto* state = static_cast<WebView2State*>(handle);
    if (!state->container_hwnd || !state->parent_hwnd) return;
    
    POINT pt;
    if (!GetCursorPos(&pt)) return;
    
    RECT rect;
    if (!GetWindowRect(state->container_hwnd, &rect)) return;
    
    bool in_window = (pt.x >= rect.left && pt.x <= rect.right && 
                     pt.y >= rect.top && pt.y <= rect.bottom);
    
    if (state->mouse_in_window && !in_window) {
        // Mouse left the window - release focus to parent
        state->mouse_in_window = false;
        SetFocus(state->parent_hwnd);
    } else if (!state->mouse_in_window && in_window) {
        // Mouse entered the window
        state->mouse_in_window = true;
    }
}

void webview2_release_focus(WebView2Handle handle) {
    if (!handle) return;
    
    auto* state = static_cast<WebView2State*>(handle);
    std::lock_guard<std::mutex> lock(state->mutex);
    
    if (state->parent_hwnd) {
        SetFocus(state->parent_hwnd);
    }
}
