#!/bin/bash

NS3_FOLDER=/storage/brno2/home/failips/ns-allinone-3.31/ns-3.31

# Copying files to NS3 simulator
# -> uncomment RapidJson lines if it is not copied yet
scp -r  src/applications/model/* failips@skirit.metacentrum.cz:$NS3_FOLDER/src/applications/model/
scp -r  src/applications/helper/* failips@skirit.metacentrum.cz:$NS3_FOLDER/src/applications/helper/
scp -r  src/internet/helper/* failips@skirit.metacentrum.cz:$NS3_FOLDER/src/internet/helper/
scp -r  scratch/* failips@skirit.metacentrum.cz:$NS3_FOLDER/scratch/
