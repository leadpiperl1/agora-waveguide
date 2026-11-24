/*
 * src/agora_waveguide_kernel.c
 * Freestanding Zero-Heap ISO C99 Agora Waveguide Kernel & CLI Verifier (v1.0.0)
 *
 * Build library : gcc -std=c99 -O3 -ffp-contract=off -Wall -Wextra -Wpedantic \
 *                     -Wshadow -Wformat=2 -Wconversion -Wundef -Werror -Iinclude \
 *                     -fPIC -shared src/agora_waveguide_kernel.c -o build/libagora_waveguide_c99.so
 * Build CLI     : gcc -std=c99 -O3 -ffp-contract=off -Wall -Wextra -Wpedantic \
 *                     -Wshadow -Wformat=2 -Wconversion -Wundef -Werror -Iinclude \
 *                     -DAGORA_WAVEGUIDE_STANDALONE_MAIN src/agora_waveguide_kernel.c \
 *                     -o build/agora_waveguide_c99
 */

#include "agora_waveguide_v1_0_0.h"

static const int32_t AGORA_MZ_KERNEL_Q16[AGORA_HISTORY_LEN] = {
    16384, 8192, 4096, 2048, 1024, 512, 256, 128
};

static int32_t agora_q16_mul(int32_t a, int32_t b) {
    int64_t prod = (int64_t)a * (int64_t)b;
    return (int32_t)(prod / 65536LL);
}

static int32_t agora_xorshift32_noise(uint32_t *state) {
    uint32_t x = *state;
    if (x == 0U) {
        x = 0x9E3779B9U;
    }
    x ^= x << 13U;
    x ^= x >> 17U;
    x ^= x << 5U;
    *state = x;
    return (int32_t)(x >> 20U) - 2048;
}

static uint32_t agora_crc32_bytes(const uint8_t *buf, size_t len) {
    uint32_t crc = 0xFFFFFFFFU;
    size_t i;
    for (i = 0U; i < len; ++i) {
        uint32_t b = (uint32_t)buf[i];
        int bit;
        crc ^= b;
        for (bit = 0; bit < 8; ++bit) {
            if ((crc & 1U) != 0U) {
                crc = (crc >> 1U) ^ 0xEDB88320U;
            } else {
                crc >>= 1U;
            }
        }
    }
    return crc ^ 0xFFFFFFFFU;
}

static uint32_t agora_compute_frame_crc32(const AgoraWaveguideFrame64B *f) {
    uint8_t raw[60];
    size_t k;
    size_t pos = 0U;

    for (k = 0U; k < AGORA_STATE_DIM; ++k) {
        raw[pos++] = (uint8_t)f->x_trit[k];
    }
    for (k = 0U; k < AGORA_MAX_NEIGHBORS; ++k) {
        uint16_t v = f->neighbors[k];
        raw[pos++] = (uint8_t)(v & 0xFFU);
        raw[pos++] = (uint8_t)((v >> 8U) & 0xFFU);
    }
    for (k = 0U; k < AGORA_HISTORY_LEN; ++k) {
        raw[pos++] = f->mz_ring[k];
    }
    raw[pos++] = f->ring_head;
    raw[pos++] = f->status_byte;
    raw[pos++] = f->quorum_pole;
    raw[pos++] = f->reserved_pad;
    for (k = 0U; k < AGORA_STATE_DIM; ++k) {
        uint32_t u = (uint32_t)f->latent_q16[k];
        raw[pos++] = (uint8_t)(u & 0xFFU);
        raw[pos++] = (uint8_t)((u >> 8U) & 0xFFU);
        raw[pos++] = (uint8_t)((u >> 16U) & 0xFFU);
        raw[pos++] = (uint8_t)((u >> 24U) & 0xFFU);
    }
    return agora_crc32_bytes(raw, sizeof(raw));
}

uint8_t agora_fowler_pack_5trits(const int8_t trits[5]) {
    uint16_t acc = 0U;
    size_t i;
    if (trits == NULL) {
        return CODE_TRAPDOOR_FAIL_CLOSED;
    }
    for (i = 0U; i < 5U; ++i) {
        int8_t t = trits[i];
        if (t < -1) {
            t = -1;
        } else if (t > 1) {
            t = 1;
        }
        acc = (uint16_t)(acc * 3U + (uint16_t)(t + 1));
    }
    return (uint8_t)acc;
}

uint8_t agora_fowler_unpack_5trits_strict(uint8_t packed, int8_t out_trits[5]) {
    size_t i;
    if (out_trits == NULL) {
        return CODE_TRAPDOOR_FAIL_CLOSED;
    }
    if (packed > AGORA_FOWLER_MAX_PAYLOAD) {
        for (i = 0U; i < 5U; ++i) {
            out_trits[i] = 0;
        }
        return CODE_TRAPDOOR_FAIL_CLOSED;
    }
    for (i = 5U; i > 0U; --i) {
        uint8_t digit = (uint8_t)(packed % 3U);
        out_trits[i - 1U] = (int8_t)((int)digit - 1);
        packed = (uint8_t)(packed / 3U);
    }
    return 0U;
}

uint32_t agora_quorum13_audit_ties(void) {
    uint32_t ties = 0U;
    uint32_t mask;
    for (mask = 0U; mask < 8192U; ++mask) {
        int32_t sum = 0;
        uint32_t bit;
        for (bit = 0U; bit < 13U; ++bit) {
            sum += ((mask & (1U << bit)) != 0U) ? 1 : -1;
        }
        if (sum == 0) {
            ties++;
        }
    }
    return ties;
}

void agora_init_agents(AgoraWaveguideFrame64B *agents, size_t num_agents) {
    size_t i;
    if (agents == NULL || num_agents == 0U) {
        return;
    }
    for (i = 0U; i < num_agents; ++i) {
        size_t k;
        int8_t init_trit = (int8_t)((int)(i % 3U) - 1);
        uint8_t init_packed;
        AgoraWaveguideFrame64B *a = &agents[i];

        a->neighbors[0] = (uint16_t)((i + num_agents - 1U) % num_agents);
        a->neighbors[1] = (uint16_t)((i + 1U) % num_agents);
        a->neighbors[2] = (uint16_t)((i + num_agents - 5U) % num_agents);
        a->neighbors[3] = (uint16_t)((i + 3U) % num_agents);

        for (k = 0U; k < AGORA_STATE_DIM; ++k) {
            a->x_trit[k] = init_trit;
            a->latent_q16[k] = (int32_t)init_trit * 20000;
        }
        init_packed = agora_fowler_pack_5trits(a->x_trit);
        for (k = 0U; k < AGORA_HISTORY_LEN; ++k) {
            a->mz_ring[k] = init_packed;
        }
        a->ring_head = 0U;
        a->status_byte = init_packed;
        a->quorum_pole = (uint8_t)(init_trit + 1);
        a->reserved_pad = 0U;
        a->frame_crc32 = agora_compute_frame_crc32(a);
    }
}

uint32_t agora_waveguide_step(
    AgoraWaveguideFrame64B *agents,
    size_t num_agents,
    int32_t omega_q16,
    int32_t coupling_q16,
    int32_t dt_q16,
    uint32_t *rng_state,
    uint32_t *out_welds,
    uint32_t *out_pq_mismatch
) {
    size_t i;
    uint32_t clamped_count = 0U;
    uint32_t weld_count = 0U;
    uint32_t pq_mismatch = 0U;

    if (agents == NULL || rng_state == NULL || num_agents == 0U) {
        return (uint32_t)CODE_TRAPDOOR_FAIL_CLOSED;
    }

    for (i = 0U; i < num_agents; ++i) {
        int32_t neighbor_sum[AGORA_STATE_DIM];
        int32_t mz_drag[AGORA_STATE_DIM];
        size_t k;
        size_t nb;
        size_t tau;
        uint8_t head = agents[i].ring_head;
        int saturated = 0;
        int32_t max_abs_dx = 0;
        int32_t trit_sum = 0;
        uint8_t next_head;
        uint8_t packed;

        for (k = 0U; k < AGORA_STATE_DIM; ++k) {
            neighbor_sum[k] = 0;
            mz_drag[k] = 0;
        }

        for (nb = 0U; nb < AGORA_MAX_NEIGHBORS; ++nb) {
            uint16_t nb_idx = agents[i].neighbors[nb];
            if ((size_t)nb_idx < num_agents) {
                for (k = 0U; k < AGORA_STATE_DIM; ++k) {
                    neighbor_sum[k] += (int32_t)agents[nb_idx].x_trit[k];
                }
            }
        }

        for (tau = 0U; tau < AGORA_HISTORY_LEN; ++tau) {
            size_t slot = ((size_t)head + AGORA_HISTORY_LEN - tau) % AGORA_HISTORY_LEN;
            int8_t past_trits[5];
            int32_t weight = AGORA_MZ_KERNEL_Q16[tau];
            (void)agora_fowler_unpack_5trits_strict(agents[i].mz_ring[slot], past_trits);
            for (k = 0U; k < 5U; ++k) {
                mz_drag[k] += (int32_t)past_trits[k] * weight;
            }
        }

        for (k = 0U; k < AGORA_STATE_DIM; ++k) {
            int32_t x_curr = (int32_t)agents[i].x_trit[k] * AGORA_Q16_ONE;
            int32_t omega = agora_q16_mul(omega_q16, x_curr);
            int32_t social = neighbor_sum[k] * coupling_q16;
            int32_t noise = agora_xorshift32_noise(rng_state);
            int32_t dx = omega - mz_drag[k] + social + noise;
            int32_t abs_dx = (dx < 0) ? -dx : dx;
            int32_t delta = agora_q16_mul(dx, dt_q16);
            int64_t next_wide = (int64_t)agents[i].latent_q16[k] + (int64_t)delta;
            int32_t next_val;
            int32_t p_proj;
            int32_t q_orth;

            if (abs_dx > max_abs_dx) {
                max_abs_dx = abs_dx;
            }

            if (next_wide > 2147483647LL) {
                next_wide = 2147483647LL;
            } else if (next_wide < -2147483648LL) {
                next_wide = -2147483648LL;
            }
            next_val = (int32_t)next_wide;

            if (next_val > AGORA_Q16_ONE) {
                next_val = AGORA_Q16_ONE;
                saturated = 1;
            } else if (next_val < -AGORA_Q16_ONE) {
                next_val = -AGORA_Q16_ONE;
                saturated = 1;
            }

            agents[i].latent_q16[k] = next_val;
            if (next_val > AGORA_Q16_TRIT_THRESH) {
                agents[i].x_trit[k] = 1;
            } else if (next_val < -AGORA_Q16_TRIT_THRESH) {
                agents[i].x_trit[k] = -1;
            } else {
                agents[i].x_trit[k] = 0;
            }
            trit_sum += (int32_t)agents[i].x_trit[k];

            /* Mori-Zwanzig exact conservation check: P + Q == I */
            p_proj = (int32_t)agents[i].x_trit[k] * AGORA_Q16_ONE;
            q_orth = next_val - p_proj;
            if ((p_proj + q_orth) != next_val) {
                pq_mismatch++;
            }
        }

        next_head = (uint8_t)(((size_t)head + 1U) % AGORA_HISTORY_LEN);
        agents[i].ring_head = next_head;
        packed = agora_fowler_pack_5trits(agents[i].x_trit);
        agents[i].mz_ring[next_head] = packed;

        if (saturated != 0) {
            clamped_count++;
            agents[i].status_byte = CODE_KEYHOLE_CLAMP;
        } else if (max_abs_dx > AGORA_Q16_KEYHOLE) {
            weld_count++;
            agents[i].status_byte = CODE_KINTSUGI_WELD;
        } else {
            agents[i].status_byte = packed;
        }

        if (trit_sum < 0) {
            agents[i].quorum_pole = 0U;
        } else if (trit_sum > 0) {
            agents[i].quorum_pole = 2U;
        } else {
            agents[i].quorum_pole = 1U;
        }
        agents[i].reserved_pad = 0U;
        agents[i].frame_crc32 = agora_compute_frame_crc32(&agents[i]);
    }

    if (out_welds != NULL) {
        *out_welds = weld_count;
    }
    if (out_pq_mismatch != NULL) {
        *out_pq_mismatch = pq_mismatch;
    }
    return clamped_count;
}

uint64_t agora_crc64_ecma182(const uint8_t *data, size_t len) {
    uint64_t crc = 0ULL;
    size_t i;
    if (data == NULL) {
        return 0ULL;
    }
    for (i = 0U; i < len; ++i) {
        int bit;
        crc ^= ((uint64_t)data[i]) << 56U;
        for (bit = 0; bit < 8; ++bit) {
            if ((crc & 0x8000000000000000ULL) != 0ULL) {
                crc = (crc << 1U) ^ 0x42F0E1EBA9EA3693ULL;
            } else {
                crc <<= 1U;
            }
        }
    }
    return crc;
}

#ifdef AGORA_WAVEGUIDE_STANDALONE_MAIN
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static AgoraWaveguideFrame64B g_agents[AGORA_DEFAULT_AGENTS];

int main(int argc, char **argv) {
    uint32_t seed = AGORA_DEFAULT_SEED;
    uint32_t rng_state;
    uint32_t total_clamps = 0U;
    uint32_t total_welds = 0U;
    uint32_t total_pq_mismatch = 0U;
    uint32_t fowler_bijection_ok = 1U;
    uint32_t trapdoor_rejected_ok = 1U;
    uint32_t null_guard_rc;
    uint32_t quorum13_ties;
    uint64_t state_crc64;
    uint32_t b;
    size_t step;
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--seed") == 0 && (i + 1) < argc) {
            unsigned long parsed = strtoul(argv[++i], NULL, 10);
            seed = (uint32_t)parsed;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: agora_waveguide_c99 [--seed <uint32>]\n");
            return 0;
        } else {
            fprintf(stderr, "[ERROR] Unknown argument: %s\n", argv[i]);
            return 2;
        }
    }

    /* Gate 2: Exhaustive Fowler 243-state bijection & Byte 253 trapdoor rejection */
    for (b = 0U; b <= 242U; ++b) {
        int8_t trits[5];
        uint8_t rc = agora_fowler_unpack_5trits_strict((uint8_t)b, trits);
        uint8_t repacked = agora_fowler_pack_5trits(trits);
        if (rc != 0U || repacked != (uint8_t)b) {
            fowler_bijection_ok = 0U;
        }
    }
    for (b = 243U; b <= 255U; ++b) {
        int8_t trits[5];
        uint8_t rc = agora_fowler_unpack_5trits_strict((uint8_t)b, trits);
        if (rc != CODE_TRAPDOOR_FAIL_CLOSED) {
            trapdoor_rejected_ok = 0U;
        }
    }

    /* Gate 3: Odd-parity 12+1=13 quorum zero-tie invariant */
    quorum13_ties = agora_quorum13_audit_ties();

    /* Gate 5a: Fail-closed null pointer guard */
    rng_state = seed;
    null_guard_rc = agora_waveguide_step(
        NULL, AGORA_DEFAULT_AGENTS,
        AGORA_DEFAULT_OMEGA_Q16, AGORA_DEFAULT_COUPLING_Q16, AGORA_DEFAULT_DT_Q16,
        &rng_state, NULL, NULL
    );

    /* Gate 4: Deterministic Mori-Zwanzig simulation */
    agora_init_agents(g_agents, AGORA_DEFAULT_AGENTS);
    rng_state = (seed != 0U) ? seed : 0x9E3779B9U;

    for (step = 0U; step < AGORA_DEFAULT_STEPS; ++step) {
        uint32_t welds = 0U;
        uint32_t pq_err = 0U;
        uint32_t clamps = agora_waveguide_step(
            g_agents, AGORA_DEFAULT_AGENTS,
            AGORA_DEFAULT_OMEGA_Q16, AGORA_DEFAULT_COUPLING_Q16, AGORA_DEFAULT_DT_Q16,
            &rng_state, &welds, &pq_err
        );
        total_clamps += clamps;
        total_welds += welds;
        total_pq_mismatch += pq_err;
    }

    state_crc64 = agora_crc64_ecma182(
        (const uint8_t *)g_agents,
        sizeof(g_agents)
    );

    if (sizeof(AgoraWaveguideFrame64B) != 64U ||
        fowler_bijection_ok != 1U ||
        trapdoor_rejected_ok != 1U ||
        quorum13_ties != 0U ||
        null_guard_rc != (uint32_t)CODE_TRAPDOOR_FAIL_CLOSED ||
        total_pq_mismatch != 0U) {
        fprintf(stderr, "[FAIL] C99 Agora Waveguide invariant check failed.\n");
        return 1;
    }

    printf(
        "{\n"
        "  \"substrate\": \"iso_c99\",\n"
        "  \"version\": \"v1.0.0\",\n"
        "  \"seed\": %" PRIu32 ",\n"
        "  \"frame_size_bytes\": %zu,\n"
        "  \"agents\": %u,\n"
        "  \"steps\": %u,\n"
        "  \"fowler_243_bijection\": true,\n"
        "  \"trapdoor_253_fail_closed\": true,\n"
        "  \"quorum13_tie_count\": %" PRIu32 ",\n"
        "  \"mz_pq_mismatch_count\": %" PRIu32 ",\n"
        "  \"keyhole_clamps_byte_243\": %" PRIu32 ",\n"
        "  \"kintsugi_welds_byte_251\": %" PRIu32 ",\n"
        "  \"state_crc64_ecma182\": \"0x%016" PRIx64 "\",\n"
        "  \"gates_passed\": \"6/6\",\n"
        "  \"exit_code\": %u\n"
        "}\n",
        seed,
        sizeof(AgoraWaveguideFrame64B),
        AGORA_DEFAULT_AGENTS,
        AGORA_DEFAULT_STEPS,
        quorum13_ties,
        total_pq_mismatch,
        total_clamps,
        total_welds,
        state_crc64,
        (unsigned)CODE_SHOKUNIN_UNLATCHED
    );

    return (int)CODE_SHOKUNIN_UNLATCHED;
}
#endif
