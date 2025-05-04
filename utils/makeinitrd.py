import os

def pad4(x): return (x + 3) & ~3

def write_newc_entry(f, name, data, mode):
    fields = [
        b'070701',
        b'00000001',
        ("%08X" % mode).encode(),
        b'00000000',
        b'00000000',
        b'00000001',
        b'00000000',
        ("%08X" % len(data)).encode(),
        b'00000000',
        b'00000000',
        b'00000000',
        b'00000000',
        ("%08X" % (len(name)+1)).encode(),
        b'00000000',
    ]
    f.write(b''.join(fields))
    f.write(name.encode() + b'\0')
    f.write(b'\0' * (pad4(len(name)+1) - (len(name)+1)))
    f.write(data)
    f.write(b'\0' * (pad4(len(data)) - len(data)))

def add_dir(f, rootdir):
    seen_dirs = set()
    for dirpath, dirnames, filenames in os.walk(rootdir):
        rel_dir = os.path.relpath(dirpath, rootdir)
        if rel_dir == ".":
            rel_dir = ""
        if rel_dir and rel_dir not in seen_dirs:
            write_newc_entry(f, rel_dir, b"", 0o040755)
            seen_dirs.add(rel_dir)
        for filename in filenames:
            fullpath = os.path.join(dirpath, filename)
            relpath = os.path.relpath(fullpath, rootdir)
            relpath = relpath.replace("\\", "/")
            parent = os.path.dirname(relpath)
            if parent and parent not in seen_dirs:
                write_newc_entry(f, parent, b"", 0o040755)
                seen_dirs.add(parent)
            with open(fullpath, "rb") as infile:
                data = infile.read()
            write_newc_entry(f, relpath, data, 0o100644)

with open("initrd.img", "wb") as f:
    add_dir(f, "../initrd")
    fields = [
        b'070701', b'00000000', b'00000000', b'00000000', b'00000000', b'00000001',
        b'00000000', b'00000000', b'00000000', b'00000000', b'00000000', b'00000000',
        b'0000000B', b'00000000'
    ]
    f.write(b''.join(fields))
    f.write(b'TRAILER!!!\0')
    f.write(b'\0' * (pad4(11+1) - (11+1)))
