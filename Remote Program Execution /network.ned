

import ned.IdealChannel;


network Network
{
    submodules:
        server0: server { @display("p=107,63"); }
 server1: server { @display("p=165,63"); }
 server2: server { @display("p=229,63"); }
 server3: server { @display("p=297,63"); }
 server4: server { @display("p=368,63"); }
 server5: server { @display("p=439,63"); }

        client0: client { @display("p=190,175"); }
 client1: client { @display("p=277,230"); }
 client2: client { @display("p=390,245"); }
 client3: client { @display("p=505,230"); }


    connections:
        client0.out0 --> IdealChannel --> server0.in0;
server0.out0 --> IdealChannel --> client0.in0;
 client0.out1 --> IdealChannel --> server1.in0;
server1.out0 --> IdealChannel --> client0.in1;
 client0.out2 --> IdealChannel --> server2.in0;
server2.out0 --> IdealChannel --> client0.in2;
 client0.out3 --> IdealChannel --> server3.in0;
server3.out0 --> IdealChannel --> client0.in3;
 client0.out4 --> IdealChannel --> server4.in0;
server4.out0 --> IdealChannel --> client0.in4;
 client0.out5 --> IdealChannel --> server5.in0;
server5.out0 --> IdealChannel --> client0.in5;
 client1.out0 --> IdealChannel --> server0.in1;
server0.out1 --> IdealChannel --> client1.in0;
 client1.out1 --> IdealChannel --> server1.in1;
server1.out1 --> IdealChannel --> client1.in1;
 client1.out2 --> IdealChannel --> server2.in1;
server2.out1 --> IdealChannel --> client1.in2;
 client1.out3 --> IdealChannel --> server3.in1;
server3.out1 --> IdealChannel --> client1.in3;
 client1.out4 --> IdealChannel --> server4.in1;
server4.out1 --> IdealChannel --> client1.in4;
 client1.out5 --> IdealChannel --> server5.in1;
server5.out1 --> IdealChannel --> client1.in5;
 client2.out0 --> IdealChannel --> server0.in2;
server0.out2 --> IdealChannel --> client2.in0;
 client2.out1 --> IdealChannel --> server1.in2;
server1.out2 --> IdealChannel --> client2.in1;
 client2.out2 --> IdealChannel --> server2.in2;
server2.out2 --> IdealChannel --> client2.in2;
 client2.out3 --> IdealChannel --> server3.in2;
server3.out2 --> IdealChannel --> client2.in3;
 client2.out4 --> IdealChannel --> server4.in2;
server4.out2 --> IdealChannel --> client2.in4;
 client2.out5 --> IdealChannel --> server5.in2;
server5.out2 --> IdealChannel --> client2.in5;
 client3.out0 --> IdealChannel --> server0.in3;
server0.out3 --> IdealChannel --> client3.in0;
 client3.out1 --> IdealChannel --> server1.in3;
server1.out3 --> IdealChannel --> client3.in1;
 client3.out2 --> IdealChannel --> server2.in3;
server2.out3 --> IdealChannel --> client3.in2;
 client3.out3 --> IdealChannel --> server3.in3;
server3.out3 --> IdealChannel --> client3.in3;
 client3.out4 --> IdealChannel --> server4.in3;
server4.out3 --> IdealChannel --> client3.in4;
 client3.out5 --> IdealChannel --> server5.in3;
server5.out3 --> IdealChannel --> client3.in5;

        client0.cout1 --> IdealChannel --> client1.cin1;
 client0.cout2 --> IdealChannel --> client2.cin1;
 client0.cout3 --> IdealChannel --> client3.cin1;
 client1.cout1 --> IdealChannel --> client0.cin2;
 client1.cout2 --> IdealChannel --> client2.cin2;
 client1.cout3 --> IdealChannel --> client3.cin2;
 client2.cout1 --> IdealChannel --> client0.cin3;
 client2.cout2 --> IdealChannel --> client1.cin3;
 client2.cout3 --> IdealChannel --> client3.cin3;
 client3.cout1 --> IdealChannel --> client0.cin1;
 client3.cout2 --> IdealChannel --> client1.cin2;
 client3.cout3 --> IdealChannel --> client2.cin3;


}

