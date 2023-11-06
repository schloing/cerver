#!/bin/bash

# chmod +x freeport.sh
# ./freeport.sh [portnum]

sudo fuser -k $1/tcp
