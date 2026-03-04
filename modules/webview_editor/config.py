def can_build(env, platform):
    # Supported platforms
    if platform in ["windows", "macos", "linuxbsd"]:
        return True
    
    return False


def configure(env):
    # WebView editor is only available in editor builds
    if not env.editor_build:
        # Disable the module for non-editor builds
        env["module_webview_editor_enabled"] = False
    pass


def get_doc_classes():
    return [
        "WebViewEditor",
        "WebViewEditorPlugin",
    ]


def get_doc_path():
    return "doc_classes"
