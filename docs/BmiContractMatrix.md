# BMI public contract matrix

Phase 1 classifies every symbol in `SimdLib::Bmi` that is outside its nested
`Detail` namespace. All tests use the public `Bmi` entry points; `Detail`
contains implementation alternatives and is not a supported test seam.

| Family | Public status | Contract and boundary semantics | Runtime proof |
| --- | --- | --- | --- |
| `boolmask`, `select`, `min`, `max`, `abs` | Supported | `boolmask(false)` is zero and `boolmask(true)` is all ones in the result width. `select` chooses its false-case operand for false and its true-case operand for true. `min`/`max` retain normal signed or unsigned ordering. `abs` returns unsigned input unchanged; signed minimum retains its two-complement bit pattern because its magnitude is unrepresentable. | Derived helper contracts; signed-boundary test |
| `andn`, `bzhi`, `blsi`, `blsr`, `blsmsk`, `mulx` | Supported | Operate on object-representation bits. `bzhi` returns zero at index zero and source at an index at or above width. Low/high `mulx` words are unsigned-product bits. `blsi`, `blsr`, and `blsmsk` define zero through their standard wraparound bit formulas. | Exhaustive 8/16-bit, randomized 32/64-bit, signed-bit-pattern, and uint128 tests |
| `blse`, `blsioff`, `bmsi`, `bmsr`, `bmse`, `bzlo`, `bmsmsk` | Supported | Extract/reset helpers return a cleared value and write or return the selected bit. `bmsr(value, index)` writes `-1` for zero; `bmse` writes zero for zero. Bit indices are zero-based. `bzlo` clears bits below its index; index zero preserves source. | Derived helper contracts |
| `pp_or`, `ps_or`, `pp_lsor` | Supported | Full-width bit-propagation operations. `pp_or` propagates the most-significant set bit through every lower bit; `ps_or` propagates the least-significant set bit through every higher bit; `pp_lsor` returns the mask from bit zero through the least-significant set bit. Zero produces zero. | Exhaustive 8-bit scalar oracles and 16-bit boundary tables |
| `pp_xor`, `ps_xor`, `pp_and`, `ps_and`, `pp_andn`, `ps_andn`, `pp_andni`, `ps_andni` | Supported | These are distance-1 adjacent-bit stages of the form `value op (value >> 1)` or `value op (value << 1)`. Prefix stages combine with the adjacent bit to the right (high-bits); suffix stages combine with the adjacent bit to the left (low-bits). The AND-NOT variants retain or mark the corresponding run-boundary bits. | Exhaustive 8-bit scalar oracles |
| `PartialSumBLSMSK`, `PartialSumBLSI` | Compatibility surface | Retain the existing 32-bit-mask staged accumulation formulas for integer-like values. For example, `PartialSumBLSMSK(0b1011) == 37` and `PartialSumBLSI(0b1011) == 24`. | Derived helper contracts |
| `flipr_unset`, `maskr_unset`, `maskl_trailing_one`, trailing/leading mask helpers | Supported | Safe variants explicitly define zero: `mask_trailing_zeros_or_zero(0)` and `mask_bits_lower_than_lsb(0)` return zero, while `mask_bits_lower_than_lsb_or_all_ones(0)` returns all ones. Full-width all-ones input leaves `flipr_unset` unchanged and yields zero from `maskr_unset`. | Derived helper contracts |
| `clear_lowest_set_bits`, `consume_bit_sequence_right`, `consume_bit_sequence_left`, `left_collapse_trailing_bits` | Supported | A contiguous sequence is the trailing or leading contiguous run described by each function. `clear_lowest_set_bits(value, out)` writes `value ^ cleared_mask`, i.e. the full consumed low-bit mask (`0b1011` writes `0b111`). Tuple overloads return `{remaining, consumed}`. | Derived helper contracts |
| `clear_bits_lower_than`, `clear_bits_higher_than`, `extract_bits_lower_than`, `extract_bits_higher_than` | Supported | `target_bit` is a one-hot bit and is excluded from the lower/higher partition as named. | Derived helper contracts |
| `bextr` overloads | Supported | Runtime controls use `(source, length, start)`. Zero length or start at/above width returns zero; ranges truncate at source width. Template controls are compile-time constrained to the intrinsic control field. | Exhaustive 8-bit and randomized 32/64-bit tests; constexpr probes |
| `pdep_u32`, `pdep_u64`, `pdepl_u32`, `pdepl_u64`, `pext_u32`, `pext_u64` | Supported | `pdep`/`pext` use ascending selected mask bits. `pdepl` first applies `operator>>` to the source by the complement-mask population count modulo its word width, then deposits it. | Exhaustive 8-bit masks, randomized 32/64-bit, and derived helper contracts |

The generic public wrappers are exercised over `uint8_t`, `uint16_t`,
`uint32_t`, `uint64_t`, and the cycle-free `uint128_t` test customization.
Signed `int32_t`/`int64_t` object-representation checks protect the signed
contracts. The portable, BMI1-only, BMI2-only, and combined profiles must
produce the same deterministic digest; their CTest equivalence tests are the
configuration proof.

## Phase 1 validation record

On 2026-07-18, the `clang-coverage` build ran 125 CTest entries successfully.
The BMI subset ran 47 entries: eleven public-contract tests in each of the
portable, BMI1-only, BMI2-only, and combined configurations, followed by the
three enabled-versus-portable deterministic-digest equivalence tests. All 47
passed. The exhaustive 8-bit contracts exposed and fixed narrow-integer
promotion defects in the AND-NOT and unset/trailing-mask helper families. The
deterministic seeds remain `0xC001D00D12345678`,
`0x9E3779B97F4A7C15`, `0xD1B54A32D192ED03`, and
`0xA0761D6478BD642F`.
