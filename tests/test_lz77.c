#include "../include/lz77.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TEST_SIZE       (1024 * 1024)
#define FUZZ_ITERATIONS 1000
#define MAX_FUZZ_SIZE   65536

static void run_case(const char *name, byte *data, uint32_t size);

void test_lz77_token_stream(void) {
    printf("\n=== LZ77 TEST SUITE ===\n\n");

    /* 
     * 1. Single byte
     */
    {
        byte data[] = {'A'};
        run_case("single byte", data, sizeof(data));
    }

    /* 
     * 2. Two bytes
     */
    {
        byte data[] = {'A', 'B'};
        run_case("two bytes", data, sizeof(data));
    }

    /*
     * 3. Exactly MIN_MATCH_LENGTH-1
     */
    {
        byte data[] = "AA";
        run_case("min-1", data, sizeof(data) - 1);
    }

    /*
     * 4. Exactly MIN_MATCH_LENGTH
     */
    {
        byte data[] = "AAA";
        run_case("exact min", data, sizeof(data) - 1);
    }

    /*
     * 5. First possible match
     */
    {
        byte data[] = "AAAA";
        run_case("simple match", data, sizeof(data) - 1);
    }

    /*
     * 6. Long overlap
     */
    {
        byte data[] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
        run_case("overlap", data, sizeof(data) - 1);
    }

    /*
     * 7. Alternating
     */
    {
        byte data[] = "ABABABABABABABABABABABABABABABABABAB";
        run_case("alternating", data, sizeof(data) - 1);
    }

    /*
     * 8. Alphabet repeat
     */
    {
        byte data[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        run_case("alphabet repeat", data, sizeof(data) - 1);
    }

    /*
     * 9. Binary values
     */
    {
        byte data[256];

        for (int i = 0; i < 256; i++)
            data[i] = i;

        run_case("binary", data, sizeof(data));
    }

    /*
     * 10. All zeros
     */
    {
        byte *data = malloc(TEST_SIZE);

        memset(data, 0, TEST_SIZE);

        run_case("zeros", data, TEST_SIZE);

        free(data);
    }

    /*
     * 11. All FF
     */
    {
        byte *data = malloc(TEST_SIZE);

        memset(data, 0xFF, TEST_SIZE);

        run_case("ff", data, TEST_SIZE);

        free(data);
    }

    /*
     * 12. Maximum match
     */
    {
        byte *data = malloc(MAX_MATCH_LENGTH + 100);

        memset(data, 'A', MAX_MATCH_LENGTH + 100);

        run_case("max match", data, MAX_MATCH_LENGTH + 100);

        free(data);
    }

    /*
     * 13. Window boundary
     */
    {
        byte *data = malloc(SEARCH_BUFFER_SIZE + 300);

        memset(data, 'X', SEARCH_BUFFER_SIZE);
        memset(data + SEARCH_BUFFER_SIZE, 'X', 300);

        run_case("window boundary", data, SEARCH_BUFFER_SIZE + 300);

        free(data);
    }

    /*
     * 14. Exact search window
     */
    {
        byte *data = malloc(SEARCH_BUFFER_SIZE);

        memset(data, 'A', SEARCH_BUFFER_SIZE);

        run_case("exact window", data, SEARCH_BUFFER_SIZE);

        free(data);
    }

    /* 
     * 15. Search window + 1
     */
    {
        byte *data = malloc(SEARCH_BUFFER_SIZE + 1);

        memset(data, 'A', SEARCH_BUFFER_SIZE + 1);

        run_case("window+1", data, SEARCH_BUFFER_SIZE + 1);

        free(data);
    }

    /*
     * 16. Random
     */
    {
        byte *data = malloc(TEST_SIZE);

        srand(12345);

        for (size_t i = 0; i < TEST_SIZE; i++)
            data[i] = rand() & 0xFF;

        run_case("random", data, TEST_SIZE);

        free(data);
    }

    /*
     * 17. Mixed realistic
     */
    {
        byte *data = malloc(TEST_SIZE);

        for (size_t i = 0; i < TEST_SIZE; i++) {
            switch ((i / 4096) % 4) {
            case 0:
                data[i] = 0;
                break;

            case 1:
                data[i] = i & 0xff;
                break;

            case 2:
                data[i] = (i / 8) & 0xff;
                break;

            default:
                data[i] = 'A' + (i % 26);
            }
        }

        run_case("mixed realistic", data, TEST_SIZE);

        free(data);
    }

    /*
     * 18. PNG-like scanlines
     */
    {
        const int width = 512;

        byte *data = malloc(width * 2048);

        for (int row = 0; row < 2048; row++) {
            data[row * width] = 0;

            for (int col = 1; col < width; col++)
                data[row * width + col] = (byte)(col & 0xff);
        }

        run_case("png scanlines", data, width * 2048);

        free(data);
    }

    /*
     * 19. Worst-case hash collisions
     */
    {
        byte *data = malloc(TEST_SIZE);

        for (size_t i = 0; i < TEST_SIZE; i++)
            data[i] = (i & 1) ? 'A' : 'B';

        run_case("hash collision", data, TEST_SIZE);

        free(data);
    }

    /*
     * 20. Fuzz
     */
    {
        srand(1);

        for (int t = 0; t < 1000; t++) {
            uint32_t size = (rand() % 4096) + 1;

            byte *data = malloc(size);

            for (uint32_t i = 0; i < size; i++)
                data[i] = rand() & 0xff;

            run_case("fuzz", data, size);

            free(data);
        }
    }

    printf("\n=== ALL LZ77 TESTS PASSED ===\n");
}

static void run_case(const char *name, byte *data, uint32_t size) {
    printf("[LZ77] %s...\n", name);

    lz_tokens_stream_t *stream = lz_77_encode(data, size);
    printf("[LZ77] encoded\n");

    assert(stream != NULL);
    assert(stream->size >= 2);

    /* EOB must be last */
    assert(stream->tokens[stream->size - 1].type == EOB_TOKEN);

    /* verify every token is valid */
    for (uint32_t i = 0; i < stream->size; i++) {
        switch (stream->tokens[i].type) {
        case LITERAL:
            break;

        case MATCH:
            assert(stream->tokens[i].length >= MIN_MATCH_LENGTH);
            assert(stream->tokens[i].length <= MAX_MATCH_LENGTH);
            assert(stream->tokens[i].distance >= 1);
            assert(stream->tokens[i].distance <= SEARCH_BUFFER_SIZE);
            break;

        case EOB_TOKEN:
            assert(i == stream->size - 1);
            break;

        default:
            assert(0);
        }
    }

    /* decode and verify roundtrip */
    byte *decoded = lz_77_decode(stream);
    printf("[LZ77] decoded\n");

    assert(decoded != NULL);
    assert(memcmp(data, decoded, size) == 0);

    free(decoded);
    free_lz77(stream);

    printf("  PASS\n");
}
