#!/bin/bash

# Configure the following values
RAPIDJSON_FOLDER=~/Documents/DP/rapidjson
NS3_FOLDER=~/Documents/DP/ns-allinone-3.31/ns-3.31

# Copying files to NS3 simulator
# -> uncomment RapidJson lines if it is not copied yet
mkdir -p $NS3_FOLDER/rapidjson
cp -r $RAPIDJSON_FOLDER/include/rapidjson/* $NS3_FOLDER/rapidjson/
cp -r src/applications/model/* $NS3_FOLDER/src/applications/model/
cp -r src/applications/helper/* $NS3_FOLDER/src/applications/helper/
cp -r src/internet/helper/* $NS3_FOLDER/src/internet/helper/
cp -r scratch/* $NS3_FOLDER/scratch/
