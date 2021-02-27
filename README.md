# SimPoS
### Simulator for Proof of Stake protocols

Extended NS-3 based Bitcoin Simulator (http://arthurgervais.github.io/Bitcoin-Simulator/index.html)

Tested on NS-3.31
For installation follow Bitcoin Simulator installation steps

### instalation pre steps:
- install libsodium from git@github.com:algorand/libsodium.git
- instructions are here: https://doc.libsodium.org/installation
      ./configure
      make && make check
      sudo make install

- if they will not work (or you dont have sudo permissions) do this:
git clone git@github.com:algorand/libsodium.git
cd libsodium
sh autogen.sh
./configure
make
  
vim ~/.bashrc
//add following lines:
export PATH="$HOME/myapps/bin:$PATH"
export LD_LIBRARY_PATH="$HOME/myapps/lib/"

//run command
source ~/.bashrc
make install

mkdir -p $HOME/ns-allinone-3.31/ns-3.31/libsodium/
cp -r $HOME/myapps/* $HOME/ns-allinone-3.31/ns-3.31/libsodium/

CXXFLAGS="-std=c++11 -g  -I$HOME/ns-allinone-3.31/ns-3.31/libsodium/include" LDFLAGS="-L$HOME/ns-allinone-3.31/ns-3.31/libsodium/lib -lsodium" ./waf configure --build-profile=debug --out=build/debug --with-pybindgen=$SCRATCHDIR/ns-allinone-3.31/pybindgen-0.21.0 --enable-mpi --enable-static

