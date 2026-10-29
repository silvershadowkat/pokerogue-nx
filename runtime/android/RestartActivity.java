package com.silvershadow.pokerogue.nativehandheld;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.content.Intent;
import android.system.Os;
import android.system.ErrnoException;
import android.system.OsConstants;
import android.widget.TextView;

/** A short-lived separate process lets SDL restart with a fresh native heap. */
public final class RestartActivity extends Activity {
    private final Handler handler = new Handler();
    private int attempts;
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        TextView text = new TextView(this);
        text.setText("Restarting PokéRogue…");
        text.setGravity(android.view.Gravity.CENTER);
        setContentView(text);
        int pid = getIntent().getIntExtra("oldPid", -1);
        handler.postDelayed(new Runnable() {
            public void run() {
                boolean alive = pid > 0;
                if (alive) {
                    try { Os.kill(pid, 0); }
                    catch (ErrnoException ex) { alive = ex.errno != OsConstants.ESRCH; }
                }
                if (alive && ++attempts < 100) { handler.postDelayed(this, 100); return; }
                if (alive) { text.setText("Please close and reopen PokéRogue to finish restarting."); return; }
                Intent intent = new Intent(RestartActivity.this, NativeActivity.class);
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
                startActivity(intent);
                finish();
            }
        }, 300);
    }
    @Override public void onDestroy() {
        handler.removeCallbacksAndMessages(null);
        super.onDestroy();
        android.os.Process.killProcess(android.os.Process.myPid());
    }
}
