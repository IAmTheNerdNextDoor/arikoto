#include <vfs.h>

void vfs_test(void) {
    vfs_mount("/", &ramdisk_ops);
    vfs_create("/roadmap.txt", "Where do we go from here?\n\nList of components working, minimally implemented or still to come:\nFramebuffer: Implemented\nVFS: Minimal implementation\nScheduler: Minimal implementation\nMemory: Minimal implementation\nInput: Minimal implementation\nShell: Minimal implementation\nlibc: Still to come\nTools: Still to come\nGUI: Still to come");
}
