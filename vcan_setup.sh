#!/bin/bash

modprobe vcan

ip link add dev can0 type vcan
ip link set up can0

ip link add dev can1 type vcan
ip link set up can1

ip link add dev can2 type vcan
ip link set up can2

ip link add dev can3 type vcan
ip link set up can3
