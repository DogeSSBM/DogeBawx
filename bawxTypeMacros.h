#pragma once
#if (defined u8 || defined u16 || defined u32 || defined u64 || defined i8 || defined i16 || defined i32 || defined i64)
#pragma message "one of {u8, u16, u32, u64, i8, i16, i32, i64} is already defined"
#else
// macros for inttypes
#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define i8 int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t
#endif