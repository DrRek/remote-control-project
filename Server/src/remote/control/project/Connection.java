package remote.control.project;

import java.net.ServerSocket;
import java.net.Socket;

public class Connection extends Thread{
	public Connection()
	{
		try{
			Server = new ServerSocket(27015);
			System.out.println("Il Server è in attesa sulla porta 27015.");
			this.start();
		}
		catch(Exception e){
			e.printStackTrace();
		}
	}
	public void run()
	{
		while(true)
		{
			try {
				System.out.println("In attesa di Connessione.");
				Socket client = Server.accept();
				System.out.println("Connessione accettata da: "+ client.getInetAddress());
				Connect c = new Connect(client);
				new SingleConnectionHandler(c);
				//Qui si può ritornare il connect in modo da poter gestire più connessioni
			}
			catch(Exception e) {
				e.printStackTrace();
			}
		}
	}

	private ServerSocket Server;
}
