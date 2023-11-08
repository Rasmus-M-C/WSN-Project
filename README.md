# rpl-udp

A simple RPL network with UDP communication. This is a self-contained example:
it includes a DAG root (`udp-server.c`) and DAG nodes (`udp-clients.c`).
This example runs without a border router -- this is a stand-alone RPL network.

The DAG root also acts as UDP server. The DAG nodes are UDP client. The clients
send a UDP request periodically, that simply includes a counter as payload.
When receiving a request, The server sends a response with the same counter
back to the originator.

The `.csc` files show example networks in the Cooja simulator, for sky motes and
for cooja motes.

For this example a "renode" make target is available, to run a 3 node
emulation in the Renode framework. For further instructions on installing and
using Renode please refer to [the documentation][1].

[1]: https://docs.contiki-ng.org/en/develop/doc/tutorials/Running-Contiki-NG-in-Renode.html

The rpl-udp.robot is a Robot framework test for renode. To run that do:

    >make TARGET=cc2538dk
    >renode-test rpl-udp.robot

