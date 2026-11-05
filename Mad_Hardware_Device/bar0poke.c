#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char **argv) {
    const char *path = "/sys/bus/pci/devices/0000:01:00.0/resource0";
    off_t offset = 0;
    uint32_t value = 0xdeadbeef;
    uint32_t readval = 0xffffffff;
    
    if (argc > 1) offset = strtoull(argv[1], NULL, 0);
    if (argc > 2) value  = strtoul(argv[2], NULL, 0);

    int fd = open(path, O_RDWR | O_SYNC);
    if (fd < 0) { perror("open"); return 1; }

    void *map = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) { perror("mmap"); close(fd); return 1; }

    volatile uint32_t *p = (volatile uint32_t *)((char *)map + offset);
    *p = value;
    readval = *p;
    
    printf("Wrote 0x%08x to BAR0+0x%lx; readval=0x%08x\n", 
           value, (unsigned long)offset, readval);

    munmap(map, 4096);
    close(fd);
    return 0;
}
