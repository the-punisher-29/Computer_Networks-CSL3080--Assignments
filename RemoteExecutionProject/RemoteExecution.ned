simple Client {
    parameters:
        int numSubtasks;
    gates:
        output toServer[];
        input in;
        output toClient[];
        input inGossip;
}

simple Server {
    gates:
        input in;
        output toClient[];
}

network RemoteExecution {
    parameters:
        int numServers = default(5);
        int numClients = default(3);
    submodules:
        client[3]: Client {
            @display("p=100,100");
        }
        server[5]: Server {
            @display("p=300,100");
        }
    connections allowunconnected:
        // Explicit Client-Server connections
        client[0].toServer++ --> server[0].in;
        client[0].toServer++ --> server[1].in;
        client[0].toServer++ --> server[2].in;
        client[0].toServer++ --> server[3].in;
        client[0].toServer++ --> server[4].in;

        client[1].toServer++ --> server[0].in;
        client[1].toServer++ --> server[1].in;
        client[1].toServer++ --> server[2].in;
        client[1].toServer++ --> server[3].in;
        client[1].toServer++ --> server[4].in;

        client[2].toServer++ --> server[0].in;
        client[2].toServer++ --> server[1].in;
        client[2].toServer++ --> server[2].in;
        client[2].toServer++ --> server[3].in;
        client[2].toServer++ --> server[4].in;

        // Explicit Server-Client connections
        server[0].toClient++ --> client[0].in;
        server[0].toClient++ --> client[1].in;
        server[0].toClient++ --> client[2].in;

        server[1].toClient++ --> client[0].in;
        server[1].toClient++ --> client[1].in;
        server[1].toClient++ --> client[2].in;

        server[2].toClient++ --> client[0].in;
        server[2].toClient++ --> client[1].in;
        server[2].toClient++ --> client[2].in;

        server[3].toClient++ --> client[0].in;
        server[3].toClient++ --> client[1].in;
        server[3].toClient++ --> client[2].in;

        server[4].toClient++ --> client[0].in;
        server[4].toClient++ --> client[1].in;
        server[4].toClient++ --> client[2].in;

        // Explicit Client-Client Gossip Connections
        client[0].toClient++ --> client[1].inGossip;
        client[0].toClient++ --> client[2].inGossip;

        client[1].toClient++ --> client[0].inGossip;
        client[1].toClient++ --> client[2].inGossip;

        client[2].toClient++ --> client[0].inGossip;
        client[2].toClient++ --> client[1].inGossip;
}
