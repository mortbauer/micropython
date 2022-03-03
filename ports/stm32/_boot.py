import os, sys, pyb


mapfs = pyb.Flash("mapfs")
os.mount(os.VfsMap(mapfs.ioctl(0x100, 0), mapfs), "/mapfs")
sys.path.insert(0, "/mapfs")

del mapfs
