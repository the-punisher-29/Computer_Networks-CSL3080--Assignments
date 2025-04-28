import inet.common.misc.ThruputMeteringChannel;
import myproject.PeerClient;

// Simple P2P Chord-like network
network P2PChordNetwork
{
    parameters:
        int numClients = default(8);   
        int numSubtasks = default(4);  
    submodules:
        client[numClients]: PeerClient {
            parameters:
                clientId = index;
                numClients = numClients;
                numSubtasks = numSubtasks;
                @display("p=200+200*cos(pi/2+index*2*pi/numClients), 200+200*sin(pi/2+index*2*pi/numClients)");
        }
    connections allowunconnected:
        // Connect clients in a ring
        for i = 0..numClients-1 {
            client[i].outRing++ <--> ThruputMeteringChannel {
                datarate = 100Mbps;
                delay = 10us;
            } <--> client[(i+1)%numClients].inRing++;
        }

        // Setup finger table shortcuts
        int m = ceil(log2(numClients));
        for j = 0..m-1, i = 0..numClients-1 {
            int dest = (i + int(pow(2,j))) % numClients;
            client[i].outFinger[j] <--> ThruputMeteringChannel {
                datarate = 100Mbps;
                delay = 10us;
            } <--> client[dest].inFinger[i]++;
        }
}
