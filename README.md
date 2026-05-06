# ImWidgetV4 Workspace

This repository now uses a split workspace layout:

- `ImWidgetV4/`: the reusable UI library itself
- `ImWidgetV4Editor/`: the editor application built on top of the library

## Build options

The root CMake project is a workspace/super-project. Useful options:

- `IMWIDGETV4_BUILD_TESTS`: build library tests
- `IMWIDGETV4_BUILD_SAMPLES`: build library samples
- `IMWIDGETV4_BUILD_EDITOR`: build the editor application

## Directory layout

```text
ImWidgetV4/
├─ CMakeLists.txt
├─ ImWidgetV4/
│  ├─ include/
│  ├─ src/
│  ├─ tests/
│  └─ samples/
└─ ImWidgetV4Editor/
   ├─ CMakeLists.txt
   └─ src/
```

## Current intent

- Keep framework code inside `ImWidgetV4/`
- Keep editor-specific code inside `ImWidgetV4Editor/`
- Treat `samples/` as library consumers, not editor code
