# Assignment 2 - Discrete Event Simulation
 - Kritin Gupta (23B0948)
 - Abhilasha Sharma (23B1011)

"How the hell does this work?!" you might ask. I once found myself at that point during this project and then what did I do? I fought the urge to smash something on my head or my head on something and instead started writing this document so that I can use it when I am 70 and still working on this assignment.

## Assumptions
1. ```timerInterrupt``` takes negligible amt of time
2. Server side `timeout` is the same for all requests

## The Flow

### ```run.py```
we run ```run.py``` and supply to it an easier form of the config file which is much less tedious. Now this has the following functions:

1. ```get_server_config```: takes the server config for each file individually

2. ```make_adjacency_matrix```: the main reason there is a higher level of config file (a bigger reason is my laziness maybe but we'll say this is the reason, ok?). Takes just the servers the current server is actually connected to and accordingly constructs the adj matrix (yes, I didn't want to write the extra zeros)

3. ```make_config```: this functions makes the basic directory structure of the results for the specific test and the basic information for the general configuration of the simulation come form here

4. ```welch_cutoff```: this will be done just for the whole server logs. The individual servers may or may not have the same number of samples from a run and I have run out of patience and surrendered to the power of eyeballing


### ```server.h``` and ```server.cpp```

- Server Class
    
    1. ```processEvent```: handles intra server ops such as ctxt switch in/out, starting/finishing service, timeouts (both client and server). Types of events are listed below

        1. ```ENTER_SERVER```: allocs thread/ queues the req for a thread
        2. ```CTX_SWITCH_IN```: when a core finally starts working on some request
        3. ```SERVER_TIMEOUT```: Now get this through that smooth brain as yours, server timeout pe you send a bad response. That response will trigger the think

    2. ```updateMetrics```: writes into the log_file; what do we want? I think service time of req, avg 


- Server Config

    If you do not understand this, take a break, dunk your face in ice cold water and try again


### ```sim.h``` and ```sim.cpp```

 - Request Class
    Each request is an object of request class, stored in a requests vector, passed around, and stores stats for that request

 - ClientNode Class 
    Handles generating new requests after waiting for a sampled think time after recieving a response from the server

 - Event Class
    Each event is an object of Event class, which are stored in `fel`. These store information required to send that event to the correct entity to be processed, and all the information to process it

 - Simulation Class 
    The main class, the one for which all other classes work. It does everything from initializing all other class objects, storing requests, event queue, running the simulation, routing events, and returning results.


## Logs
- Each test has its own directory in `results` directory
- Each test directory contains
```
{test_name}
 |
 |-config.txt
 |-{i}_users
 |  |-run_{i}
 |     |-main.log
 |     |-server_{i}.log
 |
 |-response_times.log
 |-stats.log
```

- __Main Log__: `time, #good, #bad, #dropped, #completed, response-time`
- __Server Log__: `time, #good, #bad, #dropped, response-time, service-time, active-time, # in server indicator`

**Note:** `active-time`/`time-dur` = utilization, `# in server indicator` / `time-dur` = N


In addition to these, we have `results.txt` logs for some experiments involving multiple servers, which contain relative visit counts and response times for each server. All server demands were calculated using values from these files

## Python Scripts

### `run.py` 
You only need to `run.py` with different arguments to run the simulation

 - `python3 run.py sim` : Create config for a simulation (fixed # of users, 10 runs) and run the simulation

- `python3 run.py rerun_sim <dir>` : Rerun the simulation in `<dir>` directory. Supplied directory must contain a simulation `config.txt` file. Note: overwrites old data. use carefully

 - `python3 run.py expt` : Create config for an experiment (Users vary in a range, 10 runs for each load level) and runs multiple simulation. Also, generates plots for metrics vs #users. 
 
 - `python3 run.py rerun_expt <dir>` : Similar to `rerun_sim`, but for experiments

 The options `sim` and `expt` ask for the config input in command line using descriptive promps. Just enter the values, sit back, and wait for the simulation to complete. 

**Example Command**: `python3 run.py rerun_expt ./results/basic_exp/`

 ### `welch.py`

 This contains functions to run welch procedure on a log file, and returns a transient cutoff index

 ### `stats.py`

 This contains a variety of functions for easy plotting of data. Some of these functions are called in other scripts. Whereas others are to avoid writing code repetitively. You can call these functions using a Jupyter Notebook, or using interactive python in the shell. We used the latter (in hindsight, the former seems much better)

