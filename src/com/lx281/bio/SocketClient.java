package com.lx281.bio;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.util.Random;

public class SocketClient {
	public static void main(String[] args) throws Exception {
		int threadCount = 2;
		final Random ran = new Random();
		for (int i = 0; i < threadCount; i++) {
			final int name = i;
			Runnable r = new Runnable() {

				@Override
				public void run() {
					Socket s = null;
					try {
						System.out.format("thread %d startting\n", name);
						s = new Socket("192.168.1.201", 9192);
						OutputStream os = s.getOutputStream();
						int val = ran.nextInt(1000);
						os.write(String.valueOf(val).getBytes());
						os.write("看看到底有能容纳的下多少字符串哈哈哈哈".getBytes());
						os.flush();
						
						InputStream is = s.getInputStream();
						byte[] buf = new byte[128];
						while (is.read(buf) != -1) {
							System.out.print(new String(buf, "utf-8"));
						}
					} catch (Exception e) {
						e.printStackTrace();
						System.err.format("thread %d error!\n", name);
					} finally {
						try {
							s.close();
						} catch (IOException e) {
							e.printStackTrace();
						}
					}
				}
			};
			new Thread(r).start();

			// Thread.sleep(1000);
		}
	}
}
