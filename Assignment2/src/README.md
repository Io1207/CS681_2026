"How the hell does this work?!" you might ask. I once found myself at that point during this project and then what did I do? I fought the urge to smash something on my head or my head on something and instead started writing this document so that I can use it when I am 70 and still working on this assignment.

## The Flow

### ```run.py```
we run ```run.py``` and supply to it an easier form of the config file which is much less tedious. Now this has the following functions:

1. ```get_server_config```: takes the server config for each file individually

2. ```make_adjacency_matrix```: the main reason there is a higher level of config file (a bigger reason is my laziness maybe but we'll say this is the reason, ok?). Takes just the servers the current server is actually connected to and accordingly constructs the adj matrix (yes, I didn't want to write the extra zeros)

3. ```make_config```: this functions makes the basic directory structure of the results for the specific test and the basic information for the general configuration of the simulation come form here

4. ```welch_cutoff```: this will be done just for the whole server logs. The individual servers may or may not have the same number of samples from a run and I have run out of patience and surrendered to the power of eyeballing


### ```server.h and server.cpp```

- Server Class
    
    1. ```processEvent```: handles intra server ops such as ctxt switch in/out, starting/finishing service, timeouts (both client and server). Types of events are listed below

        1. ```ENTER_SERVER```: allocs thread/ queues the req for a thread
        2. ```CTX_SWITCH_IN```: when a core finally starts working on some request
        3. ```

    2. ```updateMetrics```: writes into the log_file; what do we want? I think service time of req, avg 


- Server Config

    If you do not understand this, take a break, dunk your face in ice cold water and try again

- 