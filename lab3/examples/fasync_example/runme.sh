#!/bin/sh
# fasync_example demonstration
# Originally by Matthew Yee
# Updated for Linux 4.19 by Anthony Byrne

# Make the device node
mknod /dev/fasync_example c 61 0
# Load the kernel module
insmod km/fasync_example.ko
# Run the userland tester
ul/fasync_tester
