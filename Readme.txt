The toolkit is only implementing minimal TCP/IP stack - specifically:
    
    > Layer 1
        > Sending and receiving the packets on the interfaces
    
    > Layer 2
        > Learning of Mac addresses
        > ARP resolutions
        > VLAN based routing funtionalities [No STP]

    > Layer 3
        > layer 3 routing

    > Layer 5
        > Application layer [functionalities that are implemented in the pseudo protocol]
        > Can implement as many applications as we want
        > Applications can program the tcp/ip stack to express in packets it wants to receive by conversing
          down to the tcp/ip stack.
    
    > No usages of sockets are involved
        > While developing network protocols no need of sockets
        > In industries the devs generally use socket-interface/APIs behind simple high end APIs that are
          to be invoked by applications


CLI capabilities

    The cli capabilities have been added using libcli library, so that the user can interact with each node of the topology using the cli interface
    The libcli library parses the cli tokens, and validates the cli format and invokes the backend handler for processing in backend code.

    The backend handler could be a function of any of the layer 2,3 or 5
    
    Throughout the pseudo-protocol we have developed a bunch of custom cli's to configure the protocol

    
