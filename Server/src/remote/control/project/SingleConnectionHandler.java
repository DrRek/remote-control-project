package remote.control.project;

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
				client.send(scanner.nextLine());
			}
		}
		catch(Exception e) {}
	}
	
	private Connect client;
	Scanner scanner = new Scanner(System.in);
}
