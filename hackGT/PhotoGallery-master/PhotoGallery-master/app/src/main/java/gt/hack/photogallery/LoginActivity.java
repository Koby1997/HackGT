package gt.hack.photogallery;

import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

public class LoginActivity extends AppCompatActivity {
    private EditText  username;
    private EditText  password;
    private Button    login_button;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login);

        username = findViewById(R.id.username);
        password = findViewById(R.id.password);
        login_button = findViewById(R.id.login_btn);

        login_button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                String username_text = username.getText().toString();
                String password_text = password.getText().toString();
                if (username_text.length() == 0 || password_text.length() == 0) {
                    String message = "You must enter a username and password!";
                    Toast toast = Toast.makeText(getApplicationContext(), message, Toast.LENGTH_SHORT);
                    toast.show();
                } else {
                    Intent intent = new Intent(getApplicationContext(), PhotoGalleryActivity.class);
                    startActivity(intent);
                }
            }
        });

    }
}
