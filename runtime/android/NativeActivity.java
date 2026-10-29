package com.silvershadow.pokerogue.nativehandheld;

import android.os.Bundle;
import android.util.DisplayMetrics;
import android.content.Intent;
import android.graphics.Color;
import android.view.KeyEvent;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.Button;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import org.libsdl.app.SDLActivity;

/** SDL owns the native surface, input, audio and lifecycle. No WebView. */
public final class NativeActivity extends SDLActivity {
    private SaveDocuments documents;
    private void copySmallAsset(String asset, File output) throws IOException {
        output.getParentFile().mkdirs();
        try (InputStream in = getAssets().open(asset); FileOutputStream out = new FileOutputStream(output)) {
            byte[] buffer = new byte[32768];
            for (int n; (n = in.read(buffer)) != -1;) out.write(buffer, 0, n);
        }
    }

    @Override protected void onCreate(Bundle state) {
        try {
            // Only the small host scripts/certificate need ordinary filesystem
            // paths. The 537 MiB game archive remains inside the APK.
            for (String name : getAssets().list("runtime/js"))
                copySmallAsset("runtime/js/" + name, new File(getFilesDir(), "runtime/js/" + name));
            copySmallAsset("cacert.pem", new File(getFilesDir(), "cacert.pem"));
            File config = new File(getFilesDir(), "config.txt");
            if (!config.exists()) copySmallAsset("config.txt", config);
        } catch (IOException ex) {
            throw new IllegalStateException("Cannot prepare bundled native runtime", ex);
        }
        super.onCreate(state);
        documents = new SaveDocuments(this);
        int barHeight = Math.round(44 * getResources().getDisplayMetrics().density);
        LinearLayout bar = new LinearLayout(this);
        bar.setId(View.generateViewId());
        bar.setBackgroundColor(Color.rgb(18, 25, 36));
        RelativeLayout.LayoutParams barParams = new RelativeLayout.LayoutParams(-1, barHeight);
        barParams.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
        mLayout.addView(bar, barParams);
        RelativeLayout.LayoutParams surfaceParams = new RelativeLayout.LayoutParams(-1, -1);
        surfaceParams.addRule(RelativeLayout.ABOVE, bar.getId());
        mSurface.setLayoutParams(surfaceParams);
        addAction(bar, "Menu", KeyEvent.KEYCODE_ESCAPE);
        addAction(bar, "Back", KeyEvent.KEYCODE_DEL);
        addAction(bar, "Start run", KeyEvent.KEYCODE_ENTER);
        addAction(bar, "Filters / Info", KeyEvent.KEYCODE_C);
        addAction(bar, "Continue", KeyEvent.KEYCODE_SPACE);
    }

    private void addAction(LinearLayout bar, String label, int key) {
        Button button = new Button(this);
        button.setText(label);
        button.setTextSize(12);
        button.setTextColor(Color.rgb(225, 232, 240));
        button.setBackgroundColor(Color.TRANSPARENT);
        button.setAllCaps(false);
        button.setFocusable(false);
        button.setOnClickListener(v -> {
            onNativeKeyDown(key);
            // Let a game frame observe the press before releasing it.
            button.postDelayed(() -> onNativeKeyUp(key), 80);
        });
        bar.addView(button, new LinearLayout.LayoutParams(0, -1, 1));
    }

    /** Called on the SDL thread after the runtime flushes local saves. */
    public void restartGame() {
        runOnUiThread(() -> {
            Intent intent = new Intent(this, RestartActivity.class);
            intent.putExtra("oldPid", android.os.Process.myPid());
            startActivity(intent);
            finish();
        });
    }

    public void pickSaveDocument() { documents.requestImport(); }
    public Object[] pollSaveDocument() { return documents.takeImport(); }
    public void exportSaveDocument(String name) { documents.requestExport(name); }

    @Override protected void onActivityResult(int request, int result, Intent data) {
        if (documents == null || !documents.onResult(request, result, data))
            super.onActivityResult(request, result, data);
    }

    @Override public void onBackPressed() {
        // Android 12+ backgrounds root activities by default. This test app's
        // system Back is an explicit exit; the action bar handles in-game Back.
        finish();
    }

    @Override public boolean dispatchKeyEvent(KeyEvent event) {
        // SDL otherwise consumes KEYCODE_BACK as native input before Android's
        // onBackPressed callback. It has no in-game mapping in this host.
        if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
            if (event.getAction() == KeyEvent.ACTION_UP && !event.isCanceled()) finish();
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    @Override protected String[] getArguments() {
        DisplayMetrics metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getRealMetrics(metrics);
        int w = Math.max(metrics.widthPixels, metrics.heightPixels);
        int h = Math.min(metrics.widthPixels, metrics.heightPixels);
        return new String[] {"--data", getFilesDir().getAbsolutePath(), "--js",
            new File(getFilesDir(), "runtime/js").getAbsolutePath(), "--size", w + "x" + h};
    }

    @Override protected void onDestroy() {
        // This runtime intentionally retains its JS graph until process exit.
        // SDL joins the native main thread (which flushes saves) before returning.
        super.onDestroy();
        if (documents != null) documents.close();
        android.os.Process.killProcess(android.os.Process.myPid());
    }
}
