package com.example.boletinpractica3parte2;

import androidx.appcompat.app.AppCompatActivity;

import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import com.example.boletinpractica3parte2.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    private ImageView foto , imageView2;
    private Button button;

    // Used to load the 'boletinpractica3parte2' library on application startup.
    static {
        System.loadLibrary("boletinpractica3parte2");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        foto = findViewById(R.id.imageView);
        imageView2 = findViewById(R.id.imageView2);
        button = findViewById(R.id.button);

        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Bitmap tomada = ((BitmapDrawable) foto.getDrawable()).getBitmap();

                lbp(tomada);

                imageView2.setImageBitmap(tomada);

            }
        });





    }

    /**
     * A native method that is implemented by the 'boletinpractica3parte2' native library,
     * which is packaged with this application.
     */
    public native void lbp(android.graphics.Bitmap foto);

}