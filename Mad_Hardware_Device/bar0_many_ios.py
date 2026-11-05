import os
import mmap
import struct

RESOURCE0 = "/sys/bus/pci/devices/0000:01:00.0/resource0"
MAP_SIZE  = 0x1000
OFFSET    = 0x0
COUNT     = 10000

fd = os.open(RESOURCE0, os.O_RDWR | os.O_SYNC)
mm = mmap.mmap(fd, MAP_SIZE, mmap.MAP_SHARED, mmap.PROT_READ | mmap.PROT_WRITE)

last = 0
for i in range(COUNT):
    mm.seek(OFFSET)
    last = struct.unpack("<I", mm.read(4))[0]

mm.close()
os.close(fd)

print(f"Last read = 0x{last:08x}")
