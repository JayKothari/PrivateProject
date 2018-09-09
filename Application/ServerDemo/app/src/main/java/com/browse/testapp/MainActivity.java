package com.browse.testapp;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;

public class MainActivity extends AppCompatActivity implements View.OnClickListener
{

    Button btnStart, btnStop;


    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        btnStart = (Button) findViewById(R.id.btnStart);
        btnStop = (Button) findViewById(R.id.btnStop);

        btnStart.setOnClickListener(this);
        btnStop.setOnClickListener(this);
    }


    boolean mShouldListen;
    String TAG = "M";

    void startListening()
    {
        try
        {
            mShouldListen = true;
            DatagramSocket datagramSocket = new DatagramSocket(58266);

            byte[] data = new byte[1024];


            // ----- Receive ----

            while (mShouldListen)
            {
                DatagramPacket packet = new DatagramPacket(data, data.length);

                datagramSocket.receive(packet);

                Log.i(TAG, "startListening: receiving....");

                String recString = new String(packet.getData());

                Log.i(TAG, "startListening: received String : " + recString);

                InetAddress inetAddress = packet.getAddress();
                int port = packet.getPort();


                Log.i(TAG, "startListening: IPAddress : " + inetAddress.getHostAddress());
                Log.i(TAG, "startListening: Port      : " + port);

            }

        }
        catch (SocketException e)
        {
            e.printStackTrace();

        } catch (IOException e)
        {

            e.printStackTrace();
        }
    }

    @Override
    public void onClick(View v)
    {
        switch (v.getId())
        {
            case R.id.btnStart:
            {
                Thread thread = new Thread(new Runnable()
                {
                    @Override
                    public void run()
                    {
                        startListening();
                    }
                });

                thread.start();
            }
            break;

            case R.id.btnStop:
            {
                mShouldListen = false;
            }
            break;
        }
    }
}
