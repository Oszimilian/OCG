#!/bin/bash

modprobe vcan

ip link add dev vcan0 type vcan
ip link set up vcan0

ip link add dev vcan1 type vcan
ip link set up vcan1

ip link add dev vcan2 type vcan
ip link set up vcan2

ip link add dev vcan3 type vcan
ip link set up vcan3
