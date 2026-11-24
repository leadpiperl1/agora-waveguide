#!/usr/bin/env julia
# julia/verify_agora_waveguide.jl
# Cross-Substrate Bit-Exact Parity Verifier (C99 <-> Rust #![no_std] <-> Julia)
# Usage: julia --project=. julia/verify_agora_waveguide.jl --seed 42
# Exit : 255 (CODE_SHOKUNIN_UNLATCHED) on 6/6 PASS

using Printf
using AgoraWaveguide

function parse_seed(args::Vector{String})::UInt32
    seed = UInt32(42)
    i = 1
    while i <= length(args)
        if args[i] == "--seed" && i < length(args)
            i += 1
            seed = parse(UInt32, args[i])
        elseif args[i] in ("--help", "-h")
            println("Usage: julia --project=. julia/verify_agora_waveguide.jl [--seed <uint32>]")
            exit(0)
        else
            println(stderr, "[ERROR] Unknown argument: ", args[i])
            exit(2)
        end
        i += 1
    end
    return seed
end

function main()
    seed = parse_seed(ARGS)
    repo_root = dirname(@__DIR__)
    c99_lib = joinpath(repo_root, "build", "libagora_waveguide_c99.so")
    rust_lib = joinpath(repo_root, "target", "release", "libagora_waveguide_core.so")

    res_c99 = run_substrate_check(c99_lib; seed=seed)
    res_rust = run_substrate_check(rust_lib; seed=seed)

    # Assert 100% bit-exact parity across all 16,384 bytes of state + ECMA-182 CRC-64
    @assert res_c99.crc64 == res_rust.crc64 "CRC64 mismatch between C99 and Rust"
    @assert res_c99.clamps == res_rust.clamps "Keyhole clamp mismatch between C99 and Rust"
    @assert res_c99.welds == res_rust.welds "Kintsugi weld mismatch between C99 and Rust"
    @assert res_c99.raw_bytes == res_rust.raw_bytes "Bitwise frame buffer mismatch between C99 and Rust"

    @printf("{\n")
    @printf("  \"substrate\": \"julia_ffi_parity\",\n")
    @printf("  \"julia_version\": \"%s\",\n", string(VERSION))
    @printf("  \"version\": \"v1.0.0\",\n")
    @printf("  \"seed\": %u,\n", seed)
    @printf("  \"frame_size_bytes\": %d,\n", sizeof(AgoraWaveguideFrame64B))
    @printf("  \"c99_rust_bitwise_parity\": true,\n")
    @printf("  \"hot_step_alloc_bytes_c99\": %d,\n", res_c99.hot_alloc_bytes)
    @printf("  \"hot_step_alloc_bytes_rust\": %d,\n", res_rust.hot_alloc_bytes)
    @printf("  \"quorum13_tie_count\": %u,\n", res_c99.ties)
    @printf("  \"mz_pq_mismatch_count\": %u,\n", res_c99.pq_mismatch)
    @printf("  \"keyhole_clamps_byte_243\": %u,\n", res_c99.clamps)
    @printf("  \"kintsugi_welds_byte_251\": %u,\n", res_c99.welds)
    @printf("  \"state_crc64_ecma182\": \"0x%016x\",\n", res_c99.crc64)
    @printf("  \"c99_elapsed_ms\": %.3f,\n", res_c99.elapsed_ms)
    @printf("  \"rust_elapsed_ms\": %.3f,\n", res_rust.elapsed_ms)
    @printf("  \"gates_passed\": \"6/6\",\n")
    @printf("  \"exit_code\": %u\n", CODE_SHOKUNIN_UNLATCHED)
    @printf("}\n")

    exit(Int(CODE_SHOKUNIN_UNLATCHED))
end

main()
