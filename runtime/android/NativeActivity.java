package com.silvershadow.pokeroguenx;

import android.os.Bundle;
import android.util.DisplayMetrics;
import android.content.Intent;
import android.view.KeyEvent;
import android.view.WindowManager;
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
            // Only runtime scripts/certificates ship in the APK. Game content is
            // fetched by the client, or installed separately for offline play.
            for (String name : getAssets().list("runtime/js"))
                copySmallAsset("runtime/js/" + name, new File(getFilesDir(), "runtime/js/" + name));
            copySmallAsset("cacert.pem", new File(getFilesDir(), "cacert.pem"));
            File config = new File(getFilesDir(), "config.txt");
            if (!config.exists()) copySmallAsset("config.txt", config);
        } catch (IOException ex) {
            throw new IllegalStateException("Cannot prepare bundled native runtime", ex);
        }
        super.onCreate(state);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        documents = new SaveDocuments(this);
        // Input controls are shared with Switch and drawn by the native host.
        // Direct touch therefore uses the full surface without a permanent bar.
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
    public void exportSaveDocument(String name, String profile) { documents.requestExport(name, profile); }

    @Override protected void onActivityResult(int request, int result, Intent data) {
        if (documents == null || !documents.onResult(request, result, data))
            super.onActivityResult(request, result, data);
    }

    @Override public void onBackPressed() {
        // A full exit/relaunch returns to the mode selector with a fresh runtime.
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
