Place optional shared Android assets for all demo apps in this directory.

Supported overrides:
- `fonts/default.ttf`
- `fonts/default.otf`
- `fonts/NotoSansSC-Regular.otf`

When one of these files exists, `ImAndroidGLES3Backend` loads it first and uses it as the default ImGui font before falling back to Android system fonts.
