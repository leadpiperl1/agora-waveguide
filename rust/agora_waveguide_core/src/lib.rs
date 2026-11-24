//! Agora Waveguide Core (`v1.0.0`) -- `#![no_std]` 64-Byte C-ABI Verification Kernel
//!
//! Bit-exact mirror of `src/agora_waveguide_kernel.c` under the Agora Two-Plane Contract
//! and Bottom-Up Waveguide Invariants (`CODE_SHOKUNIN_UNLATCHED = 255`).

#![cfg_attr(not(any(test, feature = "std")), no_std)]

use core::mem::size_of;

#[cfg(not(any(test, feature = "std")))]
use core::panic::PanicInfo;

#[cfg(not(any(test, feature = "std")))]
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

pub const AGORA_FOWLER_MAX_PAYLOAD: u8 = 242;
pub const CODE_KEYHOLE_CLAMP: u8 = 243;
pub const CODE_KINTSUGI_WELD: u8 = 251;
pub const CODE_TRAPDOOR_FAIL_CLOSED: u8 = 253;
pub const CODE_SHOKUNIN_UNLATCHED: u8 = 255;

pub const AGORA_Q16_ONE: i32 = 65536;
pub const AGORA_Q16_KEYHOLE: i32 = 43691;
pub const AGORA_Q16_TRIT_THRESH: i32 = 21845;
pub const AGORA_DEFAULT_OMEGA_Q16: i32 = 9830;
pub const AGORA_DEFAULT_COUPLING_Q16: i32 = 16384;
pub const AGORA_DEFAULT_DT_Q16: i32 = 3277;

pub const AGORA_STATE_DIM: usize = 8;
pub const AGORA_HISTORY_LEN: usize = 8;
pub const AGORA_MAX_NEIGHBORS: usize = 4;
pub const AGORA_DEFAULT_AGENTS: usize = 256;
pub const AGORA_DEFAULT_STEPS: usize = 128;
pub const AGORA_DEFAULT_SEED: u32 = 42;

pub const AGORA_MZ_KERNEL_Q16: [i32; AGORA_HISTORY_LEN] = [
    16384, 8192, 4096, 2048, 1024, 512, 256, 128,
];

#[repr(C, align(64))]
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub struct AgoraWaveguideFrame64B {
    pub x_trit: [i8; AGORA_STATE_DIM],
    pub neighbors: [u16; AGORA_MAX_NEIGHBORS],
    pub mz_ring: [u8; AGORA_HISTORY_LEN],
    pub ring_head: u8,
    pub status_byte: u8,
    pub quorum_pole: u8,
    pub reserved_pad: u8,
    pub latent_q16: [i32; AGORA_STATE_DIM],
    pub frame_crc32: u32,
}

const _: () = assert!(size_of::<AgoraWaveguideFrame64B>() == 64);

impl AgoraWaveguideFrame64B {
    pub const fn zeroed() -> Self {
        Self {
            x_trit: [0; AGORA_STATE_DIM],
            neighbors: [0; AGORA_MAX_NEIGHBORS],
            mz_ring: [0; AGORA_HISTORY_LEN],
            ring_head: 0,
            status_byte: 0,
            quorum_pole: 1,
            reserved_pad: 0,
            latent_q16: [0; AGORA_STATE_DIM],
            frame_crc32: 0,
        }
    }
}

#[inline(always)]
fn agora_q16_mul(a: i32, b: i32) -> i32 {
    ((a as i64 * b as i64) / 65536_i64) as i32
}

#[inline(always)]
fn agora_xorshift32_noise(state: &mut u32) -> i32 {
    let mut x = *state;
    if x == 0 {
        x = 0x9E37_79B9;
    }
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    ((x >> 20) as i32) - 2048
}

fn agora_crc32_bytes(buf: &[u8]) -> u32 {
    let mut crc: u32 = 0xFFFF_FFFF;
    let mut i = 0;
    while i < buf.len() {
        crc ^= buf[i] as u32;
        let mut bit = 0;
        while bit < 8 {
            if (crc & 1) != 0 {
                crc = (crc >> 1) ^ 0xEDB8_8320;
            } else {
                crc >>= 1;
            }
            bit += 1;
        }
        i += 1;
    }
    crc ^ 0xFFFF_FFFF
}

fn agora_compute_frame_crc32(f: &AgoraWaveguideFrame64B) -> u32 {
    let mut raw = [0u8; 60];
    let mut pos = 0;
    let mut k = 0;
    while k < AGORA_STATE_DIM {
        raw[pos] = f.x_trit[k] as u8;
        pos += 1;
        k += 1;
    }
    k = 0;
    while k < AGORA_MAX_NEIGHBORS {
        let v = f.neighbors[k];
        raw[pos] = (v & 0xFF) as u8;
        raw[pos + 1] = ((v >> 8) & 0xFF) as u8;
        pos += 2;
        k += 1;
    }
    k = 0;
    while k < AGORA_HISTORY_LEN {
        raw[pos] = f.mz_ring[k];
        pos += 1;
        k += 1;
    }
    raw[pos] = f.ring_head;
    raw[pos + 1] = f.status_byte;
    raw[pos + 2] = f.quorum_pole;
    raw[pos + 3] = f.reserved_pad;
    pos += 4;
    k = 0;
    while k < AGORA_STATE_DIM {
        let u = f.latent_q16[k] as u32;
        raw[pos] = (u & 0xFF) as u8;
        raw[pos + 1] = ((u >> 8) & 0xFF) as u8;
        raw[pos + 2] = ((u >> 16) & 0xFF) as u8;
        raw[pos + 3] = ((u >> 24) & 0xFF) as u8;
        pos += 4;
        k += 1;
    }
    agora_crc32_bytes(&raw)
}

#[inline(always)]
pub fn fowler_pack_5trits(trits: &[i8; 5]) -> u8 {
    let mut acc: u16 = 0;
    let mut i = 0;
    while i < 5 {
        let t = trits[i].clamp(-1, 1);
        acc = acc * 3 + (t + 1) as u16;
        i += 1;
    }
    acc as u8
}

#[inline(always)]
pub fn fowler_unpack_5trits_strict(mut packed: u8, out_trits: &mut [i8; 5]) -> u8 {
    if packed > AGORA_FOWLER_MAX_PAYLOAD {
        *out_trits = [0; 5];
        return CODE_TRAPDOOR_FAIL_CLOSED;
    }
    let mut i = 5;
    while i > 0 {
        i -= 1;
        out_trits[i] = (packed % 3) as i8 - 1;
        packed /= 3;
    }
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn agora_fowler_pack_5trits(trits: *const i8) -> u8 {
    if trits.is_null() {
        return CODE_TRAPDOOR_FAIL_CLOSED;
    }
    let slice = unsafe { core::slice::from_raw_parts(trits, 5) };
    let arr = [slice[0], slice[1], slice[2], slice[3], slice[4]];
    fowler_pack_5trits(&arr)
}

#[unsafe(no_mangle)]
pub extern "C" fn agora_fowler_unpack_5trits_strict(packed: u8, out_trits: *mut i8) -> u8 {
    if out_trits.is_null() {
        return CODE_TRAPDOOR_FAIL_CLOSED;
    }
    let mut arr = [0i8; 5];
    let rc = fowler_unpack_5trits_strict(packed, &mut arr);
    let out = unsafe { core::slice::from_raw_parts_mut(out_trits, 5) };
    out.copy_from_slice(&arr);
    rc
}

#[unsafe(no_mangle)]
pub extern "C" fn agora_quorum13_audit_ties() -> u32 {
    let mut ties: u32 = 0;
    let mut mask: u32 = 0;
    while mask < 8192 {
        let mut sum: i32 = 0;
        let mut bit: u32 = 0;
        while bit < 13 {
            sum += if (mask & (1u32 << bit)) != 0 { 1 } else { -1 };
            bit += 1;
        }
        if sum == 0 {
            ties += 1;
        }
        mask += 1;
    }
    ties
}

pub fn init_agents_slice(agents: &mut [AgoraWaveguideFrame64B]) {
    let num_agents = agents.len();
    if num_agents == 0 {
        return;
    }
    for (i, a) in agents.iter_mut().enumerate() {
        let init_trit = ((i % 3) as i8) - 1;
        a.neighbors[0] = ((i + num_agents - 1) % num_agents) as u16;
        a.neighbors[1] = ((i + 1) % num_agents) as u16;
        a.neighbors[2] = ((i + num_agents - 5) % num_agents) as u16;
        a.neighbors[3] = ((i + 3) % num_agents) as u16;
        for k in 0..AGORA_STATE_DIM {
            a.x_trit[k] = init_trit;
            a.latent_q16[k] = (init_trit as i32) * 20000;
        }
        let first5 = [
            a.x_trit[0],
            a.x_trit[1],
            a.x_trit[2],
            a.x_trit[3],
            a.x_trit[4],
        ];
        let init_packed = fowler_pack_5trits(&first5);
        a.mz_ring = [init_packed; AGORA_HISTORY_LEN];
        a.ring_head = 0;
        a.status_byte = init_packed;
        a.quorum_pole = (init_trit + 1) as u8;
        a.reserved_pad = 0;
        a.frame_crc32 = agora_compute_frame_crc32(a);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn agora_init_agents(agents: *mut AgoraWaveguideFrame64B, num_agents: usize) {
    if agents.is_null() || num_agents == 0 {
        return;
    }
    let slice = unsafe { core::slice::from_raw_parts_mut(agents, num_agents) };
    init_agents_slice(slice);
}

#[unsafe(no_mangle)]
pub extern "C" fn agora_waveguide_step(
    agents: *mut AgoraWaveguideFrame64B,
    num_agents: usize,
    omega_q16: i32,
    coupling_q16: i32,
    dt_q16: i32,
    rng_state: *mut u32,
    out_welds: *mut u32,
    out_pq_mismatch: *mut u32,
) -> u32 {
    if agents.is_null() || rng_state.is_null() || num_agents == 0 {
        return CODE_TRAPDOOR_FAIL_CLOSED as u32;
    }

    let slice = unsafe { core::slice::from_raw_parts_mut(agents, num_agents) };
    let seed = unsafe { &mut *rng_state };
    let mut clamped_count: u32 = 0;
    let mut weld_count: u32 = 0;
    let mut pq_mismatch: u32 = 0;

    for i in 0..num_agents {
        let mut neighbor_sum = [0i32; AGORA_STATE_DIM];
        let mut mz_drag = [0i32; AGORA_STATE_DIM];
        let neighbors = slice[i].neighbors;

        for &nb_idx in &neighbors {
            if (nb_idx as usize) < num_agents {
                let nb_trits = slice[nb_idx as usize].x_trit;
                for k in 0..AGORA_STATE_DIM {
                    neighbor_sum[k] += nb_trits[k] as i32;
                }
            }
        }

        let head = slice[i].ring_head as usize;
        for tau in 0..AGORA_HISTORY_LEN {
            let slot = (head + AGORA_HISTORY_LEN - tau) % AGORA_HISTORY_LEN;
            let mut past_trits = [0i8; 5];
            let _ = fowler_unpack_5trits_strict(slice[i].mz_ring[slot], &mut past_trits);
            let weight = AGORA_MZ_KERNEL_Q16[tau];
            for k in 0..5 {
                mz_drag[k] += (past_trits[k] as i32) * weight;
            }
        }

        let agent = &mut slice[i];
        let mut saturated = false;
        let mut max_abs_dx: i32 = 0;
        let mut trit_sum: i32 = 0;

        for k in 0..AGORA_STATE_DIM {
            let x_curr = (agent.x_trit[k] as i32) * AGORA_Q16_ONE;
            let omega = agora_q16_mul(omega_q16, x_curr);
            let social = neighbor_sum[k] * coupling_q16;
            let noise = agora_xorshift32_noise(seed);
            let dx = omega - mz_drag[k] + social + noise;
            let abs_dx = if dx < 0 { -dx } else { dx };
            if abs_dx > max_abs_dx {
                max_abs_dx = abs_dx;
            }

            let delta = agora_q16_mul(dx, dt_q16);
            let mut next_val = agent.latent_q16[k].saturating_add(delta);
            if next_val > AGORA_Q16_ONE {
                next_val = AGORA_Q16_ONE;
                saturated = true;
            } else if next_val < -AGORA_Q16_ONE {
                next_val = -AGORA_Q16_ONE;
                saturated = true;
            }

            agent.latent_q16[k] = next_val;
            agent.x_trit[k] = if next_val > AGORA_Q16_TRIT_THRESH {
                1
            } else if next_val < -AGORA_Q16_TRIT_THRESH {
                -1
            } else {
                0
            };
            trit_sum += agent.x_trit[k] as i32;

            let p_proj = (agent.x_trit[k] as i32) * AGORA_Q16_ONE;
            let q_orth = next_val - p_proj;
            if (p_proj + q_orth) != next_val {
                pq_mismatch += 1;
            }
        }

        let next_head = ((head + 1) % AGORA_HISTORY_LEN) as u8;
        agent.ring_head = next_head;
        let first5 = [
            agent.x_trit[0],
            agent.x_trit[1],
            agent.x_trit[2],
            agent.x_trit[3],
            agent.x_trit[4],
        ];
        let packed = fowler_pack_5trits(&first5);
        agent.mz_ring[next_head as usize] = packed;

        agent.status_byte = if saturated {
            clamped_count += 1;
            CODE_KEYHOLE_CLAMP
        } else if max_abs_dx > AGORA_Q16_KEYHOLE {
            weld_count += 1;
            CODE_KINTSUGI_WELD
        } else {
            packed
        };

        agent.quorum_pole = if trit_sum < 0 {
            0
        } else if trit_sum > 0 {
            2
        } else {
            1
        };
        agent.reserved_pad = 0;
        agent.frame_crc32 = agora_compute_frame_crc32(agent);
    }

    if !out_welds.is_null() {
        unsafe { *out_welds = weld_count };
    }
    if !out_pq_mismatch.is_null() {
        unsafe { *out_pq_mismatch = pq_mismatch };
    }
    clamped_count
}

pub fn crc64_ecma182_slice(data: &[u8]) -> u64 {
    let mut crc: u64 = 0;
    for &b in data {
        crc ^= (b as u64) << 56;
        for _ in 0..8 {
            if (crc & 0x8000_0000_0000_0000) != 0 {
                crc = (crc << 1) ^ 0x42F0_E1EB_A9EA_3693;
            } else {
                crc <<= 1;
            }
        }
    }
    crc
}

#[unsafe(no_mangle)]
pub extern "C" fn agora_crc64_ecma182(data: *const u8, len: usize) -> u64 {
    if data.is_null() {
        return 0;
    }
    let slice = unsafe { core::slice::from_raw_parts(data, len) };
    crc64_ecma182_slice(slice)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn verify_abi_and_invariants() {
        assert_eq!(size_of::<AgoraWaveguideFrame64B>(), 64);
        assert_eq!(agora_quorum13_audit_ties(), 0);
        for b in 0u8..=242u8 {
            let mut trits = [0i8; 5];
            assert_eq!(fowler_unpack_5trits_strict(b, &mut trits), 0);
            assert_eq!(fowler_pack_5trits(&trits), b);
        }
        for b in 243u8..=255u8 {
            let mut trits = [0i8; 5];
            assert_eq!(
                fowler_unpack_5trits_strict(b, &mut trits),
                CODE_TRAPDOOR_FAIL_CLOSED
            );
        }
    }
}
