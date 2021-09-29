#include <stdint.h>
#include <string.h>
#include <string>
#include <benchmark/benchmark.h>
#include <immintrin.h>

__attribute__((__target__("avx2")))
int avx2_casecmp_eq_str_32(const char a[32], size_t a_len, const char b[32], size_t b_len) {
    static uint32_t len_mask_table[33] = {
            0x0000, 0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
            0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff, 0x1ffff,
            0x3ffff, 0x7ffff, 0xfffff, 0x1fffff, 0x3fffff, 0x7fffff, 0xffffff,
            0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff, 0x1fffffff, 0x3fffffff,
            0x7fffffff, 0xffffffff,
    };
    static uint8_t letter_uppercase_range_lower[32] = {
            0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
            0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40 };
    static uint8_t letter_uppercase_range_upper[32] = {
            0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a,
            0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a };
    static uint8_t letter_lowercase_bit[32] = {
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20 };

    assert(a_len <= sizeof(len_mask_table));
    assert(b_len <= sizeof(len_mask_table));

    if (a_len != b_len) {
        return false;
    }

    uint32_t eq_masq;

    // Load the strings
    __m256i a_block = _mm256_castpd_si256(_mm256_loadu_pd((double*)a));
    __m256i b_block = _mm256_castpd_si256(_mm256_loadu_pd((double*)b));
    __m256i letter_uppercase_range_lower_block = _mm256_castpd_si256(_mm256_loadu_pd((double*)&letter_uppercase_range_lower));
    __m256i letter_uppercase_range_upper_block = _mm256_castpd_si256(_mm256_loadu_pd((double*)&letter_uppercase_range_upper));
    __m256i letter_lowercase_bit_block = _mm256_castpd_si256(_mm256_loadu_pd((double*)&letter_lowercase_bit));

    // First identifies the upper case letters, because AVX2 doesn't provide a _mm256_cmplt_epi8 the _mm256_cmpgt_epi8
    // is being used in combination with _mm256_andnot_si256 to build 2 masks to identify which bytes are over A and
    // which ones are over Z and then convert the false of the second mask to true and identify the upper case letters.
    // Once the letters are identified, a bitmask for the lowercase bit is built for the single bytes and then the
    // bitmask of the bytes in the mask is applied to the initial string to covert it to lower case.
    // This process is repeated twice for the two strings.
    __m256i a_letters_uppercase_lower_mask = _mm256_cmpgt_epi8(a_block, letter_uppercase_range_lower_block);
    __m256i a_letters_uppercase_upper_mask = _mm256_cmpgt_epi8(a_block, letter_uppercase_range_upper_block);
    __m256i a_letters_uppercase_mask = _mm256_andnot_si256(a_letters_uppercase_upper_mask, a_letters_uppercase_lower_mask);
    __m256i a_letters_lowercase_bit_mask = _mm256_and_si256(a_letters_uppercase_mask, letter_lowercase_bit_block);
    __m256i a_lowercase = _mm256_or_si256(a_block, a_letters_lowercase_bit_mask);

    __m256i b_letters_uppercase_lower_mask = _mm256_cmpgt_epi8(b_block, letter_uppercase_range_lower_block);
    __m256i b_letters_uppercase_upper_mask = _mm256_cmpgt_epi8(b_block, letter_uppercase_range_upper_block);
    __m256i b_letters_uppercase_mask = _mm256_andnot_si256(b_letters_uppercase_upper_mask, b_letters_uppercase_lower_mask);
    __m256i b_letters_lowercase_bit_mask = _mm256_and_si256(b_letters_uppercase_mask, letter_lowercase_bit_block);
    __m256i b_lowercase = _mm256_or_si256(b_block, b_letters_lowercase_bit_mask);

    // This part of code matches the implementation of cmp_eq_32 but it's repeated as the code sharing wouldn't really
    // improve the readability.
    __m256i result_mask_vector = _mm256_cmpeq_epi8(a_lowercase, b_lowercase);
    eq_masq = (uint32_t)_mm256_movemask_epi8(result_mask_vector);

    return (eq_masq & len_mask_table[a_len]) == len_mask_table[a_len];
}

__attribute__((__target__("avx2")))
void BM_Hashtable_ShortStrings_Strncasecmp_Avx2(benchmark::State& state) {
    auto len = state.range(0);

    char data[192] = { 0 };

    char *a_string = data;
    char *b_string = data + 64 + 64;

    for(int i = 0; i < len; i++) {
        a_string[i] = '.';
    }
    for(int i = 0; i < len; i++) {
        b_string[i] = '.';
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(
                avx2_casecmp_eq_str_32((const char *)a_string, len, (const char *)b_string, len));
    }
}

__attribute__((__target__("avx2")))
void BM_Hashtable_ShortStrings_Strncasecmp_Glibc(benchmark::State& state) {
    auto len = state.range(0);

    char data[192] = { 0 };

    char *a_string = data;
    char *b_string = data + 64 + 64;

    for(int i = 0; i < len; i++) {
        a_string[i] = '.';
    }
    for(int i = 0; i < len; i++) {
        b_string[i] = '.';
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(
                strncasecmp((const char *)a_string, (const char *)b_string, 32));
    }
}

static void BenchArguments(benchmark::internal::Benchmark* b) {
    b->DenseRange(1,32);
    b->Iterations(100000);
}

BENCHMARK(BM_Hashtable_ShortStrings_Strncasecmp_Avx2)
    ->Apply(BenchArguments);
BENCHMARK(BM_Hashtable_ShortStrings_Strncasecmp_Glibc)
    ->Apply(BenchArguments);
