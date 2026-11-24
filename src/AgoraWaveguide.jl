module AgoraWaveguide

using Printf
using Test

export AgoraWaveguideFrame64B,
       CODE_KEYHOLE_CLAMP,
       CODE_KINTSUGI_WELD,
       CODE_TRAPDOOR_FAIL_CLOSED,
       CODE_SHOKUNIN_UNLATCHED,
       run_substrate_check

const CODE_KEYHOLE_CLAMP        = UInt8(243)
const CODE_KINTSUGI_WELD        = UInt8(251)
const CODE_TRAPDOOR_FAIL_CLOSED = UInt8(253)
const CODE_SHOKUNIN_UNLATCHED   = UInt8(255)

const AGORA_DEFAULT_OMEGA_Q16    = Int32(9830)
const AGORA_DEFAULT_COUPLING_Q16 = Int32(16384)
const AGORA_DEFAULT_DT_Q16       = Int32(3277)
const AGORA_DEFAULT_AGENTS       = 256
const AGORA_DEFAULT_STEPS        = 128

struct AgoraWaveguideFrame64B
    x_trit::NTuple{8, Int8}
    neighbors::NTuple{4, UInt16}
    mz_ring::NTuple{8, UInt8}
    ring_head::UInt8
    status_byte::UInt8
    quorum_pole::UInt8
    reserved_pad::UInt8
    latent_q16::NTuple{8, Int32}
    frame_crc32::UInt32
end

@assert sizeof(AgoraWaveguideFrame64B) == 64 "ABI error: AgoraWaveguideFrame64B must be exactly 64 bytes"

@inline function ccall_step(
    step_fn::Ptr{Cvoid},
    agents::Vector{AgoraWaveguideFrame64B},
    rng_ref::Base.RefValue{UInt32},
    welds_ref::Base.RefValue{UInt32},
    pq_ref::Base.RefValue{UInt32}
)::UInt32
    return GC.@preserve agents rng_ref welds_ref pq_ref begin
        ccall(
            step_fn,
            UInt32,
            (Ptr{AgoraWaveguideFrame64B}, Csize_t, Int32, Int32, Int32, Ptr{UInt32}, Ptr{UInt32}, Ptr{UInt32}),
            pointer(agents),
            Csize_t(length(agents)),
            AGORA_DEFAULT_OMEGA_Q16,
            AGORA_DEFAULT_COUPLING_Q16,
            AGORA_DEFAULT_DT_Q16,
            Base.unsafe_convert(Ptr{UInt32}, rng_ref),
            Base.unsafe_convert(Ptr{UInt32}, welds_ref),
            Base.unsafe_convert(Ptr{UInt32}, pq_ref)
        )
    end
end

function run_substrate_check(libpath::String; seed::UInt32=UInt32(42))
    isfile(libpath) || error("Shared library missing: $libpath")

    handle = Base.Libc.Libdl.dlopen(libpath)
    try
        pack_fn   = Base.Libc.Libdl.dlsym(handle, :agora_fowler_pack_5trits)
        unpack_fn = Base.Libc.Libdl.dlsym(handle, :agora_fowler_unpack_5trits_strict)
        quorum_fn = Base.Libc.Libdl.dlsym(handle, :agora_quorum13_audit_ties)
        init_fn   = Base.Libc.Libdl.dlsym(handle, :agora_init_agents)
        step_fn   = Base.Libc.Libdl.dlsym(handle, :agora_waveguide_step)
        crc64_fn  = Base.Libc.Libdl.dlsym(handle, :agora_crc64_ecma182)

        # Gate 2: Fowler 243-state bijection & Byte 253 trapdoor rejection via ccall
        trits_buf = Vector{Int8}(undef, 5)
        for b in UInt8(0):UInt8(242)
            rc = GC.@preserve trits_buf ccall(
                unpack_fn, UInt8, (UInt8, Ptr{Int8}), b, pointer(trits_buf)
            )
            repacked = GC.@preserve trits_buf ccall(
                pack_fn, UInt8, (Ptr{Int8},), pointer(trits_buf)
            )
            @assert rc == 0x00 && repacked == b "Fowler bijection mismatch at byte $b"
        end
        for b in UInt8(243):UInt8(255)
            rc = GC.@preserve trits_buf ccall(
                unpack_fn, UInt8, (UInt8, Ptr{Int8}), b, pointer(trits_buf)
            )
            @assert rc == CODE_TRAPDOOR_FAIL_CLOSED "Control byte $b failed trapdoor rejection"
        end

        # Gate 3: 12+1=13 odd-parity quorum zero-tie invariant
        ties = ccall(quorum_fn, UInt32, ())
        @assert ties == 0 "Quorum-13 tie invariant violated: $ties"

        # Gate 5a: Null-pointer fail-closed trapdoor
        rng_null = Ref{UInt32}(seed)
        null_rc = GC.@preserve rng_null ccall(
            step_fn,
            UInt32,
            (Ptr{AgoraWaveguideFrame64B}, Csize_t, Int32, Int32, Int32, Ptr{UInt32}, Ptr{UInt32}, Ptr{UInt32}),
            C_NULL, Csize_t(AGORA_DEFAULT_AGENTS),
            AGORA_DEFAULT_OMEGA_Q16, AGORA_DEFAULT_COUPLING_Q16, AGORA_DEFAULT_DT_Q16,
            Base.unsafe_convert(Ptr{UInt32}, rng_null), C_NULL, C_NULL
        )
        @assert null_rc == UInt32(CODE_TRAPDOOR_FAIL_CLOSED) "Null guard failed: $null_rc"

        # Gate 4 + Gate 5b: Warmup JIT & measure zero-allocation hot step
        agents = Vector{AgoraWaveguideFrame64B}(undef, AGORA_DEFAULT_AGENTS)
        GC.@preserve agents ccall(
            init_fn, Cvoid, (Ptr{AgoraWaveguideFrame64B}, Csize_t),
            pointer(agents), Csize_t(length(agents))
        )
        rng_ref = Ref{UInt32}(seed != 0 ? seed : UInt32(0x9E3779B9))
        welds_ref = Ref{UInt32}(0)
        pq_ref = Ref{UInt32}(0)

        warm_agents = copy(agents)
        warm_rng = Ref{UInt32}(rng_ref[])
        ccall_step(step_fn, warm_agents, warm_rng, welds_ref, pq_ref)
        hot_step_alloc_bytes = @allocated ccall_step(step_fn, warm_agents, warm_rng, welds_ref, pq_ref)
        @assert hot_step_alloc_bytes == 0 "Heap allocation detected in Julia ccall hot step: $(hot_step_alloc_bytes) bytes"

        total_clamps = UInt32(0)
        total_welds = UInt32(0)
        total_pq_mismatch = UInt32(0)
        t_start = time_ns()
        for _ in 1:AGORA_DEFAULT_STEPS
            total_clamps += ccall_step(step_fn, agents, rng_ref, welds_ref, pq_ref)
            total_welds += welds_ref[]
            total_pq_mismatch += pq_ref[]
        end
        elapsed_ms = (time_ns() - t_start) / 1e6
        @assert total_pq_mismatch == 0 "Mori-Zwanzig P+Q=I conservation mismatch: $total_pq_mismatch"

        nbytes = length(agents) * sizeof(AgoraWaveguideFrame64B)
        crc64 = GC.@preserve agents ccall(
            crc64_fn, UInt64, (Ptr{UInt8}, Csize_t),
            Ptr{UInt8}(pointer(agents)), Csize_t(nbytes)
        )

        raw_copy = Vector{UInt8}(undef, nbytes)
        GC.@preserve agents raw_copy unsafe_copyto!(pointer(raw_copy), Ptr{UInt8}(pointer(agents)), nbytes)

        return (
            ties=ties,
            clamps=total_clamps,
            welds=total_welds,
            pq_mismatch=total_pq_mismatch,
            crc64=crc64,
            hot_alloc_bytes=hot_step_alloc_bytes,
            elapsed_ms=elapsed_ms,
            raw_bytes=raw_copy,
        )
    finally
        Base.Libc.Libdl.dlclose(handle)
    end
end

end # module AgoraWaveguide
