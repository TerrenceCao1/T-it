# T-it: A Minimal Git Recreation by TCow

### Pronounced: "Tee - it"

T-it is a lightweight, educational re-implementation of very underground, unknown, tiny version-control software Git. Fully written in C.

The goal of T-it is to *understand how Git works under the hood* by rebuilding core functionality from scratch.

## **<u>Features:</u>** 

| Command           | Status | Description                                       |
| ----------------- | ------ | ------------------------------------------------- |
| `tit init`        | ✅      | Create (or reinitialize) an empty T-it repository |
| `tit hash_object` | ✅      | Hash file contents into Git-style objects         |
| `tit add`         | ✅      | Add files to the staging area (index)             |
| `tit write_tree`  | ✅      | Create a tree object from the staging area        |
| `tit commit`      | ✅      | Create a commit object                            |
| `tit log`         | ✅      | Read and display commit history                   |

---

## **<u>Installation</u>**:

To install:

```bash
git clone https://github.com/TerrenceCao1/T-it.git
cd T-it
make # this will create the cli
```

## **<u>Usage Example</u>**:

### Initialize repository

```bash
./tit init # create the empty T-it repo
```

Creates a .tit folder with basic Git structure

### Add and Commit

```bash
./tit add file.txt
./tit commit -m "initial commit"
```

Stages, writes a tree, and commits changes 

### View History

```bash
./tit log
```

Prints past commits

## **<u>TODO</u>**:

- Recording video explaining how it works
- Possibly other features: status, checkout, tree

### Thank you guys so much for checking out!
