## MPINET
MPINET is a large-scale HPC network simulator based on NS-3. MPINET can replay DUMPI trace of real HPC applications, e.g.HPCG.

## Usage
A simple example to show how to  use MPINET to simulate a dragonfly with the HPCG application.
### Building up and Configuring NS-3
1. Build up NS-3 environment.
``` shell
cmake -DCMAKE_BUILD_TYPE=Debug -DNS3_MPI=ON -G 'CodeBlocks - Unix Makefiles' -S . -B build
```

2. Configure NS-3.
``` shell
./ns3 configure --enable-mpi
```

### Preparing Trace Files
1. The HPCG trace files are already in `scrach/rn/traces`, and the file format is as follows.
``` shell
scratch\rn\traces\HPCG\dumpi-2023.03.31.06.35.04-0000.bin
scratch\rn\traces\HPCG\dumpi-2023.03.31.06.35.04-0001.bin
scratch\rn\traces\HPCG\dumpi-2023.03.31.06.35.04-0002.bin
scratch\rn\traces\HPCG\dumpi-2023.03.31.06.35.04-0003.bin
scratch\rn\traces\HPCG\dumpi-2023.03.31.06.35.04-0004.bin
scratch\rn\traces\HPCG\dumpi-2023.03.31.06.35.04-0005.bin
scratch\rn\traces\HPCG\dumpi-2023.03.31.06.35.04-0006.bin
scratch\rn\traces\HPCG\dumpi-2023.03.31.06.35.04-0007.bin
scratch\rn\traces\HPCG\dumpi-2023.03.31.06.35.04-0008.bin
scratch\rn\traces\HPCG\dumpi-2023.03.31.06.35.04-0009.bin
scratch\rn\traces\HPCG\dumpi-2023.03.31.06.35.04-0010.bin
scratch\rn\traces\HPCG\dumpi-2023.03.31.06.35.04-0011.bin
scratch\rn\traces\HPCG\dumpi-2023.03.31.06.35.04-0012.bin
scratch\rn\traces\HPCG\dumpi-2023.03.31.06.35.04-0013.bin
scratch\rn\traces\HPCG\dumpi-2023.03.31.06.35.04-0014.bin
scratch\rn\traces\HPCG\dumpi-2023.03.31.06.35.04-0015.bin
```

1. The AMR, IMB, LULESH, miniFE trace files are already in `scrach/rn/traces`.

2. If you want to replay DUMPI traces of other HPC applications, please follow the above file format.

### Compiling Simulator
1. The default initialization of the simulator is `scrach/rn/dragonfly-test.cpp`. You can make your own initialization of MPINET.

2. Compile the simulator and then you will get the simultor.
``` shell
cd build
make simulator
```

### Running Simulator
1. Run the simulator with the HPCG application.
``` shell
cd build
./simulator ../scrach/rn/traces/HPCG
```

2. MPINET supports a range of typical and cutting-edge network functions. You can enable these network functions through different parameters.
Here's a reformatted version of the parameters and descriptions:

| Parameter                  | Description                                                                                                    |
|----------------------------|----------------------------------------------------------------------------------------------------------------|
| tracePath                  | Path containing binary dumpi traces                                                                        |
| p                          | Number of servers per TOR switch                                     |
| a                          | Number of TOR switches per group                                     |
| h                          | Count of inter-group links per TOR switch                            |
| g                          | Number of groups; set to zero means using balanced dragonfly (g = a * h + 1) |
| bandwidth                  | Bandwidth of the links in the topology                                                                      |
| delay                      | Delay of the links in the topology                                                                          |
| ocs                        | Count of OCS used in the reconfigurable topology, each OCS is connected to a TOR in every group                |
| ugal                       | Whether to use UGAL routing                                                                                     |
| flowRouting                | Whether to use flow routing                                                                                     |
| congestionMonitorPeriod    | Congestion monitor interval                                                                    |
| enable_reconfiguration     | Whether to enable reconfiguration                                                                               |
| reconfiguration_timestep   | Reconfiguration interval                                                                      |
| stop_time                  | Time to stop generating flows, if synthetic traffic is enabled                                                                                  |
| is_adversial               | Whether the traffic pattern is adversarial or neighbor, if synthetic traffic is enabled                                                                                |
| ecmp                       | Whether to enable ECMP                                                                                                     |
| app_bd                     | Bandwidth injected into the network by synthetic traffic, if synthetic traffic is enabled                                                                                                |
| bias                       | Bias for UGAL to determine the preference for adaptive routing or shortest path routing                                                                                                   |
| reconfiguration_count      | Maximum number of reconfigurations                                                                                     |
| only_reconfiguration       | Whether the dragonfly has no background links                                                                                     |


## Brief Introduction
MPINET code are mainlycontained in `scr/mpi-application` and `scratch/rn`. Here's a brief overview of the functions and focus areas of each folder in the context of HPC system support.

### `src/mpi-application`
To replay real HPC application traces, this folder contains implementations of MPI point-to-point and collective communication.These component below collectively form the trace replay module, enabling the simulation and analysis of real HPC application traces within the NS-3 environment.

- **MPI Asynchronous Communication Operation**
   - Implements asynchronous communication operations in a synchronous programming manner.
   
- **MPI Socket and MPI Serialization Protocol**
   - **MPI Socket:** Encapsulates general sockets in NS-3 as MPISockets.
   - **MPI Serialization Protocol:** Handles complex data structures, providing serialization and deserialization interfaces.
   
- **MPI Communicator**
   - Provides MPI point-to-point and collective communication functionalities.
   
- **MPI Application**
    - Start simulation applications.
    - Execute MPI function calls.
    - Stop applications upon completion.

### `scratch/rn/topology`
To provide user-friendly network simulation, Within this directory, various common topologies found in HPC (High-Performance Computing) systems are supported. These include:

- **Supported Topologies**:
  - **Dragonfly**: Implementation of the Dragonfly topology, known for its scalability and fault tolerance in large-scale systems.
  - **Fat-tree**: Implementation of the Fat-tree topology, widely used for its balanced structure and efficient routing in data centers.
  - **Flexfly**: Implementation of the Fat-tree topology, widely used for its balanced structure and efficient routing in data centers.
- **Additional Features**:
  - **Optical Circuit Switching (OCS)**: Integration of OCS into network simulations, exploring advanced switching technologies for enhanced performance.
  - **Reconfiguration Algorithm**: Implementation of a state-of-the-art algorithm for dynamically reconfiguring network topologies, optimizing resource utilization and performance in dynamic environments.
### `scratch/rn/routing`
To provide user-friendly network simulation,This folder supports common routing schemes used in HPC systems, including:

- **UGAL(Uniform Global Adaptive Routing)**: Implementation of adaptive routing algorithms that balance traffic load across network paths, improving overall network efficiency.
- **MIN-UGAL**: Implementation of a routing scheme designed for optimizing network communication in High-Performance Computing (HPC) systems. It combines adaptive routing for inter-group communication with shortest-path routing for intra-group communication.

## Credit
This repository contains code from the repository of NS-3:
* [NS-3](https://github.com/nsnam/ns-3-dev-git/tree/master)

