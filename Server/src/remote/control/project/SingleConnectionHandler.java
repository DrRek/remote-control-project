package remote.control.project;

import java.io.BufferedReader;
import java.io.FileReader;
import java.util.Scanner;

public class SingleConnectionHandler extends Thread{

	public SingleConnectionHandler(Connect c) {
		this.client = c;
		this.start();
	}

	public void run()
	{
		try
		{
			while(true){
				System.out.println(client.recive());
				String toSend = scanner.nextLine();
				client.send(toSend);
				if(toSend.startsWith("up")){
					String fileName = toSend.substring(3);
					FileReader fr = new FileReader(fileName);
					BufferedReader br = new BufferedReader(fr);
					
					while ((toSend = br.readLine()) != null) {
						client.send(toSend);
					}
					br.close();
					fr.close();
					client.send("fileend");
					client.send("end");
				}
			}
		}
		catch(Exception e) {}
	}
	
	private Connect client;
	Scanner scanner = new Scanner(System.in);
}
