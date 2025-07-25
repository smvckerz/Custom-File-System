
# Custom File System (CSC415)

> **Eduardo Muñoz – Summer 2025, SFSU**
>
> A pedagogical file system written in C for the *Operating Systems* course (CSC 415). The project demonstrates low‑level disk layout planning, free‑space management with a bitmap, and POSIX‑style directory operations.

---

## ✨ Features
| Area | Details |
|------|---------|
| **Volume Control Block** | Stores magic number, block counts, bitmap offset, root dir location |
| **Bitmap Allocator** | First‑fit scan for `O(1)` allocate/free; 1 bit represents 1 block |
| **Directory API** | Implements `fs_setcwd`, `fs_getcwd`, `fs_mkdir`, `fs_opendir`, `fs_readdir` |
| **Root Directory Init** | Creates `.` and `..` entries during *format* step |
| **Error Handling** | Detects corrupted VCB, out‑of‑space, and illegal paths |
| **Hexdump Debugger** | Script in `tools/` verifies raw on‑disk structures (VCB‑>bitmap‑>dir) |

---

## 🗂️ Repository Layout
```
custom-fs/
├── src/               # C source files
│   ├── fs.c           # Public API wrappers
│   ├── bitmap.c       # Free‑space bitmap logic (MY CODE)
│   ├── dir.c          # Directory traversal + mkdir (MY CODE)
│   ├── vcb.c          # VCB init / validate
│   └── ...
├── include/          # Header files
├── tests/            # Allocation & traversal unit tests (Criterion)
├── tools/            # hexdump.sh and helper scripts
└── Makefile          # Stand‑alone build script
```
*(“MY CODE” marks modules I wrote from scratch – the rest was provided as scaffolding by the instructor.)*

---

## 🛠️ Build
**Prerequisites:** GCC ≥ 11, `make`, POSIX‑compatible OS (Linux, macOS, WSL).

```bash
# clone
git clone https://github.com/<your-handle>/custom-fs.git
cd custom-fs

# build library + test shell
make               # produces libfs.a and fs_shell
```

### Running the Mini Shell
`fs_shell` exposes simple commands (`format`, `mkdir`, `ls`, `cd`, `stat`).
```bash
./fs_shell
fs> format myvol 10MB   # create 10 MB virtual volume
fs> mkdir /projects
fs> cd /projects
fs> mkdir logs
fs> ls -l
.
..
logs/
```

---

## ✅ Testing
```bash
make test           # requires Criterion — see tests/README.md
```
Unit tests cover:
* Bitmap allocate/free edge cases
* Deep directory traversal & path normalization
* VCB integrity (magic number, offsets)

---

## 📝 Design Notes
* **Block size:** 4 KB (configurable via `fs_config.h`)
* **Filesystem limits:** max 1 GB volume, max 65,535 files/dirs (16‑bit inode id)
* **Endianess:** little‑endian on‑disk structures (assumes x86/ARM‑LE)

See `docs/design.md` for rationale, diagrams, and future ideas (extents, journaling).

---

## 🚧 Roadmap
- [ ] Add file read/write support
- [ ] Implement FAT‑style free‑space alternative for comparison
- [ ] Performance benchmark versus ext2 on tmpfs

---

## 📄 License
MIT – see `LICENSE` file.

---

## 🙋‍♂️ Author
**Eduardo Muñoz**  •  [LinkedIn](https://www.linkedin.com/in/eduardo-munoz-93b09523a)  •  [GitHub](https://github.com/smvckerz)
