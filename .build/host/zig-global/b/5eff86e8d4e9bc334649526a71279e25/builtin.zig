const std = @import("std");
pub const zig_version = std.SemanticVersion.parse(zig_version_string) catch unreachable;
pub const zig_version_string = "0.17.1";
pub const zig_backend = std.lang.CompilerBackend.stage2_llvm;

pub const output_mode: std.lang.OutputMode = .Lib;
pub const link_mode: std.lang.LinkMode = .static;
pub const unwind_tables: std.lang.UnwindTables = .async;
pub const is_test = false;
pub const single_threaded = false;
/// Deprecated; to be removed in 0.18.0. Use `target.abi` instead.
pub const abi: std.Target.Abi = .gnu;
/// Deprecated; to be removed in 0.18.0. Use `target.cpu` instead.
pub const cpu: std.Target.Cpu = .{
    .arch = .x86_64,
    .model = &std.Target.x86.cpu.znver1,
    .features = std.Target.x86.featureSet(&.{
        .@"64bit",
        .adx,
        .aes,
        .allow_light_256_bit,
        .avx,
        .avx2,
        .bmi,
        .bmi2,
        .branchfusion,
        .clflushopt,
        .clzero,
        .cmov,
        .crc32,
        .cx16,
        .cx8,
        .f16c,
        .fast_15bytenop,
        .fast_bextr,
        .fast_imm16,
        .fast_lzcnt,
        .fast_movbe,
        .fast_scalar_fsqrt,
        .fast_scalar_shift_masks,
        .fast_variable_perlane_shuffle,
        .fast_vector_fsqrt,
        .fma,
        .fsgsbase,
        .fxsr,
        .idivq_to_divl,
        .lzcnt,
        .mmx,
        .movbe,
        .mwaitx,
        .nopl,
        .pclmul,
        .popcnt,
        .prfchw,
        .rdrnd,
        .rdseed,
        .sahf,
        .sbb_dep_breaking,
        .sha,
        .slow_shld,
        .smap,
        .smep,
        .sse,
        .sse2,
        .sse3,
        .sse4_1,
        .sse4_2,
        .sse4a,
        .ssse3,
        .vzeroupper,
        .x87,
        .xsave,
        .xsavec,
        .xsaveopt,
        .xsaves,
    }),
};
/// Deprecated; to be removed in 0.18.0. Use `target.os` instead.
pub const os: std.Target.Os = .{
    .tag = .windows,
    .version_range = .{ .windows = .{
        .min = .win11_ge,
        .max = .win11_ge,
    }},
};
pub const target: std.Target = .{
    .cpu = cpu,
    .os = os,
    .abi = abi,
    .ofmt = object_format,
    .dynamic_linker = .none,
};
/// Deprecated; to be removed in 0.18.0. Use `target.ofmt` instead.
pub const object_format: std.Target.ObjectFormat = .coff;
/// Deprecated, to be removed after 0.18.0
pub const mode = optimize;
pub const optimize: std.lang.Optimize = .fast;
pub const link_libc = true;
pub const link_libcpp = false;
pub const have_error_return_tracing = false;
pub const valgrind_support = false;
pub const sanitize_thread = false;
pub const fuzz = false;
pub const position_independent_code = true;
pub const position_independent_executable = false;
pub const strip_debug_info = false;
pub const code_model: std.lang.CodeModel = .default;
pub const omit_frame_pointer = false;
