#!/bin/bash

# Configure the following values
DATADIR="~"
RAPIDJSON_FOLDER=$DATADIR/rapidjson-master
LIBSODIUM_FOLDER=$DATADIR/libsodium-draft-irtf-cfrg-vrf-03
NS3_FOLDER=$DATADIR/ns-allinone-3.31/ns-3.31
SIMPOS_FOLDER=$DATADIR/SimPoS

# Copying files to NS3 simulator
mkdir -p $NS3_FOLDER/rapidjson
cp -r  $RAPIDJSON_FOLDER/include/rapidjson/* $NS3_FOLDER/rapidjson/
cp -r  $SIMPOS_FOLDER/src/applications/model/* $NS3_FOLDER/src/applications/model/
cp -r  $SIMPOS_FOLDER/src/applications/helper/* $NS3_FOLDER/src/applications/helper/
cp -r  $SIMPOS_FOLDER/src/internet/helper/* $NS3_FOLDER/src/internet/helper/
cp -r  $SIMPOS_FOLDER/scratch/* $NS3_FOLDER/scratch/
