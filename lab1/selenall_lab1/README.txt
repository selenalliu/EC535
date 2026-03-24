Name: Selena Liu
BU Email: selenall@bu.edu

Initially, to minimize the system (build a minimum rootfs image and minimum kernel image), stripping the "letters" binary (in task 3) and removing some unnecessary modules allowed for the filesystem to fit on a disk image of size 3 MB.

To further minimize the rootfs image, I attempted to remove some more modules and seemingly unused binaries. However, I ended up mistakingly removing an important module/binary, and QEMU was no longer loading properly. As such, I decided to restore my rootfs to a backup. But while using the backup, the rootfs image suddenly began to need at least 700 MB of space to fully encapsulate the system. 

This was most likely caused by the symlinks not being properly preserved when I created the backup of the rootfs folder. However, I came to this conclusion only after 6 hours straight of troubleshooting, while losing more and more of my sanity throughout this process. After resolving the issue, I was able to get my root.img to a size of 24MB. I was too traumatized to remove any modules again, which is why my kernel config has been left at basically default settings.