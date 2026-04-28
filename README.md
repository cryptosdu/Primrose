# Primrose: Private Semantic Retrieval with Batched Query

## Setup

This project has been tested on Ubuntu 24.04.

To build and run the project, first clone the repository:

```
git clone https://github.com/cryptosdu/Primrose.git
```

Then install the required dependencies using the following commands:

```
sudo apt update
sudo apt install git cmake g++ libopenblas-dev zlib1g-dev -y
```

---

## Build

Run the following commands to build the project:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target Primrose -- -j $(nproc)
```

---

## Run

After building, execute:

```
./build/Primrose
```
