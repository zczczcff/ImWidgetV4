# ImWidgetV4 Editor

This directory contains the editor application built on top of the ImWidgetV4 library.

## Current state

The project currently provides a minimal shell executable linked against:

- `imwidgetv4_core`
- `imwidgetv4_platform_imgui`
- `imwidgetv4_platform_win32_dx11`

## Planned responsibility

This project is intended to own editor-only systems such as:

- document management
- layout and docking workflow built from library widgets
- hierarchy/outline panels
- property inspection panels
- canvas authoring tools
- asset and file workflow
