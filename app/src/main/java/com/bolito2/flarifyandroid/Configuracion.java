package com.bolito2.flarifyandroid;

import android.app.Activity;
import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.SeekBar;

public class Configuracion extends AppCompatActivity {

    SeekBar cara, ojos;
    float pCara = 1.1f, pOjos = 1.05f;
    Button ok;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_configuracion);

        ok = findViewById(R.id.ok);
        cara = findViewById(R.id.cara);
        ojos = findViewById(R.id.ojos);

        ok.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent i = new Intent();
                i.putExtra("pCara", pCara);
                i.putExtra("pOjos", pOjos);

                setResult(Activity.RESULT_OK, i);
                Log.e("ADIOH", "YEE");
                finish();
            }
        });

        cara.setProgress((int)(151 - 100*getIntent().getFloatExtra("pCara", 0f)));
        ojos.setProgress((int)(151 - 100*getIntent().getFloatExtra("pOjos", 0f)));

        cara.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                pCara = 1 + 0.01f*(51 - progress);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });
        ojos.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                pOjos = 1 + 0.01f*(51 - progress);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });
    }
}
