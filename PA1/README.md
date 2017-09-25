Implementation of Reliable UDP to transfer files >5MB

The folder structure
           server - udp_server.c
           client - udp_client.c

To compile the files
       Go to respective directories
            make server
            make client
How to run
      server - ./server port number
      client - ./client server ip port number

Features Implemented
       get a file from server  -- get when propmted enter filename
                                  read message printed for result
       put a file into server  -- put when prompted enter filename
                                  read message printed for result
       
       delete a file from server -- delete when prompted enter filename
                                    read message printed for result
       list of files             -- ls
        
       exit                      -- exit both client and server

Reliability is implemented with sequence numbers and sending the data twice.
TCP like ACK can be implemented but UDP being a light weight protocol didn't want
to steal the importance. 
if 2 packets with same sequence number and data one is discarded
if 2 packets same sequence number different data one with large size is taken
if 2 packets different sequence number if greater by 1 then both are stored
if 2 packets different sequence number if less then lower is discarded

Morethan 2 packets can also be transmitted but didn't want to complicate so much

So at times you have 2 useful packets on the flight which is better than StopARQ  
