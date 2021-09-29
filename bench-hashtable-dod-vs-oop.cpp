#include <stdint.h>
#include <string.h>
#include <string>
#include <benchmark/benchmark.h>

#define HASHTABLE_SEARCH_MAX (16*32)
#define HASHTABLE_BUCKET_FLAGS_FILLED 0x01

typedef struct ht_bucket_dod ht_bucket_dod_t;
struct ht_bucket_dod {
    // filled represent just some metadata, fields are volatile to avoid that the compiler
    // optimize the fields away because knows the value in advance
    volatile bool filled;
    volatile uint16_t hash_quarter;
} __attribute__((aligned(4)));

typedef struct ht_bucket_oop ht_bucket_oop_t;
struct ht_bucket_oop {
    // filled represent just some metadata, fields are volatile to avoid that the compiler
    // optimize the fields away because knows the value in advance
    volatile bool filled;
    volatile uint16_t hash_quarter;
    char* key;
    uint16_t key_length;
    void* data;
};

typedef struct benchmark_params benchmark_params_t;
struct benchmark_params {
    uint32_t buckets_count;
    uint32_t iterations;
};
static benchmark_params_t benchmark_params = {
    .buckets_count = 600,
    .iterations = 1000000
};

template <typename T>
void BM_Hashtable_DodVsOop(benchmark::State& state) {
    uint32_t distance = state.range(0);
    uint64_t hash = distance;
    uint32_t buckets_count = benchmark_params.buckets_count;
    uint32_t iterations = benchmark_params.iterations;

    uint16_t hash_quarter = hash & 0xFFFFu;
    size_t ht_buckets_size = buckets_count * sizeof(T);

    auto ht_buckets = (T*)malloc(ht_buckets_size * iterations);
    memset(ht_buckets, 0, ht_buckets_size * iterations);

    for(uint64_t iteration = 0; iteration < iterations; iteration++) {
        uint64_t iteration_start_index = iteration * buckets_count;
        for(
                uint32_t index = iteration_start_index;
                index < iteration_start_index + buckets_count;
                index++) {
            ht_buckets[index].filled = true;
            ht_buckets[index].hash_quarter = (uint16_t)((index - iteration_start_index) & 0xFFFFu);
        }
    }

    uint64_t iteration = 0;
    for (auto _ : state) {
        bool found;
        uint64_t iteration_start_index = iteration * buckets_count;

        for(
                uint32_t index = iteration_start_index;
                index < iteration_start_index + HASHTABLE_SEARCH_MAX;
                index++) {
            if (!ht_buckets[index].filled) {
                continue;
            }

            benchmark::DoNotOptimize((found = ht_buckets[index].hash_quarter == hash_quarter));
            if (found) {
                break;
            }
        }

#ifdef DEBUG
        if (!found) {
            throw std::runtime_error("Unable to find requested hash, iteration " + std::to_string(iteration) + ", hash " + std::to_string(hash));
        }
#endif

        iteration++;
    }

    free(ht_buckets);
}

static void BenchArguments(benchmark::internal::Benchmark* b) {
    b->Arg(1);
    b->Arg(5);
    b->Arg(10);
    b->Arg(25);
    b->Arg(50);
    b->Arg(100);
    b->Arg(200);
    b->Arg(300);
    b->Arg(400);
    b->Arg(500);
    b->Iterations(1000000);
}

BENCHMARK_TEMPLATE(BM_Hashtable_DodVsOop, ht_bucket_dod_t)
    ->Apply(BenchArguments);
BENCHMARK_TEMPLATE(BM_Hashtable_DodVsOop, ht_bucket_oop_t)
    ->Apply(BenchArguments);
