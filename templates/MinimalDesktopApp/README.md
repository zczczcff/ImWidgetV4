# Minimal Desktop App

This template is a tiny Windows desktop consumer for the packaged ImWidgetV4 SDK.

Configure it with the SDK package directory:

```powershell
cmake -S . -B build -DImWidgetV4_DIR=<ImWidgetV4-Release>/sdk/cmake
cmake --build build --config Debug
```

The application entry point is provided by `ImWidgetV4::app_host_win32_main`.
The project only implements `CreateApplicationHostDelegate()` and the root widget.
