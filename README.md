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