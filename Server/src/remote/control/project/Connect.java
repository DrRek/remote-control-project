package remote.control.project;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.net.Socket;

class Connect {
	public Connect() {}

	public Connect(Socket clientSocket)
	{
		client = clientSocket;
		try
		{
			in = new BufferedReader(
					new InputStreamReader(client.getInputStream(), "UTF-16LE"));
			out = new PrintStream(client.getOutputStream(), true, "UTF-16LE");
		}
		catch(Exception e1)
		{
			try { client.close(); }
			catch(Exception e) { System.out.println(e.getMessage());}
			return;
		}
	}

	public String recive(){
		try{
			String recived = "";
			Boolean recive = true;
			while(recive){
				recived = recived + in.readLine();
				if(recived.length()>=3 && recived.substring(recived.length()-3, recived.length()).equals("end")){
					recive = false;
					recived = recived.substring(0, recived.length()-3);
				}
			}
			return recived;
		}
		catch(Exception e ){
			e.printStackTrace();
		}
		return null;
	}
	
	public void send(String send){
		String current;
		do{
			if(send.length()>DEFAULT_BUFLEN){
				current = send.substring(0, DEFAULT_BUFLEN);
				send = send.substring(DEFAULT_BUFLEN);
			}
			else{
				current = send;
			}
			out.println(current);
			out.flush();
		}while(send.length()>DEFAULT_BUFLEN);
		out.println("end");
		out.flush();
	}
	

	public int DEFAULT_BUFLEN = 512;
	private Socket client = null;
	BufferedReader in = null;
	PrintStream out = null;
}