simple PeerClient
{
    parameters:
        int clientId;     // Unique ID
        int numClients;   // Total number of peers
        int numSubtasks;  // How many parts to split the main task into
        @display("i=device/pc2");
    gates:
        inRing, outRing;                  // Ring connections
        inFinger[numClients], outFinger[numClients]; // Finger connections for lookup shortcuts
}
