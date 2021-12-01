# ProxyEndpointToFile

This is a proxy working as a tunnel with a hole.
1) You need other proxy (easiest is CCProxy), and setup it to accept incoming connections on port 9090
2) You setup some program to work through THIS proxy (configure its proxy settings to use port 9093)
3) Now you connect with your own TCP client to a port 2345 and get a full copy of the data send by a remote.
