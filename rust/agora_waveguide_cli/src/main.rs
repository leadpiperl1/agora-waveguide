//! Agora Waveguide Rust CLI Verifier (`--seed 42`, `exit = 255`)

use agora_waveguide_core::{
    agora_quorum13_audit_ties, agora_waveguide_step, crc64_ecma182_slice, fowler_pack_5trits,
    fowler_unpack_5trits_strict, init_agents_slice, AgoraWaveguideFrame64B, AGORA_DEFAULT_AGENTS,
    AGORA_DEFAULT_COUPLING_Q16, AGORA_DEFAULT_DT_Q16, AGORA_DEFAULT_OMEGA_Q16, AGORA_DEFAULT_SEED,
    AGORA_DEFAULT_STEPS, CODE_SHOKUNIN_UNLATCHED, CODE_TRAPDOOR_FAIL_CLOSED,
};
use core::mem::size_of;
use std::env;
use std::process;

fn main() {
    let mut seed: u32 = AGORA_DEFAULT_SEED;
    let args: Vec<String> = env::args().collect();
    let mut i = 1;
    while i < args.len() {
        match args[i].as_str() {
            "--seed" => {
                if i + 1 >= args.len() {
                    eprintln!("[ERROR] Missing value after --seed");
                    process::exit(2);
                }
                i += 1;
                seed = match args[i].parse::<u32>() {
                    Ok(v) => v,
                    Err(_) => {
                        eprintln!("[ERROR] Invalid --seed value: {}", args[i]);
                        process::exit(2);
                    }
                };
            }
            "--help" | "-h" => {
                println!("Usage: verify_agora_rust [--seed <uint32>]");
                process::exit(0);
            }
            other => {
                eprintln!("[ERROR] Unknown argument: {}", other);
                process::exit(2);
            }
        }
        i += 1;
    }

    /* Gate 2: Exhaustive Fowler 243-state bijection & Byte 253 trapdoor rejection */
    let mut fowler_ok = true;
    for b in 0u8..=242u8 {
        let mut trits = [0i8; 5];
        if fowler_unpack_5trits_strict(b, &mut trits) != 0 || fowler_pack_5trits(&trits) != b {
            fowler_ok = false;
        }
    }
    let mut trapdoor_ok = true;
    for b in 243u8..=255u8 {
        let mut trits = [0i8; 5];
        if fowler_unpack_5trits_strict(b, &mut trits) != CODE_TRAPDOOR_FAIL_CLOSED {
            trapdoor_ok = false;
        }
    }

    /* Gate 3: 12+1=13 Odd-parity quorum zero-tie invariant */
    let quorum13_ties = agora_quorum13_audit_ties();

    /* Gate 5a: Null-pointer fail-closed guard */
    let mut rng_state: u32 = seed;
    let null_guard_rc = agora_waveguide_step(
        core::ptr::null_mut(),
        AGORA_DEFAULT_AGENTS,
        AGORA_DEFAULT_OMEGA_Q16,
        AGORA_DEFAULT_COUPLING_Q16,
        AGORA_DEFAULT_DT_Q16,
        &mut rng_state,
        core::ptr::null_mut(),
        core::ptr::null_mut(),
    );

    /* Gate 4: Deterministic Mori-Zwanzig simulation */
    let mut agents = [AgoraWaveguideFrame64B::zeroed(); AGORA_DEFAULT_AGENTS];
    init_agents_slice(&mut agents);
    rng_state = if seed != 0 { seed } else { 0x9E37_79B9 };

    let mut total_clamps: u32 = 0;
    let mut total_welds: u32 = 0;
    let mut total_pq_mismatch: u32 = 0;

    for _ in 0..AGORA_DEFAULT_STEPS {
        let mut welds: u32 = 0;
        let mut pq_err: u32 = 0;
        let clamps = agora_waveguide_step(
            agents.as_mut_ptr(),
            agents.len(),
            AGORA_DEFAULT_OMEGA_Q16,
            AGORA_DEFAULT_COUPLING_Q16,
            AGORA_DEFAULT_DT_Q16,
            &mut rng_state,
            &mut welds,
            &mut pq_err,
        );
        total_clamps += clamps;
        total_welds += welds;
        total_pq_mismatch += pq_err;
    }

    let raw_bytes = unsafe {
        core::slice::from_raw_parts(
            agents.as_ptr() as *const u8,
            agents.len() * size_of::<AgoraWaveguideFrame64B>(),
        )
    };
    let state_crc64 = crc64_ecma182_slice(raw_bytes);

    if size_of::<AgoraWaveguideFrame64B>() != 64
        || !fowler_ok
        || !trapdoor_ok
        || quorum13_ties != 0
        || null_guard_rc != CODE_TRAPDOOR_FAIL_CLOSED as u32
        || total_pq_mismatch != 0
    {
        eprintln!("[FAIL] Rust Agora Waveguide invariant check failed.");
        process::exit(1);
    }

    println!(
        "{{\n  \"substrate\": \"rust_no_std\",\n  \"version\": \"v1.0.0\",\n  \"seed\": {},\n  \"frame_size_bytes\": {},\n  \"agents\": {},\n  \"steps\": {},\n  \"fowler_243_bijection\": true,\n  \"trapdoor_253_fail_closed\": true,\n  \"quorum13_tie_count\": {},\n  \"mz_pq_mismatch_count\": {},\n  \"keyhole_clamps_byte_243\": {},\n  \"kintsugi_welds_byte_251\": {},\n  \"state_crc64_ecma182\": \"0x{:016x}\",\n  \"gates_passed\": \"6/6\",\n  \"exit_code\": {}\n}}",
        seed,
        size_of::<AgoraWaveguideFrame64B>(),
        AGORA_DEFAULT_AGENTS,
        AGORA_DEFAULT_STEPS,
        quorum13_ties,
        total_pq_mismatch,
        total_clamps,
        total_welds,
        state_crc64,
        CODE_SHOKUNIN_UNLATCHED
    );

    process::exit(CODE_SHOKUNIN_UNLATCHED as i32);
}
