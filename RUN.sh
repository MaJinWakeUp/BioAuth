#!/bin/bash
echo "BioAuth Network Simulation Experiment"
docker build -f Dockerfile.simple -t bioauth . && docker run --rm --privileged bioauth