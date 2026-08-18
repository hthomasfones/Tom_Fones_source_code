import os
import mmap
import struct

# Adjust this to your device
RESOURCE0 = "/sys/bus/pci/devices/0000:01:00.0/resource0"

# Offset inside BAR (must match your test)
OFFSET = 0xC

# Value to write
VALUE = 0x01234567

# Open BAR
fd = os.open(RESOURCE0, os.O_RDWR | os.O_SYNC)

# Map first 4KB (more than enough for your offset)
mm = mmap.mmap(fd, 0x1000, mmap.MAP_SHARED,
               mmap.PROT_READ | mmap.PROT_WRITE)
#mm.seek(OFFSET+ 0x20000) # outside 128KB BAR
#mm.write(0xdeadbeef)   

# --- READ 1st ---
mm.seek(OFFSET)
data = mm.read(4)
read_val = struct.unpack("<I", data)[0]
print(f"Read  0x{read_val:08x} from offset 0x{OFFSET:x}")

# --- WRITE ---
mm.seek(OFFSET)
mm.write(struct.pack("<I", VALUE))
print(f"Wrote 0x{VALUE:08x} to offset 0x{OFFSET:x}")

# --- READ BACK ---
mm.seek(OFFSET)
data = mm.read(4)
read_val = struct.unpack("<I", data)[0]
print(f"Read  0x{read_val:08x} from offset 0x{OFFSET:x}")

mm.close()
os.close(fd)
