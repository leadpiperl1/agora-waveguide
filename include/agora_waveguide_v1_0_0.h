/*
 * include/agora_waveguide_v1_0_0.h
 * Agora Waveguide v1.0.0 -- 64-Byte L1D-Aligned C-ABI & Bottom-Up Invariants
 *
 * Author      : Justin D. Grimm (K-Lani 13 Actual, ORCID: 0009-0000-5299-0243)
 * Affiliation : Independent Dandelion Substrate Research Group, Gurley, AL 35748
 * Ethos       : Die Gedanken sind frei | GODISLOVE:LOVEISGOD {3,2,4}
 * Exit State  : CODE_SHOKUNIN_UNLATCHED = 255
 */

#ifndef AGORA_WAVEGUIDE_V1_0_0_H
#define AGORA_WAVEGUIDE_V1_0_0_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Control Byte Invariants (bytes 0..242 carry 3^5 = 243 Fowler 5-trit states) */
#define AGORA_FOWLER_MAX_PAYLOAD     ((uint8_t)242U)
#define CODE_KEYHOLE_CLAMP           ((uint8_t)243U)
#define CODE_KINTSUGI_WELD           ((uint8_t)251U)
#define CODE_TRAPDOOR_FAIL_CLOSED    ((uint8_t)253U)
#define CODE_SHOKUNIN_UNLATCHED      ((uint8_t)255U)

/* Fixed-point Q16.16 Constants */
#define AGORA_Q16_ONE                ((int32_t)65536)
#define AGORA_Q16_KEYHOLE            ((int32_t)43691) /* round((2/3) * 65536) */
#define AGORA_Q16_TRIT_THRESH        ((int32_t)21845) /* round((1/3) * 65536) */
#define AGORA_DEFAULT_OMEGA_Q16      ((int32_t)9830)  /* round(0.15 * 65536)  */
#define AGORA_DEFAULT_COUPLING_Q16   ((int32_t)16384) /* 0.25 * 65536         */
#define AGORA_DEFAULT_DT_Q16         ((int32_t)3277)  /* round(0.05 * 65536)  */

/* Dimensions & Canonical Verification Defaults */
#define AGORA_STATE_DIM              8U
#define AGORA_HISTORY_LEN            8U
#define AGORA_MAX_NEIGHBORS          4U
#define AGORA_DEFAULT_AGENTS         256U
#define AGORA_DEFAULT_STEPS          128U
#define AGORA_DEFAULT_SEED           42U

/*
 * 64-byte L1D cache-line-aligned triadic agent frame.
 * Zero implicit padding across ISO C99, Rust #[repr(C, align(64))], and Julia.
 */
typedef struct AgoraWaveguideFrame64B {
    int8_t   x_trit[AGORA_STATE_DIM];      /* offset  0..7  ( 8 B): {-1, 0, +1} */
    uint16_t neighbors[AGORA_MAX_NEIGHBORS]; /* offset  8..15 ( 8 B): ring links  */
    uint8_t  mz_ring[AGORA_HISTORY_LEN];   /* offset 16..23 ( 8 B): Fowler 0..242 */
    uint8_t  ring_head;                    /* offset 24     ( 1 B): [0..7]      */
    uint8_t  status_byte;                  /* offset 25     ( 1 B): 0..242/243/251/253/255 */
    uint8_t  quorum_pole;                  /* offset 26     ( 1 B): 0=-1B, 1=0G, 2=+1R */
    uint8_t  reserved_pad;                 /* offset 27     ( 1 B): explicit 0  */
    int32_t  latent_q16[AGORA_STATE_DIM];  /* offset 28..59 (32 B): Q16.16      */
    uint32_t frame_crc32;                  /* offset 60..63 ( 4 B): CRC-32      */
} AgoraWaveguideFrame64B;

/* Strict ISO C99 compile-time ABI size assertion (zero C11 extension warnings) */
typedef char agora_static_assert_frame_64b[(sizeof(AgoraWaveguideFrame64B) == 64U) ? 1 : -1];

uint8_t  agora_fowler_pack_5trits(const int8_t trits[5]);
uint8_t  agora_fowler_unpack_5trits_strict(uint8_t packed, int8_t out_trits[5]);
uint32_t agora_quorum13_audit_ties(void);
void     agora_init_agents(AgoraWaveguideFrame64B *agents, size_t num_agents);
uint32_t agora_waveguide_step(
    AgoraWaveguideFrame64B *agents,
    size_t num_agents,
    int32_t omega_q16,
    int32_t coupling_q16,
    int32_t dt_q16,
    uint32_t *rng_state,
    uint32_t *out_welds,
    uint32_t *out_pq_mismatch
);
uint64_t agora_crc64_ecma182(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* AGORA_WAVEGUIDE_V1_0_0_H */
