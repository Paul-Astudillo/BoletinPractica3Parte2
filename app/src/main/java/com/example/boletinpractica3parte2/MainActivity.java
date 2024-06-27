package com.example.boletinpractica3parte2;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.app.ProgressDialog;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.provider.MediaStore;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import com.example.boletinpractica3parte2.databinding.ActivityMainBinding;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class MainActivity extends AppCompatActivity {

    private static final int REQUEST_IMAGE_CAPTURE = 1;
    private static final int REQUEST_CAMERA_PERMISSION = 100;
    private static final int REQUEST_IMAGE_SELECT = 2;
    private static final String TAG = "MainActivity";

    private ImageView foto;
    private TextView textView;
    private Button button, captureButton, selectButton;

    static {
        System.loadLibrary("boletinpractica3parte2");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Copiar el archivo SVM desde assets al almacenamiento interno
        copyAsset("svm_model.xml");

        // Inicializar la ruta del modelo en JNI
        String modelPath = getFilesDir().getAbsolutePath() + "/svm_model.xml";
        initModelPath(modelPath);

        foto = findViewById(R.id.imageView);
        textView = findViewById(R.id.textView);
        captureButton = findViewById(R.id.captureButton);
        captureButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (ContextCompat.checkSelfPermission(MainActivity.this, android.Manifest.permission.CAMERA)
                        != PackageManager.PERMISSION_GRANTED) {
                    ActivityCompat.requestPermissions(MainActivity.this, new String[]{android.Manifest.permission.CAMERA}, REQUEST_CAMERA_PERMISSION);
                } else {
                    openCamera();
                }
            }
        });

        selectButton = findViewById(R.id.selectButton);
        selectButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                openGallery();
            }
        });

        button = findViewById(R.id.button);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Bitmap tomada = ((BitmapDrawable) foto.getDrawable()).getBitmap();
                new PredictTask().execute(tomada);
            }
        });
    }

    private void openCamera() {
        Intent takePictureIntent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
        if (takePictureIntent.resolveActivity(getPackageManager()) != null) {
            startActivityForResult(takePictureIntent, REQUEST_IMAGE_CAPTURE);
        }
    }

    private void openGallery() {
        Intent intent = new Intent(Intent.ACTION_PICK, MediaStore.Images.Media.EXTERNAL_CONTENT_URI);
        startActivityForResult(intent, REQUEST_IMAGE_SELECT);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_IMAGE_CAPTURE && resultCode == RESULT_OK) {
            Bundle extras = data.getExtras();
            Bitmap imageBitmap = (Bitmap) extras.get("data");

            // Ajustar el tamaño del Bitmap
            int width = imageBitmap.getWidth();
            int height = imageBitmap.getHeight();
            int newWidth = width * 3; // Aumentar el tamaño a tu preferencia
            int newHeight = height * 3; // Aumentar el tamaño a tu preferencia
            Bitmap resizedBitmap = Bitmap.createScaledBitmap(imageBitmap, newWidth, newHeight, false);

            foto.setImageBitmap(resizedBitmap);
        } else if (requestCode == REQUEST_IMAGE_SELECT && resultCode == RESULT_OK) {
            Uri selectedImage = data.getData();
            try {
                Bitmap imageBitmap = MediaStore.Images.Media.getBitmap(this.getContentResolver(), selectedImage);

                // Ajustar el tamaño del Bitmap
                int width = imageBitmap.getWidth();
                int height = imageBitmap.getHeight();
                int newWidth = width * 3; // Aumentar el tamaño a tu preferencia
                int newHeight = height * 3; // Aumentar el tamaño a tu preferencia
                Bitmap resizedBitmap = Bitmap.createScaledBitmap(imageBitmap, newWidth, newHeight, false);

                foto.setImageBitmap(resizedBitmap);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    private void copyAsset(String filename) {
        InputStream in = null;
        OutputStream out = null;
        try {
            in = getAssets().open(filename);
            String outFileName = getFilesDir().getAbsolutePath() + "/" + filename;
            out = new FileOutputStream(outFileName);
            copyFile(in, out);
            in.close();
            out.flush();
            out.close();

            // Verificar si el archivo fue copiado correctamente
            File file = new File(outFileName);
            if (file.exists()) {
                Log.d(TAG, "El archivo " + filename + " fue copiado exitosamente a " + outFileName);
            } else {
                Log.e(TAG, "Error al copiar el archivo " + filename);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void copyFile(InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[1024];
        int read;
        while ((read = in.read(buffer)) != -1) {
            out.write(buffer, 0, read);
        }
    }

    public native String lbp(Bitmap foto);
    public native void initModelPath(String modelPath);

    private class PredictTask extends AsyncTask<Bitmap, Void, String> {
        private ProgressDialog progressDialog;

        @Override
        protected void onPreExecute() {
            progressDialog = ProgressDialog.show(MainActivity.this, "Prediciendo", "Por favor espera...", true);
        }

        @Override
        protected String doInBackground(Bitmap... bitmaps) {
            return lbp(bitmaps[0]);
        }

        @Override
        protected void onPostExecute(String result) {
            progressDialog.dismiss();
            textView.setText("Categoría: " + result);
        }
    }
}
