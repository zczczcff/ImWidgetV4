package com.imwidgetv4.android;

import android.app.ActivityManager;
import android.app.NativeActivity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.FrameLayout;

public class ImWidgetNativeActivity extends NativeActivity {
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private EditText keyboardProxyView;
    private long nativeBackendHandle;
    private boolean suppressProxyCallbacks;

    private static native void nativeOnTextInput(long backendHandle, int codePoint);
    private static native void nativeOnSpecialKey(long backendHandle, int keyCode, boolean isDown);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ensureKeyboardProxyView();
    }

    public void showNativeKeyboard() {
        mainHandler.post(() -> {
            ensureKeyboardProxyView();
            if (keyboardProxyView == null) {
                return;
            }

            keyboardProxyView.requestFocus();
            keyboardProxyView.setSelection(keyboardProxyView.getText().length());

            InputMethodManager inputMethodManager =
                (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
            if (inputMethodManager != null) {
                inputMethodManager.showSoftInput(keyboardProxyView, InputMethodManager.SHOW_IMPLICIT);
            }
        });
    }

    public void hideNativeKeyboard() {
        mainHandler.post(() -> {
            View targetView = keyboardProxyView != null ? keyboardProxyView : getWindow().getDecorView();
            InputMethodManager inputMethodManager =
                (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
            if (inputMethodManager != null && targetView != null) {
                inputMethodManager.hideSoftInputFromWindow(targetView.getWindowToken(), 0);
            }

            if (keyboardProxyView != null) {
                keyboardProxyView.clearFocus();
            }
        });
    }

    public void setNativeClipboardText(String text) {
        ClipboardManager clipboardManager =
            (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        if (clipboardManager == null) {
            return;
        }

        clipboardManager.setPrimaryClip(
            ClipData.newPlainText("ImWidgetV4", text != null ? text : ""));
    }

    public String getNativeClipboardText() {
        ClipboardManager clipboardManager =
            (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        if (clipboardManager == null || !clipboardManager.hasPrimaryClip()) {
            return "";
        }

        ClipData primaryClip = clipboardManager.getPrimaryClip();
        if (primaryClip == null || primaryClip.getItemCount() <= 0) {
            return "";
        }

        CharSequence text = primaryClip.getItemAt(0).coerceToText(this);
        return text != null ? text.toString() : "";
    }

    public void setNativeTaskDescription(String label, byte[] rgbaBytes, int width, int height) {
        mainHandler.post(() -> {
            try {
                if (rgbaBytes == null || width <= 0 || height <= 0) {
                    setTaskDescription(new ActivityManager.TaskDescription(label));
                    return;
                }

                int[] argbPixels = new int[width * height];
                for (int index = 0; index < argbPixels.length; ++index) {
                    int baseOffset = index * 4;
                    int red = rgbaBytes[baseOffset] & 0xFF;
                    int green = rgbaBytes[baseOffset + 1] & 0xFF;
                    int blue = rgbaBytes[baseOffset + 2] & 0xFF;
                    int alpha = rgbaBytes[baseOffset + 3] & 0xFF;
                    argbPixels[index] = (alpha << 24) | (red << 16) | (green << 8) | blue;
                }

                Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
                bitmap.setPixels(argbPixels, 0, width, 0, 0, width, height);
                setTaskDescription(new ActivityManager.TaskDescription(label, bitmap, 0));
            } catch (Throwable ignored) {
            }
        });
    }

    public void setNativeBackendHandle(long backendHandle) {
        nativeBackendHandle = backendHandle;
    }

    private void ensureKeyboardProxyView() {
        if (keyboardProxyView != null) {
            return;
        }

        View contentView = findViewById(android.R.id.content);
        if (!(contentView instanceof ViewGroup)) {
            return;
        }

        FrameLayout.LayoutParams layoutParams = new FrameLayout.LayoutParams(1, 1);
        layoutParams.gravity = Gravity.TOP | Gravity.START;

        EditText proxyView = new EditText(this);
        proxyView.setLayoutParams(layoutParams);
        proxyView.setAlpha(0.0f);
        proxyView.setBackground(null);
        proxyView.setSingleLine(true);
        proxyView.setFocusable(true);
        proxyView.setFocusableInTouchMode(true);
        proxyView.setImeOptions(EditorInfo.IME_FLAG_NO_EXTRACT_UI | EditorInfo.IME_ACTION_DONE);
        proxyView.setInputType(
            InputType.TYPE_CLASS_TEXT |
            InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS |
            InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
        proxyView.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }

            @Override
            public void afterTextChanged(Editable editable) {
                if (suppressProxyCallbacks || editable == null || editable.length() == 0) {
                    return;
                }

                if (nativeBackendHandle != 0L) {
                    String text = editable.toString();
                    int offset = 0;
                    while (offset < text.length()) {
                        int codePoint = text.codePointAt(offset);
                        nativeOnTextInput(nativeBackendHandle, codePoint);
                        offset += Character.charCount(codePoint);
                    }
                }

                suppressProxyCallbacks = true;
                editable.clear();
                suppressProxyCallbacks = false;
            }
        });
        proxyView.setOnKeyListener((view, keyCode, event) -> {
            if (nativeBackendHandle == 0L) {
                return false;
            }

            if (keyCode != KeyEvent.KEYCODE_DEL &&
                keyCode != KeyEvent.KEYCODE_FORWARD_DEL &&
                keyCode != KeyEvent.KEYCODE_ENTER &&
                keyCode != KeyEvent.KEYCODE_NUMPAD_ENTER &&
                keyCode != KeyEvent.KEYCODE_TAB) {
                return false;
            }

            if (event.getAction() == KeyEvent.ACTION_DOWN) {
                nativeOnSpecialKey(nativeBackendHandle, keyCode, true);
                return true;
            }

            if (event.getAction() == KeyEvent.ACTION_UP) {
                nativeOnSpecialKey(nativeBackendHandle, keyCode, false);
                return true;
            }

            return false;
        });

        ((ViewGroup)contentView).addView(proxyView);
        keyboardProxyView = proxyView;
    }
}
