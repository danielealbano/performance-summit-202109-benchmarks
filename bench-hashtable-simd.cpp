#include <stdint.h>
#include <string.h>
#include <string>
#include <benchmark/benchmark.h>
#include <immintrin.h>

#define HASHTABLE_SEARCH_MAX (16*32)

#define HASHTABLE_BUCKET_FLAGS_FILLED 0x01

typedef union ht_bucket ht_bucket_t;
union ht_bucket {
    uint32_t hash;
    struct {
        // filled represent just one of the useful metadata per bucket
        bool filled;
        uint16_t hash_quarter;
    } data __attribute__((aligned(4)));
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
void BM_Hashtable_Simd_without(benchmark::State& state) {
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
            ht_buckets[index].data.filled = true;
            ht_buckets[index].data.hash_quarter = (uint16_t)((index - iteration_start_index) & 0xFFFFu);
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
            if (!ht_buckets[index].data.filled) {
                continue;
            }

            benchmark::DoNotOptimize((found = ht_buckets[index].data.hash_quarter == hash_quarter));
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

#define HASHTABLE_MCMP_SUPPORT_HASH_SEARCH_NOT_FOUND     32u

__attribute__((__target__("avx2")))
uint32_t hashtable_linear_search_avx2_16(
        uint32_t half_hash,
        uint32_t* half_hashes,
        uint32_t skip_indexes_mask) {
    uint32_t compacted_result_mask = 0;
    uint32_t skip_indexes_mask_inv = ~skip_indexes_mask;
    __m256i cmp_vector = _mm256_set1_epi32(half_hash);

    for(uint8_t base_index = 0; base_index < 16; base_index += 8) {
        __m256i ring_vector = _mm256_loadu_si256((__m256i*) (half_hashes + base_index));
        __m256i result_mask_vector = _mm256_cmpeq_epi32(ring_vector, cmp_vector);

        // Uses _mm256_movemask_ps to reduce the bandwidth
        compacted_result_mask |= (uint32_t)_mm256_movemask_ps(_mm256_castsi256_ps(result_mask_vector)) << (base_index);
    }

    return _tzcnt_u32(compacted_result_mask & skip_indexes_mask_inv);
}

template <typename T>
void BM_Hashtable_Simd_with(benchmark::State& state) {
    uint32_t skip_indexes_mask;
    uint32_t distance = state.range(0);
    uint64_t hash = distance;
    uint32_t buckets_count = benchmark_params.buckets_count;
    uint32_t iterations = benchmark_params.iterations;

    buckets_count += (buckets_count % 16) + 16;

    uint16_t hash_quarter = hash & 0xFFFFu;
    size_t ht_hashes_size = buckets_count * sizeof(T);

    auto ht_hashes = (T*)malloc(ht_hashes_size * iterations);
    memset(ht_hashes, 0, ht_hashes_size * iterations);

    for(uint64_t iteration = 0; iteration < iterations; iteration++) {
        uint64_t iteration_start_index = iteration * buckets_count;
        for(
                uint32_t index = iteration_start_index;
                index < iteration_start_index + buckets_count;
                index++) {
            ht_hashes[index].data.filled = true;
            ht_hashes[index].data.hash_quarter = (uint16_t)((index - iteration_start_index) & 0xFFFFu);
        }
    }

    uint64_t iteration = 0;
    uint32_t chunk_start_index = 0;
    for (auto _ : state) {
        bool found = false;
        uint64_t iteration_start_index = iteration * buckets_count;
        ht_bucket_t bucket_search = { 0 };
        bucket_search.data.filled = true;
        bucket_search.data.hash_quarter = hash_quarter;

        for(
                uint64_t chunk_index = iteration_start_index + chunk_start_index;
                chunk_index <= iteration_start_index + chunk_start_index + HASHTABLE_SEARCH_MAX && !found;
                chunk_index += 16) {
            ht_bucket_t* ht_bucket_search_start = &ht_hashes[chunk_index];

            // The mask is used in case of collisions, the while loop below updates the mask to exclude
            // the colliding value and search the one after
            skip_indexes_mask = 0;

            while (true) {
                uint32_t chunk_slot_index = hashtable_linear_search_avx2_16(
                        bucket_search.hash,
                        (uint32_t*)ht_bucket_search_start,
                        skip_indexes_mask);

                if (chunk_slot_index == HASHTABLE_MCMP_SUPPORT_HASH_SEARCH_NOT_FOUND) {
                    break;
                }

                benchmark::DoNotOptimize(found = true);
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

    free(ht_hashes);
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

BENCHMARK_TEMPLATE(BM_Hashtable_Simd_without, ht_bucket_t)
    ->Apply(BenchArguments);
BENCHMARK_TEMPLATE(BM_Hashtable_Simd_with, ht_bucket_t)
    ->Apply(BenchArguments);
