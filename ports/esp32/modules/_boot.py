import gc
import os
from esp32 import Partition
from flashbdev import bdev

if mapfs := Partition.find(Partition.TYPE_DATA, label="mapfs"):
    uos.mount(uos.VfsMap(mapfs[0].mmap(), mapfs[0]), "/mapfs")
del mapfs

try:
    if bdev:
        os.mount(bdev, "/")
except OSError:
    import inisetup

    vfs = inisetup.setup()

gc.collect()
