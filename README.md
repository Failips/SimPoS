# SimPoS
### Simulator for Proof of Stake protocols

Extended NS-3 based Bitcoin Simulator (http://arthurgervais.github.io/Bitcoin-Simulator/index.html)

Implemented simulator for PoS protocols Algorand, Gasper and hybrid protocol CasperFFG. 
Tested on NS-3.31

### Table of Contents
1. [Instalation](#installation)
    1. [Installing with full SimPoS archive](#installing-with-full-simpos-archive)
    2. [Installing from scratch](#installing-from-scratch)
2. [Compile](#compile)
3. [Run simulation](#run-simulation)
3. [Folders description](#folders-description)

### Instalation

There are two possible ways for installation.

1) [Use fully prepared archive containing NS3-3.31, Libsodium, RapidJSON and SimPoS source code.](#installing-with-full-simpos-archive)

2) [Use only SimPoS source code and install NS3 with other packages manually from scratch.](#installing-from-scratch)

#### Installing with full SimPoS archive

1. Download archive from: TODO doplnit cestu
2. Unpack archive wherewhere you want (this path is in this manual used under $DATADIR) 
3. Prepare environment variables for libsodium use: 

    `echo 'export LD_LIBRARY_PATH="$DATADIR/ns-allinone-3.31/ns-3.31/libsodium/lib/"' > .simposenv && source .simposenv`

4. Now should be environment prepared and you can continue with step [Compile](#compile).

#### Installing from scratch

These instructions are based on [Bitcoin Simulator installation steps](http://arthurgervais.github.io/Bitcoin-Simulator/Installation.html) with extension for libsodium support.

1. Clone SimPoS simulator. ([link](https://github.com/Failips/SimPoS.git))
2. Download a suitable release from [**NS3** official pages](https://www.nsnam.org/) (tested on version 3.31). Installation instructions for various platforms can be found [here](https://www.nsnam.org/wiki/Installation).
3. Untar downloaded archive:

    `tar xvfj ns-allinone-3.31.tar.bz2`

4. Download **RapidJSON**. ([link](https://github.com/Tencent/rapidjson))
5. Create a new directory named **rapidjson** under **ns-allinone-3.xx/ns-3.xx** and copy the contents of the directory **include/rapidjson** (from the downloaded rapidjson project in step 4) to the newly created rapidjson folder (**ns-allinone-3.xx/ns-3.xx/rapidjson**). You also have to copy all the files from **SimPoS** to the respective folders under **ns-allinone-3.xx/ns-3.xx/**. All the above can be done automatically by the provided **copy-to-ns3.sh** script.
6. Update the **ns-allinone-3.xx/ns-3.xx/src/applications/wscript**.

- Add the following lines in **module.source**:

```
    'model/bitcoin.cc',
    'model/blockchain.cpp',
    'model/bitcoin-node.cc',
    'model/bitcoin-miner.cc',
    'model/bitcoin-simple-attacker.cc',
    'model/bitcoin-selfish-miner.cc',
    'model/bitcoin-selfish-miner-trials.cc',
    'model/algorand/algorand-node.cpp',
    'model/algorand/algorand-participant.cpp',
    'model/casper/casper-node.cpp',
    'model/casper/casper-participant.cpp',
    'model/casper/casper-miner.cpp',
    'model/gasper/gasper-node.cpp',
    'model/gasper/gasper-participant.cpp',
    'helper/bitcoin-topology-helper.cc',
    'helper/bitcoin-node-helper.cc',
    'helper/bitcoin-miner-helper.cc',
    'helper/algorand/algorand-participant-helper.cpp',
    'helper/casper/casper-participant-helper.cpp',
    'helper/gasper/gasper-participant-helper.cpp',
```

- Add the following lines in **headers.source**:

```
    'model/bitcoin.h',
    'model/blockchain.h',
    'model/bitcoin-node.h',
    'model/bitcoin-miner.h',
    'model/bitcoin-simple-attacker.h',
    'model/bitcoin-selfish-miner.h',
    'model/bitcoin-selfish-miner-trials.h',
    'model/algorand/algorand-node.h',
    'model/algorand/algorand-participant.h',
    'model/casper/casper-node.h',
    'model/casper/casper-participant.h',
    'model/casper/casper-miner.h',
    'model/gasper/gasper-node.h',
    'model/gasper/gasper-participant.h',
    'helper/bitcoin-topology-helper.h',
    'helper/bitcoin-node-helper.h',
    'helper/bitcoin-miner-helper.h',
    'helper/algorand/algorand-participant-helper.h',
    'helper/casper/casper-participant-helper.h',
    'helper/gasper/gasper-participant-helper.h',
```

7. Update the ns-allinone-3.xx/ns-3.xx/src/internet/wscript.

- Add the following line in **obj.source**:
```
    'helper/ipv4-address-helper-custom.cc',
```
- Add the following line in **headers.source**:
```
    'helper/ipv4-address-helper-custom.h',
```

8. **LibSodium installation**:

- install libsodium from https://github.com/algorand/libsodium.git , instructions are here: https://doc.libsodium.org/installation
```
    ./configure
    make && make check
    sudo make install
```

- if those instructions will not work for you (or you don't have sudo permissions) do this:
```
git clone git@github.com:algorand/libsodium.git
cd libsodium
sh autogen.sh
./configure
make
  
vim ~/.bashrc
//add following lines:
export PATH="$DATADIR/libsodium-install/bin:$PATH"
export LD_LIBRARY_PATH="$DATADIR/libsodium-install/lib/"

//run command
source ~/.bashrc
make install

mkdir -p $DATADIR/ns-allinone-3.31/ns-3.31/libsodium/
cp -r $DATADIR/libsodium-install/* $DATADIR/ns-allinone-3.31/ns-3.31/libsodium/
```

9. Prepare environment variables for libsodium use in SimPoS: 

    `echo 'export LD_LIBRARY_PATH="$DATADIR/ns-allinone-3.31/ns-3.31/libsodium/lib/"' > .simposenv && source .simposenv`

10. Now should be environment prepared and you can continue with step [Compile](#compile).

### Compile

1. Enter the NS3 directory
    `cd $DATADIR/ns-allinone-3.31/ns-3.31/`
2. When compiling, you have two options:
    1. **optimized build** - should be faster, but with many CPUs it sometimes have memory issues at run
```
    CXXFLAGS="-std=c++11 -I$DATADIR/ns-allinone-3.31/ns-3.31/libsodium/include" LDFLAGS="-L$DATADIR/ns-allinone-3.31/ns-3.31/libsodium/lib -lsodium" ./waf configure --build-profile=optimized --out=build/optimized --with-pybindgen=$DATADIR/ns-allinone-3.31/pybindgen-0.21.0 --enable-mpi --enable-static
```

    2. **debug build** - is little bit slower, but is working fine even with higher number of CPUs. Also support NS_LOG option, so you can trace complete background of simulation
```
    CXXFLAGS="-std=c++11 -g -I$DATADIR/ns-allinone-3.31/ns-3.31/libsodium/include" LDFLAGS="-L$DATADIR/ns-allinone-3.31/ns-3.31/libsodium/lib -lsodium" ./waf configure --build-profile=debug --out=build/debug --with-pybindgen=$DATADIR/ns-allinone-3.31/pybindgen-0.21.0 --enable-mpi --enable-static
```

3. Clean solution:
```
./waf clean

# this file is making issues so we need to delete it always
rm -rf "$NS3/build/debug/compile_commands.json"
rm -rf "$NS3/build/compile/compile_commands.json"
```

4. Build the solution:
```
./waf
```

### Run simulation

For simulation start enter command with following syntax:
```
./waf --run "<scenario> [<params>]"
```

Proof of stake protocols can be launched simply like: 

```
./waf --run "algorand-test"
./waf --run "casper-test"
./waf --run "gasper-test"
```

Each protocol supports different parameters. To list them you can use parameter `--help`.

For faster run, simulator support paralell simulation over multiple processors with use of MPI. Tu run simulator with MPI, launch it this way:
```
mpirun -np <number_of_processors> ./waf --run "<scenario> [<params>]"
```

### Folders description

Desription of important folders containing source files.

```
SimPos  
|_scratch           # contains files which are used for simulations setup and run
| |_bitcoin-test.cc/.h                      # Bitcoin, Litecoin, Dogecoin simulation setup and run
| |_selfish-miner-test.cc/.h                # Bitcoin, Litecoin, Dogecoin simulation with selfish miner setup and run
| |_algorand-test.cc/.h                     # Algorand simulation setup and run
| |_casper-test.cc/.h                       # Casper FFG simulation setup and run
| |_gasper-test.cc/.h                       # Gasper simulation setup and run
|
|_src               # folder with all classes used for simulation of blockchain protocols
  |_applications    # classes used to create nodes and simulate behavior of protocols
  | |_helper        # classes used for creating network topology and setup of blockchain nodes
  | | |_algorand    # setup of Algorand nodes
  | | | |_algorand-participant-helper.cpp/.h # Algorand participants setup
  | | |
  | | |_casper      # setup of Casper FFG nodes
  | | | |_casper-participant-helper.cpp/.h  # Casper FFG participants setup
  | | |
  | | |_gasper      # setup of Gasper nodes
  | | | |_gasper-participant-helper.cpp/.h  # Gasper participants setup
  | | |
  | | |_bitcoin-miner-helper.cc/.h          # Bitcoin miner, selfish miner, simple attacker setup
  | | |_bitcoin-node-helper.cc/.h           # Bitcoin node setup
  | | |_bitcoin-topology-helper.cc/.h       # network topology setup
  | | 
  | |_model         # classes for simulation of blockchain protocols
  |   |_algorand    # classes for simulation of Algorand nodes behavior
  |   | |_algorand-node.cpp/.h              # additional nodes (can be used for creating of transactions in future extensions) 
  |   | |_algorand-participant.cpp/.h       # participants that are creating the blockchain ledger
  |   |
  |   |_casper      # classes for simulation of Casper FFG nodes behavior (including miners with finalization support)
  |   | |_casper-node.cpp/.h                # additional nodes (can be used for creating of transactions in future extensions) 
  |   | |_casper-miner.cpp/.h               # Bitcoin miners with implemented finalization support
  |   | |_casper-participant.cpp/.h         # participants that are finalizing the blockchain ledger
  |   |
  |   |_gasper      # classes for simulation of Gasper nodes behavior
  |   | |_gasper-node.cpp/.h                # additional nodes (can be used for creating of transactions in future extensions)
  |   | |_gasper-participant.cpp/.h         # participants that are creating the blockchain ledger
  |   |
  |   |_bitcoin.cc/.h                       # bitcoin chunk implementation
  |   |_bitcoin-miner.cc/.h                 # bitcoin miner implementation
  |   |_bitcoin-node.cc/.h                  # bitcoin node implementation, containing handlers for packet sending and receiving
  |   |_bitcoin-selfish-miner.cc/.h         # bitcoin selfish miner implementation
  |   |_bitcoin-selfish-miner-trials.cc/.h  # bitcoin selfish miner trials implementation
  |   |_bitcoin-simple-attacker.cc/.h       # bitcoin simple attacker implementation
  |   |_blockchain.cc/.h                    # implementation of blocks and blockchain, containing other structures
  |
  |_internet        # classes extending basic NS3 internet package
    |_ipv4-address-helper-custom.cc/.h      # Bitcoin Simulator IPV4 address support 
```