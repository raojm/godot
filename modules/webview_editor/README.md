# Godot Engine WebView Editor Module

This module adds a fully functional WebView tab to the Godot Engine editor, allowing you to browse web content directly within the editor interface.

## Features

- **Full WebView Integration**: Browse websites directly in the Godot editor
- **Navigation Controls**: Back, Forward, Refresh, and Home buttons
- **Address Bar**: URL input with auto-complete and submission
- **Tab Integration**: Appears as a main tab in the editor (next to AssetLib)
- **Cross-Platform**: Supports Windows, macOS, and Linux
- **Input Conflict Resolution**: Uses sub-window approach to avoid input conflicts with Godot editor

## Architecture

### Input Conflict Solution

The module uses a **Godot Sub-Window** approach to handle input conflicts:

1. **Sub-Window Strategy**: The WebView is hosted in a separate Godot Window that is positioned over the main editor area. This isolates WebView input events from Godot's input system.

2. **Alternative Solutions** (if sub-window is not viable):
   - **Offscreen Rendering**: Render WebView to texture and display in Godot viewport (requires more complex implementation)
   - **Input Forwarding**: Capture all input in Godot and forward to WebView via platform-specific APIs

### Platform Implementations

- **Windows**: Uses Microsoft Edge WebView2
- **macOS**: Uses WKWebView
- **Linux**: Uses WebKitGTK

## Installation

### Prerequisites

#### Windows
- Windows 10/11
- Microsoft Edge WebView2 Runtime (usually pre-installed)
- Visual Studio 2019 or later with C++ workload

#### macOS
- macOS 10.14 or later
- Xcode Command Line Tools

#### Linux
- GTK3 development libraries
- WebKit2GTK development libraries

For Ubuntu/Debian:
```bash
sudo apt-get install libgtk-3-dev libwebkit2gtk-4.0-dev
```

For Fedora:
```bash
sudo dnf install gtk3-devel webkit2gtk3-devel
```

### Building Godot with WebView Module

1. Clone or download the Godot Engine source code

2. Copy this module to the `modules` directory:
   ```bash
   cp -r webview_editor /path/to/godot/source/modules/
   ```

3. Build Godot with the module enabled:

   **Windows:**
   ```bash
   scons platform=windows target=editor arch=x86_64
   ```

   **macOS:**
   ```bash
   scons platform=macos target=editor arch=arm64
   ```

   **Linux:**
   ```bash
   scons platform=linuxbsd target=editor arch=x86_64
   ```

4. Run the compiled Godot editor:
   ```bash
   ./bin/godot.windows.editor.x86_64.exe  # Windows
   ./bin/godot.macos.editor.arm64         # macOS
   ./bin/godot.linuxbsd.editor.x86_64     # Linux
   ```

## Usage

1. Open the Godot editor
2. Look for the "Web" tab in the main editor tabs (next to "AssetLib")
3. Click on the "Web" tab to open the WebView
4. Use the navigation controls:
   - **Back/Forward**: Navigate through browsing history
   - **Refresh**: Reload the current page
   - **Home**: Go to the default homepage (godotengine.org)
   - **Address Bar**: Type a URL and press Enter to navigate

## Configuration

The module can be configured by modifying the source code:

- **Default URL**: Change `DEFAULT_URL` in `webview_editor.h`
- **Home URL**: Change `HOME_URL` in `webview_editor.h`
- **Sub-window mode**: Toggle `use_subwindow` in `webview_editor.h`

## Troubleshooting

### WebView doesn't appear
- Check that the module was compiled correctly
- Verify platform-specific dependencies are installed
- Check the Godot console for error messages

### Input not working in WebView
- Ensure the WebView tab is active
- Try clicking inside the WebView area to focus it
- Check if the sub-window is properly positioned

### Build errors
- **Windows**: Ensure WebView2 SDK is installed
- **macOS**: Ensure Xcode Command Line Tools are installed
- **Linux**: Ensure GTK3 and WebKit2GTK development packages are installed

## License

This module is part of Godot Engine and follows the same license (MIT).

## Contributing

Contributions are welcome! Please follow the Godot Engine contribution guidelines.
