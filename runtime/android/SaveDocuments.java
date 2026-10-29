package com.silvershadow.pokerogue.nativehandheld;

import android.app.Activity;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.provider.OpenableColumns;
import android.widget.Toast;
import java.io.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Android document access for the offline game's existing import/export flow. */
final class SaveDocuments {
    private static final int IMPORT = 8101, EXPORT = 8102;
    private static final int MAX_IMPORT_BYTES = 32 * 1024 * 1024;
    private final Activity activity;
    private final ExecutorService worker = Executors.newSingleThreadExecutor();
    private volatile Object[] result;
    private File pendingExport;
    private boolean busy;
    SaveDocuments(Activity activity) { this.activity = activity; }

    void requestImport() {
        activity.runOnUiThread(() -> {
            if (busy) return;
            busy = true;
            result = null;
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("*/*"); // Official encrypted saves use .prsv, not JSON.
            try { activity.startActivityForResult(intent, IMPORT); }
            catch (RuntimeException ex) { busy = false; result = new Object[0]; message("Cannot open the file picker."); }
        });
    }

    synchronized Object[] takeImport() {
        Object[] value = result;
        result = null;
        return value; // null: pending; empty: cancelled; otherwise [name, bytes].
    }

    void requestExport(String name) {
        activity.runOnUiThread(() -> {
            if (busy) { message("Finish the current file operation first."); return; }
            // The game supplies a filename, never an arbitrary filesystem path.
            if (name.isEmpty() || name.contains("/") || name.contains("\\") || name.equals("..")) return;
            File source = new File(new File(activity.getFilesDir(), "exports"), name);
            if (!source.isFile()) { message("The save export could not be prepared."); return; }
            busy = true;
            pendingExport = source;
            Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("application/octet-stream");
            intent.putExtra(Intent.EXTRA_TITLE, name);
            try { activity.startActivityForResult(intent, EXPORT); }
            catch (RuntimeException ex) { busy = false; pendingExport = null; message("Cannot open the file picker."); }
        });
    }

    boolean onResult(int request, int status, Intent data) {
        if (request != IMPORT && request != EXPORT) return false;
        Uri uri = data == null ? null : data.getData();
        if (status != Activity.RESULT_OK || uri == null) {
            if (request == IMPORT) result = new Object[0];
            pendingExport = null; busy = false; return true;
        }
        File source = pendingExport;
        pendingExport = null;
        worker.execute(() -> {
            try {
                if (request == IMPORT) {
                    String name = "save.prsv";
                    try (Cursor cursor = activity.getContentResolver().query(uri,
                            new String[]{OpenableColumns.DISPLAY_NAME}, null, null, null)) {
                        if (cursor != null && cursor.moveToFirst() && !cursor.isNull(0)) name = cursor.getString(0);
                    }
                    try (InputStream in = activity.getContentResolver().openInputStream(uri);
                         ByteArrayOutputStream out = new ByteArrayOutputStream()) {
                        if (in == null) throw new IOException("No input stream");
                        byte[] buffer = new byte[32768];
                        for (int n; (n = in.read(buffer)) != -1;) {
                            if ((long)out.size() + n > MAX_IMPORT_BYTES) throw new IOException("Save exceeds 32 MiB");
                            out.write(buffer, 0, n);
                        }
                        result = new Object[]{name, out.toByteArray()};
                    }
                } else {
                    if (source == null) throw new IOException("No export pending");
                    try (InputStream in = new FileInputStream(source);
                         OutputStream out = activity.getContentResolver().openOutputStream(uri, "wt")) {
                        if (out == null) throw new IOException("No output stream");
                        byte[] buffer = new byte[32768];
                        for (int n; (n = in.read(buffer)) != -1;) out.write(buffer, 0, n);
                    }
                    message("Save exported.");
                }
            } catch (IOException | RuntimeException ex) {
                if (request == IMPORT) result = new Object[0];
                message(request == IMPORT ? "Could not read this save (maximum 32 MiB)." : "Could not export the save.");
            } finally { activity.runOnUiThread(() -> busy = false); }
        });
        return true;
    }

    private void message(String text) {
        activity.runOnUiThread(() -> Toast.makeText(activity, text, Toast.LENGTH_LONG).show());
    }
    void close() { worker.shutdown(); }
}
