# Performance Summit - September 2021

This repository contains most of the source code used for the benchmarks presented during the talk
`cachegrand - pratical examples of successful optimizations` at the `Performance Summit - September 2021`, the code is
under the BSD 3-Clause license so feel free to reuse the code as you see fit.

[cachegrand](https://github.com/danielealbano/cachegrand) is an open-source fast, scalable and secure Key-Value store
able to act as Redis drop-in replacement designed from the ground up to take advantage of modern  hardware vertical
scalability, able to provide better performance and a larger cache at lower cost, without losing focus on distributed
systems.

### Introduction

This repository contains 4 categories of benchmarks
- Context Switching
- DoD (Data Oriented Development) vs OOP (Object Oriented Programming) data structures & algorithms
- SIMD optimized linear search
- Short strings optimizations

The benchmarks in the presentation have been run on the following hardware:
- 2 x Intel Xeon E5-2690 v4 2.60Ghz
- 8 x 16GB (128GB) ECC DDR4 2100Mhz
- 1 x Intel XL710-QDA1 (1 x 40Gbit QSFP+ link)
- 2 x 480GB Corsair SSD
- Ubuntu server 20.04 running with the kernel 5.13.14

If you decide to run the benchmarks to compare the numbers please keep in mind that:
- software running on the test machine can affect the benchmark
- the governor in use will affect the benchmark (should be set to performance)
- the kernel in use will affect the benchmark
- if you are running a desktop version of the OS the benchmark will definitely be heavily affected by all the software 
  running
- numbers always change a bit, please look at the relative difference and not to the absolute numbers

### HOW TO

#### Requirements

You will need to have installed cmake, git and the gcc compiler with the standard build tools, if you are on ubuntu you
can run the following commands to install the required dependencies
```bash
sudo apt-get install cmake build-essential git
```

The cmake build files will take care of downloading and building any additional dependency (i.e. the Google Benchmark
library).

#### Checkout

```bash
git clone https://github.com/danielealbano/performance-summit-202109-benchmarks.git
cd performance-summit-202109-benchmarks
```

#### Build

```bash
mkdir cmake-build-release
cd cmake-build-release
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

#### Run

From the build folder (e.g. `cmake-build-release`) run the following command
```bash
./performance_summit_202109_benchmarks
```

It's possible to produce the data in the CSV format and save them onto a file running the following command from the
build folder (e.g. `cmake-build-release`)
```bash
./performance_summit_202109_benchmarks --benchmark_format=console --benchmark_out=benchmarks.csv --benchmark_out_format=csv
```

Here an example of the output
```text
2021-09-28T19:40:01+01:00
Running ./performance_summit_202109_benchmarks
Run on (12 X 4299.83 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 12288 KiB (x1)
Load Average: 0.71, 0.92, 1.16
CPU Core Count: 12
CPU Frequency: 3200
CPU Name: Intel(R) Core(TM) i7-8700 CPU @ 3.20GHz
NUMA Node Count: 1
-----------------------------------------------------------------------------------------------------------
Benchmark                                                                 Time             CPU   Iterations
-----------------------------------------------------------------------------------------------------------
BM_ContextSwitching_Reference/iterations:1000000                        868 ns          868 ns      1000000
BM_ContextSwitching_OsOverheadUnpinned/iterations:1000000              5630 ns         4010 ns      1000000
BM_ContextSwitching_OsOverheadPinned/iterations:1000000                3766 ns         1881 ns      1000000
BM_ContextSwitching_Fiber2XPinnedOverhead/iterations:1000000           20.8 ns         20.8 ns      1000000
BM_Hashtable_DodVsOop<ht_bucket_dod_t>/1/iterations:1000000            17.1 ns         17.1 ns      1000000
BM_Hashtable_DodVsOop<ht_bucket_dod_t>/5/iterations:1000000            27.7 ns         27.7 ns      1000000
BM_Hashtable_DodVsOop<ht_bucket_dod_t>/10/iterations:1000000           39.7 ns         39.7 ns      1000000
BM_Hashtable_DodVsOop<ht_bucket_dod_t>/25/iterations:1000000           81.3 ns         81.3 ns      1000000
BM_Hashtable_DodVsOop<ht_bucket_dod_t>/50/iterations:1000000           99.3 ns         99.3 ns      1000000
BM_Hashtable_DodVsOop<ht_bucket_dod_t>/100/iterations:1000000           145 ns          145 ns      1000000
BM_Hashtable_DodVsOop<ht_bucket_dod_t>/200/iterations:1000000           204 ns          204 ns      1000000
BM_Hashtable_DodVsOop<ht_bucket_dod_t>/300/iterations:1000000           250 ns          250 ns      1000000
BM_Hashtable_DodVsOop<ht_bucket_dod_t>/400/iterations:1000000           299 ns          299 ns      1000000
BM_Hashtable_DodVsOop<ht_bucket_dod_t>/500/iterations:1000000           362 ns          362 ns      1000000
BM_Hashtable_DodVsOop<ht_bucket_oop_t>/1/iterations:1000000             221 ns          215 ns      1000000
BM_Hashtable_DodVsOop<ht_bucket_oop_t>/5/iterations:1000000            57.1 ns         57.1 ns      1000000
...
```

### Contribute

If you find bugs or do you want to improve the code used feel free to open to submit a PR.