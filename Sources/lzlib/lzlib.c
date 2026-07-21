/* Lzlib - Compression library for the lzip format
   Copyright (C) 2009-2026 Antonio Diaz Diaz.

   This library is free software. Redistribution and use in source and
   binary forms, with or without modification, are permitted provided
   that the following conditions are met:

   1. Redistributions of source code must retain the above copyright
   notice, this list of conditions, and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions, and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* LZ_API_VERSION was first defined in lzlib 1.8 to 1.
   Since lzlib 1.12, LZ_API_VERSION is defined as (major * 1000 + minor). */

#define LZ_API_VERSION 1016

static const char * const LZ_version_string = "1.16";

typedef enum LZ_Errno
  { LZ_ok = 0,         LZ_bad_argument, LZ_mem_error,
    LZ_sequence_error, LZ_header_error, LZ_unexpected_eof,
    LZ_data_error,     LZ_library_error } LZ_Errno;


int LZ_api_version( void );				/* new in 1.12 */
const char * LZ_version( void );
const char * LZ_strerror( const LZ_Errno lz_errno );

int LZ_min_dictionary_bits( void );
int LZ_min_dictionary_size( void );
int LZ_max_dictionary_bits( void );
int LZ_max_dictionary_size( void );
int LZ_min_match_len_limit( void );
int LZ_max_match_len_limit( void );


/* --------------------- Compression Functions --------------------- */

typedef struct LZ_Encoder LZ_Encoder;

LZ_Encoder * LZ_compress_open( const int dictionary_size,
                               const int match_len_limit,
                               const unsigned long long member_size );
int LZ_compress_close( LZ_Encoder * const encoder );

int LZ_compress_finish( LZ_Encoder * const encoder );
int LZ_compress_restart_member( LZ_Encoder * const encoder,
                                const unsigned long long member_size );
int LZ_compress_sync_flush( LZ_Encoder * const encoder );

int LZ_compress_read( LZ_Encoder * const encoder,
                      uint8_t * const buffer, const int size );
int LZ_compress_write( LZ_Encoder * const encoder,
                       const uint8_t * const buffer, const int size );
int LZ_compress_write_size( LZ_Encoder * const encoder );

LZ_Errno LZ_compress_errno( LZ_Encoder * const encoder );
int LZ_compress_finished( LZ_Encoder * const encoder );
int LZ_compress_member_finished( LZ_Encoder * const encoder );

unsigned long long LZ_compress_data_position( LZ_Encoder * const encoder );
unsigned long long LZ_compress_member_position( LZ_Encoder * const encoder );
unsigned long long LZ_compress_total_in_size( LZ_Encoder * const encoder );
unsigned long long LZ_compress_total_out_size( LZ_Encoder * const encoder );


/* -------------------- Decompression Functions -------------------- */

typedef struct LZ_Decoder LZ_Decoder;

LZ_Decoder * LZ_decompress_open( void );
int LZ_decompress_close( LZ_Decoder * const decoder );

int LZ_decompress_finish( LZ_Decoder * const decoder );
int LZ_decompress_reset( LZ_Decoder * const decoder );
int LZ_decompress_sync_to_member( LZ_Decoder * const decoder );

int LZ_decompress_read( LZ_Decoder * const decoder,
                        uint8_t * const buffer, const int size );
int LZ_decompress_write( LZ_Decoder * const decoder,
                         const uint8_t * const buffer, const int size );
int LZ_decompress_write_size( LZ_Decoder * const decoder );

LZ_Errno LZ_decompress_errno( LZ_Decoder * const decoder );
int LZ_decompress_finished( LZ_Decoder * const decoder );
int LZ_decompress_member_finished( LZ_Decoder * const decoder );

int LZ_decompress_member_version( LZ_Decoder * const decoder );
int LZ_decompress_dictionary_size( LZ_Decoder * const decoder );
unsigned LZ_decompress_data_crc( LZ_Decoder * const decoder );

unsigned long long LZ_decompress_data_position( LZ_Decoder * const decoder );
unsigned long long LZ_decompress_member_position( LZ_Decoder * const decoder );
unsigned long long LZ_decompress_total_in_size( LZ_Decoder * const decoder );
unsigned long long LZ_decompress_total_out_size( LZ_Decoder * const decoder );

#ifdef __cplusplus
}
#endif

#ifndef max
  #define max(x,y) ((x) >= (y) ? (x) : (y))
#endif
#ifndef min
  #define min(x,y) ((x) <= (y) ? (x) : (y))
#endif

typedef int State;

enum { states = 12 };

static inline bool St_is_char( const State st ) { return st < 7; }

static inline State St_set_char( const State st )
  {
  static const State next[states] = { 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 4, 5 };
  return next[st];
  }
static inline State St_set_char_rep() { return 8; }
static inline State St_set_match( const State st )
  { return ( st < 7 ) ? 7 : 10; }
static inline State St_set_rep( const State st )
  { return ( st < 7 ) ? 8 : 11; }
static inline State St_set_shortrep( const State st )
  { return ( st < 7 ) ? 9 : 11; }


enum {
  min_dictionary_bits = 12,
  min_dictionary_size = 1 << min_dictionary_bits,	/* >= modeled_distances */
  max_dictionary_bits = 29,
  max_dictionary_size = 1 << max_dictionary_bits,
  literal_context_bits = 3,
  literal_pos_state_bits = 0,				/* not used */
  pos_state_bits = 2,
  pos_states = 1 << pos_state_bits,
  pos_state_mask = pos_states - 1,

  len_states = 4,
  dis_slot_bits = 6,
  start_dis_model = 4,
  end_dis_model = 14,
  modeled_distances = 1 << (end_dis_model / 2),		/* 128 */
  dis_align_bits = 4,
  dis_align_size = 1 << dis_align_bits,

  len_low_bits = 3,
  len_mid_bits = 3,
  len_high_bits = 8,
  len_low_symbols = 1 << len_low_bits,
  len_mid_symbols = 1 << len_mid_bits,
  len_high_symbols = 1 << len_high_bits,
  max_len_symbols = len_low_symbols + len_mid_symbols + len_high_symbols,

  min_match_len = 2,					/* must be 2 */
  max_match_len = min_match_len + max_len_symbols - 1,	/* 273 */
  min_match_len_limit = 5 };

static inline int get_len_state( const int len )
  { return min( len - min_match_len, len_states - 1 ); }

static inline int get_lit_state( const uint8_t prev_byte )
  { return prev_byte >> ( 8 - literal_context_bits ); }


enum { bit_model_move_bits = 5,
       bit_model_total_bits = 11,
       bit_model_total = 1 << bit_model_total_bits };

typedef int Bit_model;

static inline void Bm_init( Bit_model * const probability )
  { *probability = bit_model_total / 2; }

static inline void Bm_array_init( Bit_model bm[], const int size )
  { int i; for( i = 0; i < size; ++i ) Bm_init( &bm[i] ); }

typedef struct Len_model
  {
  Bit_model choice1;
  Bit_model choice2;
  Bit_model bm_low[pos_states][len_low_symbols];
  Bit_model bm_mid[pos_states][len_mid_symbols];
  Bit_model bm_high[len_high_symbols];
  } Len_model;

static inline void Lm_init( Len_model * const lm )
  {
  Bm_init( &lm->choice1 );
  Bm_init( &lm->choice2 );
  Bm_array_init( lm->bm_low[0], pos_states * len_low_symbols );
  Bm_array_init( lm->bm_mid[0], pos_states * len_mid_symbols );
  Bm_array_init( lm->bm_high, len_high_symbols );
  }


/* Table of CRCs of all 8-bit messages. */
static const uint32_t crc32[256] =
  {
  0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
  0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
  0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
  0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
  0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
  0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
  0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
  0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
  0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
  0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
  0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
  0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
  0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
  0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
  0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
  0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
  0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
  0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
  0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
  0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
  0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
  0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
  0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
  0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
  0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
  0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
  0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
  0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
  0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
  0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
  0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
  0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
  0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
  0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
  0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
  0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
  0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
  0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
  0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
  0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
  0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
  0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
  0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D };


static inline void CRC32_update_byte( uint32_t * const crc, const uint8_t byte )
  { *crc = crc32[(*crc^byte)&0xFF] ^ ( *crc >> 8 ); }

/* about as fast as it is possible without messing with endianness */
static inline void CRC32_update_buf( uint32_t * const crc,
                                     const uint8_t * const buffer,
                                     const int size )
  {
  uint32_t c = *crc;
  const uint8_t * ptr = buffer;
  const uint8_t * const endPtr = buffer + size;
  while( ptr < endPtr )
    { c = crc32[(c^*ptr)&0xFF] ^ ( c >> 8 ); ++ptr; }
  *crc = c;
  }


static inline bool isvalid_ds( const unsigned dictionary_size )
  { return dictionary_size >= min_dictionary_size &&
           dictionary_size <= max_dictionary_size; }


static inline int real_bits( unsigned value )
  {
  int bits = 0;
  while( value > 0 ) { value >>= 1; ++bits; }
  return bits;
  }


static const uint8_t lzip_magic[4] = { 0x4C, 0x5A, 0x49, 0x50 }; /* "LZIP" */

enum { Lh_size = 6 };
typedef uint8_t Lzip_header[Lh_size];	/* 0-3 magic bytes */
					/*   4 version */
					/*   5 coded dictionary size */

static inline void Lh_set_magic( Lzip_header data )
  { memcpy( data, lzip_magic, 4 ); data[4] = 1; }

static inline bool Lh_check_magic( const Lzip_header data )
  { return memcmp( data, lzip_magic, 4 ) == 0; }

/* detect (truncated) header */
static inline bool Lh_check_prefix( const Lzip_header data, const int sz )
  {
  int i; for( i = 0; i < sz && i < 4; ++i )
    if( data[i] != lzip_magic[i] ) return false;
  return sz > 0;
  }

/* detect corrupt header */
static inline bool Lh_check_corrupt( const Lzip_header data )
  {
  int matches = 0;
  int i; for( i = 0; i < 4; ++i )
    if( data[i] == lzip_magic[i] ) ++matches;
  return matches > 1 && matches < 4;
  }

static inline uint8_t Lh_version( const Lzip_header data )
  { return data[4]; }

static inline bool Lh_check_version( const Lzip_header data )
  { return data[4] == 1; }

static inline unsigned Lh_get_dictionary_size( const Lzip_header data )
  {
  unsigned sz = 1 << ( data[5] & 0x1F );
  if( sz > min_dictionary_size )
    sz -= ( sz / 16 ) * ( ( data[5] >> 5 ) & 7 );
  return sz;
  }

static inline bool Lh_set_dictionary_size( Lzip_header data, const unsigned sz )
  {
  if( !isvalid_ds( sz ) ) return false;
  data[5] = real_bits( sz - 1 );
  if( sz > min_dictionary_size )
    {
    const unsigned base_size = 1 << data[5];
    const unsigned fraction = base_size / 16;
    unsigned i;
    for( i = 7; i >= 1; --i )
      if( base_size - ( i * fraction ) >= sz )
        { data[5] |= i << 5; break; }
    }
  return true;
  }

static inline bool Lh_check( const Lzip_header data )
  {
  return Lh_check_magic( data ) && Lh_check_version( data ) &&
         isvalid_ds( Lh_get_dictionary_size( data ) );
  }


enum { Lt_size = 20 };
typedef uint8_t Lzip_trailer[Lt_size];
			/*  0-3  CRC32 of the uncompressed data */
			/*  4-11 size of the uncompressed data */
			/* 12-19 member size including header and trailer */

static inline unsigned Lt_get_data_crc( const Lzip_trailer data )
  {
  unsigned tmp = 0;
  int i; for( i = 3; i >= 0; --i ) { tmp <<= 8; tmp += data[i]; }
  return tmp;
  }

static inline void Lt_set_data_crc( Lzip_trailer data, unsigned crc )
  { int i; for( i = 0; i <= 3; ++i ) { data[i] = (uint8_t)crc; crc >>= 8; } }

static inline unsigned long long Lt_get_data_size( const Lzip_trailer data )
  {
  unsigned long long tmp = 0;
  int i; for( i = 11; i >= 4; --i ) { tmp <<= 8; tmp += data[i]; }
  return tmp;
  }

static inline void Lt_set_data_size( Lzip_trailer data, unsigned long long sz )
  { int i; for( i = 4; i <= 11; ++i ) { data[i] = (uint8_t)sz; sz >>= 8; } }

static inline unsigned long long Lt_get_member_size( const Lzip_trailer data )
  {
  unsigned long long tmp = 0;
  int i; for( i = 19; i >= 12; --i ) { tmp <<= 8; tmp += data[i]; }
  return tmp;
  }

static inline void Lt_set_member_size( Lzip_trailer data, unsigned long long sz )
  { int i; for( i = 12; i <= 19; ++i ) { data[i] = (uint8_t)sz; sz >>= 8; } }

typedef struct Circular_buffer
  {
  uint8_t * buffer;
  unsigned buffer_size;		/* capacity == buffer_size - 1 */
  unsigned get;			/* buffer is empty when get == put */
  unsigned put;
  } Circular_buffer;

static inline bool Cb_init( Circular_buffer * const cb,
                            const unsigned buf_size )
  {
  cb->buffer_size = buf_size + 1;
  cb->get = 0;
  cb->put = 0;
  cb->buffer =
    ( cb->buffer_size > 1 ) ? (uint8_t *)malloc( cb->buffer_size ) : 0;
  return cb->buffer != 0;
  }

static inline void Cb_free( Circular_buffer * const cb )
  { free( cb->buffer ); cb->buffer = 0; }

static inline void Cb_reset( Circular_buffer * const cb )
  { cb->get = 0; cb->put = 0; }

static inline unsigned Cb_empty( const Circular_buffer * const cb )
  { return cb->get == cb->put; }

static inline unsigned Cb_used_bytes( const Circular_buffer * const cb )
  { return ( (cb->get <= cb->put) ? 0 : cb->buffer_size ) + cb->put - cb->get; }

static inline unsigned Cb_free_bytes( const Circular_buffer * const cb )
  { return ( (cb->get <= cb->put) ? cb->buffer_size : 0 ) - cb->put + cb->get - 1; }

static inline uint8_t Cb_get_byte( Circular_buffer * const cb )
  {
  const uint8_t b = cb->buffer[cb->get];
  if( ++cb->get >= cb->buffer_size ) cb->get = 0;
  return b;
  }

static inline void Cb_put_byte( Circular_buffer * const cb, const uint8_t b )
  {
  cb->buffer[cb->put] = b;
  if( ++cb->put >= cb->buffer_size ) cb->put = 0;
  }


static bool Cb_unread_data( Circular_buffer * const cb, const unsigned size )
  {
  if( size > Cb_free_bytes( cb ) ) return false;
  if( cb->get >= size ) cb->get -= size;
  else cb->get = cb->buffer_size - size + cb->get;
  return true;
  }


/* Copy up to 'out_size' bytes to 'out_buffer' and update 'get'.
   If 'out_buffer' is null, the bytes are discarded.
   Return the number of bytes copied or discarded.
*/
static unsigned Cb_read_data( Circular_buffer * const cb,
                              uint8_t * const out_buffer,
                              const unsigned out_size )
  {
  unsigned size = 0;
  if( out_size == 0 ) return 0;
  if( cb->get > cb->put )
    {
    size = min( cb->buffer_size - cb->get, out_size );
    if( size > 0 )
      {
      if( out_buffer ) memcpy( out_buffer, cb->buffer + cb->get, size );
      cb->get += size;
      if( cb->get >= cb->buffer_size ) cb->get = 0;
      }
    }
  if( cb->get < cb->put )
    {
    const unsigned size2 = min( cb->put - cb->get, out_size - size );
    if( size2 > 0 )
      {
      if( out_buffer ) memcpy( out_buffer + size, cb->buffer + cb->get, size2 );
      cb->get += size2;
      size += size2;
      }
    }
  return size;
  }


/* Copy up to 'in_size' bytes from 'in_buffer' and update 'put'.
   Return the number of bytes copied.
*/
static unsigned Cb_write_data( Circular_buffer * const cb,
                               const uint8_t * const in_buffer,
                               const unsigned in_size )
  {
  unsigned size = 0;
  if( in_size == 0 ) return 0;
  if( cb->put >= cb->get )
    {
    size = min( cb->buffer_size - cb->put - (cb->get == 0), in_size );
    if( size > 0 )
      {
      memcpy( cb->buffer + cb->put, in_buffer, size );
      cb->put += size;
      if( cb->put >= cb->buffer_size ) cb->put = 0;
      }
    }
  if( cb->put < cb->get )
    {
    const unsigned size2 = min( cb->get - cb->put - 1, in_size - size );
    if( size2 > 0 )
      {
      memcpy( cb->buffer + cb->put, in_buffer + size, size2 );
      cb->put += size2;
      size += size2;
      }
    }
  return size;
  }

enum { rd_min_available_bytes = 11 };

typedef struct Range_decoder
  {
  Circular_buffer cb;			/* input buffer */
  unsigned long long member_position;
  uint32_t code;
  uint32_t range;
  bool at_stream_end;
  bool reload_pending;
  } Range_decoder;

static inline bool Rd_init( Range_decoder * const rdec )
  {
  if( !Cb_init( &rdec->cb, 65536 + rd_min_available_bytes ) ) return false;
  rdec->member_position = 0;
  rdec->code = 0;
  rdec->range = 0xFFFFFFFFU;
  rdec->at_stream_end = false;
  rdec->reload_pending = false;
  return true;
  }

static inline void Rd_free( Range_decoder * const rdec )
  { Cb_free( &rdec->cb ); }

static inline bool Rd_finished( const Range_decoder * const rdec )
  { return rdec->at_stream_end && Cb_empty( &rdec->cb ); }

static inline void Rd_finish( Range_decoder * const rdec )
  { rdec->at_stream_end = true; }

static inline bool Rd_enough_available_bytes( const Range_decoder * const rdec )
  { return Cb_used_bytes( &rdec->cb ) >= rd_min_available_bytes; }

static inline unsigned Rd_available_bytes( const Range_decoder * const rdec )
  { return Cb_used_bytes( &rdec->cb ); }

static inline unsigned Rd_free_bytes( const Range_decoder * const rdec )
  { return rdec->at_stream_end ? 0 : Cb_free_bytes( &rdec->cb ); }

static inline unsigned long long Rd_purge( Range_decoder * const rdec )
  {
  const unsigned long long size =
    rdec->member_position + Cb_used_bytes( &rdec->cb );
  Cb_reset( &rdec->cb );
  rdec->member_position = 0; rdec->at_stream_end = true;
  return size;
  }

static inline void Rd_reset( Range_decoder * const rdec )
  { Cb_reset( &rdec->cb );
    rdec->member_position = 0; rdec->at_stream_end = false; }


/* Seek for a member header and update 'get'. Set '*skippedp' to the number
   of bytes skipped. Return true if a valid header is found.
*/
static bool Rd_find_header( Range_decoder * const rdec,
                            unsigned * const skippedp )
  {
  *skippedp = 0;
  while( rdec->cb.get != rdec->cb.put )
    {
    if( rdec->cb.buffer[rdec->cb.get] == lzip_magic[0] )
      {
      unsigned get = rdec->cb.get;
      int i;
      Lzip_header header;
      for( i = 0; i < Lh_size; ++i )
        {
        if( get == rdec->cb.put ) return false;		/* not enough data */
        header[i] = rdec->cb.buffer[get];
        if( ++get >= rdec->cb.buffer_size ) get = 0;
        }
      if( Lh_check( header ) ) return true;
      }
    if( ++rdec->cb.get >= rdec->cb.buffer_size ) rdec->cb.get = 0;
    ++*skippedp;
    }
  return false;
  }


static inline int Rd_write_data( Range_decoder * const rdec,
                                 const uint8_t * const inbuf, const int size )
  {
  if( rdec->at_stream_end || size <= 0 ) return 0;
  return Cb_write_data( &rdec->cb, inbuf, size );
  }

static inline uint8_t Rd_get_byte( Range_decoder * const rdec )
  {
  /* 0xFF avoids decoder error if member is truncated at EOS marker */
  if( Rd_finished( rdec ) ) return 0xFF;
  ++rdec->member_position;
  return Cb_get_byte( &rdec->cb );
  }

static inline int Rd_read_data( Range_decoder * const rdec,
                                uint8_t * const outbuf, const int size )
  {
  const int sz = Cb_read_data( &rdec->cb, outbuf, size );
  if( sz > 0 ) rdec->member_position += sz;
  return sz;
  }

static inline bool Rd_unread_data( Range_decoder * const rdec,
                                   const unsigned size )
  {
  if( size > rdec->member_position || !Cb_unread_data( &rdec->cb, size ) )
    return false;
  rdec->member_position -= size;
  return true;
  }

static int Rd_try_reload( Range_decoder * const rdec )
  {
  if( rdec->reload_pending && Rd_available_bytes( rdec ) >= 5 )
    {
    rdec->reload_pending = false;
    rdec->code = 0;
    rdec->range = 0xFFFFFFFFU;
    /* check first byte of the LZMA stream without reading it */
    if( rdec->cb.buffer[rdec->cb.get] != 0 ) return 2;
    Rd_get_byte( rdec );	/* discard first byte of the LZMA stream */
    int i; for( i = 0; i < 4; ++i )
      rdec->code = (rdec->code << 8) | Rd_get_byte( rdec );
    }
  return !rdec->reload_pending;
  }

static inline void Rd_normalize( Range_decoder * const rdec )
  {
  if( rdec->range <= 0x00FFFFFFU )
    { rdec->range <<= 8; rdec->code = (rdec->code << 8) | Rd_get_byte( rdec ); }
  }

static inline unsigned Rd_decode( Range_decoder * const rdec, int num_bits )
  {
  unsigned symbol = 0;
  do {
    Rd_normalize( rdec );
    rdec->range >>= 1;
/*    symbol <<= 1; */
/*    if( rdec->code >= rdec->range ) { rdec->code -= rdec->range; symbol |= 1; } */
    const bool bit = rdec->code >= rdec->range;
    symbol <<= 1; symbol += bit;
    rdec->code -= rdec->range & ( 0U - bit );
    } while( --num_bits > 0 );
  return symbol;
  }

static inline unsigned Rd_decode_bit( Range_decoder * const rdec,
                                      Bit_model * const probability )
  {
  Rd_normalize( rdec );
  const uint32_t bound = ( rdec->range >> bit_model_total_bits ) * *probability;
  if( rdec->code < bound )
    {
    rdec->range = bound;
    *probability += ( bit_model_total - *probability ) >> bit_model_move_bits;
    return 0;
    }
  else
    {
    rdec->code -= bound;
    rdec->range -= bound;
    *probability -= *probability >> bit_model_move_bits;
    return 1;
    }
  }

static inline void Rd_decode_symbol_bit( Range_decoder * const rdec,
                         Bit_model * const probability, unsigned * symbol )
  {
  Rd_normalize( rdec );
  *symbol <<= 1;
  const uint32_t bound = ( rdec->range >> bit_model_total_bits ) * *probability;
  if( rdec->code < bound )
    {
    rdec->range = bound;
    *probability += ( bit_model_total - *probability ) >> bit_model_move_bits;
    }
  else
    {
    rdec->code -= bound;
    rdec->range -= bound;
    *probability -= *probability >> bit_model_move_bits;
    *symbol |= 1;
    }
  }

static inline void Rd_decode_symbol_bit_reversed( Range_decoder * const rdec,
                         Bit_model * const probability, unsigned * model,
                         unsigned * symbol, const int i )
  {
  Rd_normalize( rdec );
  *model <<= 1;
  const uint32_t bound = ( rdec->range >> bit_model_total_bits ) * *probability;
  if( rdec->code < bound )
    {
    rdec->range = bound;
    *probability += ( bit_model_total - *probability ) >> bit_model_move_bits;
    }
  else
    {
    rdec->code -= bound;
    rdec->range -= bound;
    *probability -= *probability >> bit_model_move_bits;
    *model |= 1;
    *symbol |= 1 << i;
    }
  }

static inline unsigned Rd_decode_tree6( Range_decoder * const rdec,
                                        Bit_model bm[] )
  {
  unsigned symbol = 1;
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  return symbol & 0x3F;
  }

static inline unsigned Rd_decode_tree8( Range_decoder * const rdec,
                                        Bit_model bm[] )
  {
  unsigned symbol = 1;
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  return symbol & 0xFF;
  }

static inline unsigned
Rd_decode_tree_reversed( Range_decoder * const rdec,
                         Bit_model bm[], const int num_bits )
  {
  unsigned model = 1;
  unsigned symbol = 0;
  int i;
  for( i = 0; i < num_bits; ++i )
    Rd_decode_symbol_bit_reversed( rdec, &bm[model], &model, &symbol, i );
  return symbol;
  }

static inline unsigned
Rd_decode_tree_reversed4( Range_decoder * const rdec, Bit_model bm[] )
  {
  unsigned model = 1;
  unsigned symbol = 0;
  Rd_decode_symbol_bit_reversed( rdec, &bm[model], &model, &symbol, 0 );
  Rd_decode_symbol_bit_reversed( rdec, &bm[model], &model, &symbol, 1 );
  Rd_decode_symbol_bit_reversed( rdec, &bm[model], &model, &symbol, 2 );
  Rd_decode_symbol_bit_reversed( rdec, &bm[model], &model, &symbol, 3 );
  return symbol;
  }

static inline unsigned Rd_decode_matched( Range_decoder * const rdec,
                                          Bit_model bm[], unsigned match_byte )
  {
  unsigned symbol = 1;
  unsigned mask = 0x100;
  while( true )
    {
    const unsigned match_bit = ( match_byte <<= 1 ) & mask;
    const unsigned bit = Rd_decode_bit( rdec, &bm[symbol+match_bit+mask] );
    symbol <<= 1; symbol += bit;
    if( symbol > 0xFF ) return symbol & 0xFF;
    mask &= ~(match_bit ^ (bit << 8));	/* if( match_bit != bit ) mask = 0; */
    }
  }

static inline unsigned Rd_decode_len( Range_decoder * const rdec,
                                      Len_model * const lm,
                                      const int pos_state )
  {
  Bit_model * bm;
  unsigned mask, offset, symbol = 1;

  if( Rd_decode_bit( rdec, &lm->choice1 ) == 0 )
    { bm = lm->bm_low[pos_state]; mask = 7; offset = 0; goto len3; }
  if( Rd_decode_bit( rdec, &lm->choice2 ) == 0 )
    { bm = lm->bm_mid[pos_state]; mask = 7; offset = len_low_symbols; goto len3; }
  bm = lm->bm_high; mask = 0xFF; offset = len_low_symbols + len_mid_symbols;
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
len3:
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  Rd_decode_symbol_bit( rdec, &bm[symbol], &symbol );
  return ( symbol & mask ) + min_match_len + offset;
  }


enum { lzd_min_free_bytes = max_match_len };

typedef struct LZ_decoder
  {
  Circular_buffer cb;
  unsigned long long partial_data_pos;
  Range_decoder * rdec;
  unsigned dictionary_size;
  uint32_t crc;
  bool check_trailer_pending;
  bool member_finished;
  bool pos_wrapped;
  unsigned dis0;		/* dis[0-3] latest four distances */
  unsigned dis1;		/* used for efficient coding of */
  unsigned dis2;		/* repeated distances */
  unsigned dis3;
  State state;

  Bit_model bm_literal[1<<literal_context_bits][0x300];
  Bit_model bm_match[states][pos_states];
  Bit_model bm_rep[states];
  Bit_model bm_rep0[states];
  Bit_model bm_rep1[states];
  Bit_model bm_rep2[states];
  Bit_model bm_len[states][pos_states];
  Bit_model bm_dis_slot[len_states][1<<dis_slot_bits];
  Bit_model bm_dis[modeled_distances-end_dis_model+1];
  Bit_model bm_align[dis_align_size];

  Len_model match_len_model;
  Len_model rep_len_model;
  } LZ_decoder;

static inline bool LZd_enough_free_bytes( const LZ_decoder * const d )
  { return Cb_free_bytes( &d->cb ) >= lzd_min_free_bytes; }

static inline uint8_t LZd_peek_prev( const LZ_decoder * const d )
  { return d->cb.buffer[((d->cb.put > 0) ? d->cb.put : d->cb.buffer_size)-1]; }

static inline uint8_t LZd_peek( const LZ_decoder * const d,
                                const unsigned distance )
  {
  const unsigned i = ( (d->cb.put > distance) ? 0 : d->cb.buffer_size ) +
                     d->cb.put - distance - 1;
  return d->cb.buffer[i];
  }

static inline void LZd_put_byte( LZ_decoder * const d, const uint8_t b )
  {
  CRC32_update_byte( &d->crc, b );
  d->cb.buffer[d->cb.put] = b;
  if( ++d->cb.put >= d->cb.buffer_size )
    { d->partial_data_pos += d->cb.put; d->cb.put = 0; d->pos_wrapped = true; }
  }

static inline void LZd_copy_block( LZ_decoder * const d,
                                   const unsigned distance, unsigned len )
  {
  unsigned lpos = d->cb.put, i;
  bool fast, fast2;
  if( lpos > distance )
    {
    i = lpos - distance - 1;
    fast = len < d->cb.buffer_size - lpos;
    fast2 = fast && len <= lpos - i;
    }
  else
    {
    i = d->cb.buffer_size + lpos - distance - 1;
    fast = len < d->cb.buffer_size - i;		/* (i == pos) may happen */
    fast2 = fast && len <= i - lpos;
    }
  if( fast )					/* no wrap */
    {
    const unsigned tlen = len;
    if( fast2 )					/* no wrap, no overlap */
      memcpy( d->cb.buffer + lpos, d->cb.buffer + i, len );
    else
      for( ; len > 0; --len ) d->cb.buffer[lpos++] = d->cb.buffer[i++];
    CRC32_update_buf( &d->crc, d->cb.buffer + d->cb.put, tlen );
    d->cb.put += tlen;
    }
  else for( ; len > 0; --len )
    {
    LZd_put_byte( d, d->cb.buffer[i] );
    if( ++i >= d->cb.buffer_size ) i = 0;
    }
  }

static inline bool LZd_init( LZ_decoder * const d, Range_decoder * const rde,
                             const unsigned dict_size )
  {
  if( !Cb_init( &d->cb, max( 65536, dict_size ) + lzd_min_free_bytes ) )
    return false;
  d->partial_data_pos = 0;
  d->rdec = rde;
  d->dictionary_size = dict_size;
  d->crc = 0xFFFFFFFFU;
  d->check_trailer_pending = false;
  d->member_finished = false;
  d->pos_wrapped = false;
  d->cb.buffer[d->cb.buffer_size-1] = 0;	/* prev_byte of first byte */
  d->dis0 = 0;
  d->dis1 = 0;
  d->dis2 = 0;
  d->dis3 = 0;
  d->state = 0;

  Bm_array_init( d->bm_literal[0], (1 << literal_context_bits) * 0x300 );
  Bm_array_init( d->bm_match[0], states * pos_states );
  Bm_array_init( d->bm_rep, states );
  Bm_array_init( d->bm_rep0, states );
  Bm_array_init( d->bm_rep1, states );
  Bm_array_init( d->bm_rep2, states );
  Bm_array_init( d->bm_len[0], states * pos_states );
  Bm_array_init( d->bm_dis_slot[0], len_states * (1 << dis_slot_bits) );
  Bm_array_init( d->bm_dis, modeled_distances - end_dis_model + 1 );
  Bm_array_init( d->bm_align, dis_align_size );
  Lm_init( &d->match_len_model );
  Lm_init( &d->rep_len_model );
  return true;
  }

static inline void LZd_free( LZ_decoder * const d ) { Cb_free( &d->cb ); }

static inline bool LZd_member_finished( const LZ_decoder * const d )
  { return d->member_finished && Cb_empty( &d->cb ); }

static inline unsigned LZd_crc( const LZ_decoder * const d )
  { return d->crc ^ 0xFFFFFFFFU; }

static inline unsigned long long
LZd_data_position( const LZ_decoder * const d )
  { return d->partial_data_pos + d->cb.put; }

static int LZd_try_check_trailer( LZ_decoder * const d )
  {
  Lzip_trailer trailer;
  if( Rd_available_bytes( d->rdec ) < Lt_size )
    { if( !d->rdec->at_stream_end ) return 0; else return 2; }
  d->check_trailer_pending = false;
  d->member_finished = true;

  if( Rd_read_data( d->rdec, trailer, Lt_size ) == Lt_size &&
      Lt_get_data_crc( trailer ) == LZd_crc( d ) &&
      Lt_get_data_size( trailer ) == LZd_data_position( d ) &&
      Lt_get_member_size( trailer ) == d->rdec->member_position ) return 0;
  return 3;
  }


/* Return value: 0 = OK, 1 = decoder error, 2 = unexpected EOF,
                 3 = trailer error, 4 = unknown marker found,
                 5 = nonzero first LZMA byte found, 6 = library error. */
static int LZd_decode_member( LZ_decoder * const d )
  {
  Range_decoder * const rdec = d->rdec;
  State * const state = &d->state;
  unsigned old_mpos = rdec->member_position;

  if( d->member_finished ) return 0;
  const int tmp = Rd_try_reload( rdec );
  if( tmp > 1 ) return 5;
  if( !tmp ) { if( !rdec->at_stream_end ) return 0; else return 2; }
  if( d->check_trailer_pending ) return LZd_try_check_trailer( d );

  while( !Rd_finished( rdec ) )
    {
    const unsigned mpos = rdec->member_position;
    if( mpos - old_mpos > rd_min_available_bytes ) return 6;
    old_mpos = mpos;
    if( !Rd_enough_available_bytes( rdec ) )	/* check unexpected EOF */
      { if( !rdec->at_stream_end ) return 0;
        if( Cb_empty( &rdec->cb ) ) break; }	/* decode until EOF */
    if( !LZd_enough_free_bytes( d ) ) return 0;
    const int pos_state = LZd_data_position( d ) & pos_state_mask;
    if( Rd_decode_bit( rdec, &d->bm_match[*state][pos_state] ) == 0 ) /* 1st bit */
      {
      /* literal byte */
      Bit_model * const bm = d->bm_literal[get_lit_state(LZd_peek_prev( d ))];
      if( ( *state = St_set_char( *state ) ) < 4 )
        LZd_put_byte( d, Rd_decode_tree8( rdec, bm ) );
      else
        LZd_put_byte( d, Rd_decode_matched( rdec, bm, LZd_peek( d, d->dis0 ) ) );
      continue;
      }
    /* match or repeated match */
    int len;
    if( Rd_decode_bit( rdec, &d->bm_rep[*state] ) != 0 )	/* 2nd bit */
      {
      if( Rd_decode_bit( rdec, &d->bm_rep0[*state] ) == 0 )	/* 3rd bit */
        {
        if( Rd_decode_bit( rdec, &d->bm_len[*state][pos_state] ) == 0 )	/* 4th bit */
          { *state = St_set_shortrep( *state );
            LZd_put_byte( d, LZd_peek( d, d->dis0 ) ); continue; }
        }
      else
        {
        unsigned distance;
        if( Rd_decode_bit( rdec, &d->bm_rep1[*state] ) == 0 )	/* 4th bit */
          distance = d->dis1;
        else
          {
          if( Rd_decode_bit( rdec, &d->bm_rep2[*state] ) == 0 )	/* 5th bit */
            distance = d->dis2;
          else
            { distance = d->dis3; d->dis3 = d->dis2; }
          d->dis2 = d->dis1;
          }
        d->dis1 = d->dis0;
        d->dis0 = distance;
        }
      *state = St_set_rep( *state );
      len = Rd_decode_len( rdec, &d->rep_len_model, pos_state );
      }
    else					/* match */
      {
      len = Rd_decode_len( rdec, &d->match_len_model, pos_state );
      unsigned dis0 = Rd_decode_tree6( rdec, d->bm_dis_slot[get_len_state(len)] );
      if( dis0 >= start_dis_model )
        {
        const unsigned dis_slot = dis0;
        const int direct_bits = ( dis_slot >> 1 ) - 1;
        dis0 = ( 2 | ( dis_slot & 1 ) ) << direct_bits;
        if( dis_slot < end_dis_model )
          dis0 += Rd_decode_tree_reversed( rdec, d->bm_dis + ( dis0 - dis_slot ),
                                           direct_bits );
        else
          {
          dis0 += Rd_decode( rdec, direct_bits - dis_align_bits ) << dis_align_bits;
          dis0 += Rd_decode_tree_reversed4( rdec, d->bm_align );
          if( dis0 == 0xFFFFFFFFU )		/* marker found */
            {
            Rd_normalize( rdec );
            const unsigned mpos = rdec->member_position;
            if( mpos - old_mpos > rd_min_available_bytes ) return 6;
            old_mpos = mpos;
            if( len == min_match_len )		/* End Of Stream marker */
              {
              d->check_trailer_pending = true;
              return LZd_try_check_trailer( d );
              }
            if( len == min_match_len + 1 )	/* Sync Flush marker */
              {
              rdec->reload_pending = true;
              const int tmp = Rd_try_reload( rdec );
              if( tmp > 1 ) return 5;
              if( tmp ) continue;
              if( !rdec->at_stream_end ) return 0; else break;
              }
            return 4;
            }
          }
        }
      d->dis3 = d->dis2; d->dis2 = d->dis1; d->dis1 = d->dis0; d->dis0 = dis0;
      if( dis0 >= d->dictionary_size ||
          ( dis0 >= d->cb.put && !d->pos_wrapped ) ) return 1;
      *state = St_set_match( *state );
      }
    LZd_copy_block( d, d->dis0, len );
    }
  return 2;
  }

enum { price_shift_bits = 6,
       price_step_bits = 2 };

static const uint8_t dis_slots[1<<10] =
  {
   0,  1,  2,  3,  4,  4,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,
   8,  8,  8,  8,  8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9,  9,
  10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
  11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
  14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
  14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
  14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
  14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19 };

static inline uint8_t get_slot( const unsigned dis )
  {
  if( dis < (1 << 10) ) return dis_slots[dis];
  if( dis < (1 << 19) ) return dis_slots[dis>> 9] + 18;
  if( dis < (1 << 28) ) return dis_slots[dis>>18] + 36;
  return dis_slots[dis>>27] + 54;
  }


static const short prob_prices[bit_model_total >> price_step_bits] =
{
640, 539, 492, 461, 438, 419, 404, 390, 379, 369, 359, 351, 343, 336, 330, 323,
318, 312, 307, 302, 298, 293, 289, 285, 281, 277, 274, 270, 267, 264, 261, 258,
255, 252, 250, 247, 244, 242, 239, 237, 235, 232, 230, 228, 226, 224, 222, 220,
218, 216, 214, 213, 211, 209, 207, 206, 204, 202, 201, 199, 198, 196, 195, 193,
192, 190, 189, 188, 186, 185, 184, 182, 181, 180, 178, 177, 176, 175, 174, 172,
171, 170, 169, 168, 167, 166, 165, 164, 163, 162, 161, 159, 158, 157, 157, 156,
155, 154, 153, 152, 151, 150, 149, 148, 147, 146, 145, 145, 144, 143, 142, 141,
140, 140, 139, 138, 137, 136, 136, 135, 134, 133, 133, 132, 131, 130, 130, 129,
128, 127, 127, 126, 125, 125, 124, 123, 123, 122, 121, 121, 120, 119, 119, 118,
117, 117, 116, 115, 115, 114, 114, 113, 112, 112, 111, 111, 110, 109, 109, 108,
108, 107, 106, 106, 105, 105, 104, 104, 103, 103, 102, 101, 101, 100, 100,  99,
 99,  98,  98,  97,  97,  96,  96,  95,  95,  94,  94,  93,  93,  92,  92,  91,
 91,  90,  90,  89,  89,  88,  88,  88,  87,  87,  86,  86,  85,  85,  84,  84,
 83,  83,  83,  82,  82,  81,  81,  80,  80,  80,  79,  79,  78,  78,  77,  77,
 77,  76,  76,  75,  75,  75,  74,  74,  73,  73,  73,  72,  72,  71,  71,  71,
 70,  70,  70,  69,  69,  68,  68,  68,  67,  67,  67,  66,  66,  65,  65,  65,
 64,  64,  64,  63,  63,  63,  62,  62,  61,  61,  61,  60,  60,  60,  59,  59,
 59,  58,  58,  58,  57,  57,  57,  56,  56,  56,  55,  55,  55,  54,  54,  54,
 53,  53,  53,  53,  52,  52,  52,  51,  51,  51,  50,  50,  50,  49,  49,  49,
 48,  48,  48,  48,  47,  47,  47,  46,  46,  46,  45,  45,  45,  45,  44,  44,
 44,  43,  43,  43,  43,  42,  42,  42,  41,  41,  41,  41,  40,  40,  40,  40,
 39,  39,  39,  38,  38,  38,  38,  37,  37,  37,  37,  36,  36,  36,  35,  35,
 35,  35,  34,  34,  34,  34,  33,  33,  33,  33,  32,  32,  32,  32,  31,  31,
 31,  31,  30,  30,  30,  30,  29,  29,  29,  29,  28,  28,  28,  28,  27,  27,
 27,  27,  26,  26,  26,  26,  26,  25,  25,  25,  25,  24,  24,  24,  24,  23,
 23,  23,  23,  22,  22,  22,  22,  22,  21,  21,  21,  21,  20,  20,  20,  20,
 20,  19,  19,  19,  19,  18,  18,  18,  18,  18,  17,  17,  17,  17,  17,  16,
 16,  16,  16,  15,  15,  15,  15,  15,  14,  14,  14,  14,  14,  13,  13,  13,
 13,  13,  12,  12,  12,  12,  12,  11,  11,  11,  11,  10,  10,  10,  10,  10,
  9,   9,   9,   9,   9,   9,   8,   8,   8,   8,   8,   7,   7,   7,   7,   7,
  6,   6,   6,   6,   6,   5,   5,   5,   5,   5,   4,   4,   4,   4,   4,   4,
  3,   3,   3,   3,   3,   2,   2,   2,   2,   2,   1,   1,   1,   1,   1,   1 };

static inline int get_price( const int probability )
  { return prob_prices[probability >> price_step_bits]; }


static inline int price0( const Bit_model probability )
  { return get_price( probability ); }

static inline int price1( const Bit_model probability )
  { return get_price( bit_model_total - probability ); }

static inline int price_bit( const Bit_model bm, const bool bit )
  { return bit ? price1( bm ) : price0( bm ); }


static inline int price_symbol3( const Bit_model bm[], int symbol )
  {
  bool bit = symbol & 1;
  symbol |= 8; symbol >>= 1;
  int price = price_bit( bm[symbol], bit );
  bit = symbol & 1; symbol >>= 1; price += price_bit( bm[symbol], bit );
  return price + price_bit( bm[1], symbol & 1 );
  }


static inline int price_symbol6( const Bit_model bm[], unsigned symbol )
  {
  bool bit = symbol & 1;
  symbol |= 64; symbol >>= 1;
  int price = price_bit( bm[symbol], bit );
  bit = symbol & 1; symbol >>= 1; price += price_bit( bm[symbol], bit );
  bit = symbol & 1; symbol >>= 1; price += price_bit( bm[symbol], bit );
  bit = symbol & 1; symbol >>= 1; price += price_bit( bm[symbol], bit );
  bit = symbol & 1; symbol >>= 1; price += price_bit( bm[symbol], bit );
  return price + price_bit( bm[1], symbol & 1 );
  }


static inline int price_symbol8( const Bit_model bm[], int symbol )
  {
  bool bit = symbol & 1;
  symbol |= 0x100; symbol >>= 1;
  int price = price_bit( bm[symbol], bit );
  bit = symbol & 1; symbol >>= 1; price += price_bit( bm[symbol], bit );
  bit = symbol & 1; symbol >>= 1; price += price_bit( bm[symbol], bit );
  bit = symbol & 1; symbol >>= 1; price += price_bit( bm[symbol], bit );
  bit = symbol & 1; symbol >>= 1; price += price_bit( bm[symbol], bit );
  bit = symbol & 1; symbol >>= 1; price += price_bit( bm[symbol], bit );
  bit = symbol & 1; symbol >>= 1; price += price_bit( bm[symbol], bit );
  return price + price_bit( bm[1], symbol & 1 );
  }


static inline int price_symbol_reversed( const Bit_model bm[], int symbol,
                                         const int num_bits )
  {
  int price = 0;
  int model = 1;
  int i;
  for( i = num_bits; i > 0; --i )
    {
    const bool bit = symbol & 1;
    symbol >>= 1;
    price += price_bit( bm[model], bit );
    model <<= 1; model |= bit;
    }
  return price;
  }


static inline int price_matched( const Bit_model bm[], unsigned symbol,
                                 unsigned match_byte )
  {
  int price = 0;
  unsigned mask = 0x100;
  symbol |= mask;
  while( true )
    {
    const unsigned match_bit = ( match_byte <<= 1 ) & mask;
    const bool bit = ( symbol <<= 1 ) & 0x100;
    price += price_bit( bm[(symbol>>9)+match_bit+mask], bit );
    if( symbol >= 0x10000 ) return price;
    mask &= ~(match_bit ^ symbol);	/* if( match_bit != bit ) mask = 0; */
    }
  }


typedef struct Matchfinder_base
  {
  unsigned long long partial_data_pos;
  uint8_t * buffer;		/* input buffer */
  int32_t * prev_positions;	/* 1 + last seen position of key. else 0 */
  int32_t * pos_array;		/* may be tree or chain */
  int before_size;		/* bytes to keep in buffer before dictionary */
  int after_size;		/* bytes to keep in buffer after pos */
  int buffer_size;
  int dictionary_size;		/* bytes to keep in buffer before pos */
  int pos;			/* current pos in buffer */
  int cyclic_pos;		/* cycles through [0, dictionary_size] */
  int stream_pos;		/* first byte not yet read from file */
  int pos_limit;		/* when reached, a new block must be read */
  int key4_mask;
  int num_prev_positions23;
  int num_prev_positions;	/* size of prev_positions */
  int pos_array_size;
  int saved_dictionary_size;	/* dictionary_size restored by Mb_reset */
  bool at_stream_end;		/* stream_pos shows real end of file */
  bool sync_flush_pending;
  } Matchfinder_base;

static bool Mb_normalize_pos( Matchfinder_base * const mb );

static bool Mb_init( Matchfinder_base * const mb, const int before_size,
                     const int dict_size, const int after_size,
                     const int dict_factor, const int num_prev_positions23,
                     const int pos_array_factor );

static inline void Mb_free( Matchfinder_base * const mb )
  { free( mb->prev_positions ); free( mb->buffer ); }

static inline uint8_t Mb_peek( const Matchfinder_base * const mb,
                               const int distance )
  { return mb->buffer[mb->pos-distance]; }

static inline int Mb_available_bytes( const Matchfinder_base * const mb )
  { return mb->stream_pos - mb->pos; }

static inline unsigned long long
Mb_data_position( const Matchfinder_base * const mb )
  { return mb->partial_data_pos + mb->pos; }

static inline void Mb_finish( Matchfinder_base * const mb )
  { mb->at_stream_end = true; mb->sync_flush_pending = false; }

static inline bool Mb_data_finished( const Matchfinder_base * const mb )
  { return mb->at_stream_end && mb->pos >= mb->stream_pos; }

static inline bool Mb_flushing_or_end( const Matchfinder_base * const mb )
  { return mb->at_stream_end || mb->sync_flush_pending; }

static inline int Mb_free_bytes( const Matchfinder_base * const mb )
  { if( Mb_flushing_or_end( mb ) ) return 0;
    return mb->buffer_size - mb->stream_pos; }

static inline bool
Mb_enough_available_bytes( const Matchfinder_base * const mb )
  { return mb->pos + mb->after_size <= mb->stream_pos ||
           ( Mb_flushing_or_end( mb ) && mb->pos < mb->stream_pos ); }

static inline const uint8_t *
Mb_ptr_to_current_pos( const Matchfinder_base * const mb )
  { return mb->buffer + mb->pos; }

static int Mb_write_data( Matchfinder_base * const mb,
                          const uint8_t * const inbuf, const int size )
  {
  const int sz = min( mb->buffer_size - mb->stream_pos, size );
  if( Mb_flushing_or_end( mb ) || sz <= 0 ) return 0;
  memcpy( mb->buffer + mb->stream_pos, inbuf, sz );
  mb->stream_pos += sz;
  return sz;
  }

static inline int Mb_true_match_len( const Matchfinder_base * const mb,
                                     const int index, const int distance )
  {
  const uint8_t * const data = mb->buffer + mb->pos;
  int i = index;
  const int len_limit = min( Mb_available_bytes( mb ), max_match_len );
  while( i < len_limit && data[i-distance] == data[i] ) ++i;
  return i;
  }

static inline bool Mb_move_pos( Matchfinder_base * const mb )
  {
  if( ++mb->cyclic_pos > mb->dictionary_size ) mb->cyclic_pos = 0;
  if( ++mb->pos >= mb->pos_limit ) return Mb_normalize_pos( mb );
  return true;
  }


typedef struct Range_encoder
  {
  Circular_buffer cb;
  unsigned min_free_bytes;
  uint64_t low;
  unsigned long long partial_member_pos;
  uint32_t range;
  unsigned ff_count;
  uint8_t cache;
  Lzip_header header;
  } Range_encoder;

static inline void Re_shift_low( Range_encoder * const renc )
  {
  if( renc->low >> 24 != 0xFF )
    {
    const bool carry = renc->low > 0xFFFFFFFFU;
    Cb_put_byte( &renc->cb, renc->cache + carry );
    for( ; renc->ff_count > 0; --renc->ff_count )
      Cb_put_byte( &renc->cb, 0xFF + carry );
    renc->cache = renc->low >> 24;
    }
  else ++renc->ff_count;
  renc->low = ( renc->low & 0x00FFFFFFU ) << 8;
  }

static inline void Re_reset( Range_encoder * const renc,
                             const unsigned dictionary_size )
  {
  Cb_reset( &renc->cb );
  renc->low = 0;
  renc->partial_member_pos = 0;
  renc->range = 0xFFFFFFFFU;
  renc->ff_count = 0;
  renc->cache = 0;
  Lh_set_dictionary_size( renc->header, dictionary_size );
  int i; for( i = 0; i < Lh_size; ++i ) Cb_put_byte( &renc->cb, renc->header[i] );
  }

static inline bool Re_init( Range_encoder * const renc,
                            const unsigned dictionary_size,
                            const unsigned min_free_bytes )
  {
  if( !Cb_init( &renc->cb, 65536 + min_free_bytes ) ) return false;
  renc->min_free_bytes = min_free_bytes;
  Lh_set_magic( renc->header );
  Re_reset( renc, dictionary_size );
  return true;
  }

static inline void Re_free( Range_encoder * const renc )
  { Cb_free( &renc->cb ); }

static inline unsigned long long
Re_member_position( const Range_encoder * const renc )
  { return renc->partial_member_pos + Cb_used_bytes( &renc->cb ) + renc->ff_count; }

static inline bool Re_enough_free_bytes( const Range_encoder * const renc )
  { return Cb_free_bytes( &renc->cb ) >= renc->min_free_bytes + renc->ff_count; }

static inline int Re_read_data( Range_encoder * const renc,
                                uint8_t * const out_buffer, const int out_size )
  {
  const int size = Cb_read_data( &renc->cb, out_buffer, out_size );
  if( size > 0 ) renc->partial_member_pos += size;
  return size;
  }

static inline void Re_flush( Range_encoder * const renc )
  {
  int i; for( i = 0; i < 5; ++i ) Re_shift_low( renc );
  renc->low = 0;
  renc->range = 0xFFFFFFFFU;
  renc->ff_count = 0;
  renc->cache = 0;
  }

static inline void Re_encode( Range_encoder * const renc,
                              const int symbol, const int num_bits )
  {
  unsigned mask;
  for( mask = 1 << ( num_bits - 1 ); mask > 0; mask >>= 1 )
    {
    renc->range >>= 1;
    if( symbol & mask ) renc->low += renc->range;
    if( renc->range <= 0x00FFFFFFU ) { renc->range <<= 8; Re_shift_low( renc ); }
    }
  }

static inline void Re_encode_bit( Range_encoder * const renc,
                                  Bit_model * const probability, const bool bit )
  {
  const uint32_t bound = ( renc->range >> bit_model_total_bits ) * *probability;
  if( !bit )
    {
    renc->range = bound;
    *probability += (bit_model_total - *probability) >> bit_model_move_bits;
    }
  else
    {
    renc->low += bound;
    renc->range -= bound;
    *probability -= *probability >> bit_model_move_bits;
    }
  if( renc->range <= 0x00FFFFFFU ) { renc->range <<= 8; Re_shift_low( renc ); }
  }

static inline void Re_encode_tree3( Range_encoder * const renc,
                                    Bit_model bm[], const int symbol )
  {
  bool bit = ( symbol >> 2 ) & 1;
  Re_encode_bit( renc, &bm[1], bit );
  int model = 2 | bit;
  bit = ( symbol >> 1 ) & 1;
  Re_encode_bit( renc, &bm[model], bit ); model <<= 1; model |= bit;
  Re_encode_bit( renc, &bm[model], symbol & 1 );
  }

static inline void Re_encode_tree6( Range_encoder * const renc,
                                    Bit_model bm[], const unsigned symbol )
  {
  bool bit = ( symbol >> 5 ) & 1;
  Re_encode_bit( renc, &bm[1], bit );
  int model = 2 | bit;
  bit = ( symbol >> 4 ) & 1;
  Re_encode_bit( renc, &bm[model], bit ); model <<= 1; model |= bit;
  bit = ( symbol >> 3 ) & 1;
  Re_encode_bit( renc, &bm[model], bit ); model <<= 1; model |= bit;
  bit = ( symbol >> 2 ) & 1;
  Re_encode_bit( renc, &bm[model], bit ); model <<= 1; model |= bit;
  bit = ( symbol >> 1 ) & 1;
  Re_encode_bit( renc, &bm[model], bit ); model <<= 1; model |= bit;
  Re_encode_bit( renc, &bm[model], symbol & 1 );
  }

static inline void Re_encode_tree8( Range_encoder * const renc,
                                    Bit_model bm[], const int symbol )
  {
  int model = 1;
  int i;
  for( i = 7; i >= 0; --i )
    {
    const bool bit = ( symbol >> i ) & 1;
    Re_encode_bit( renc, &bm[model], bit );
    model <<= 1; model |= bit;
    }
  }

static inline void Re_encode_tree_reversed( Range_encoder * const renc,
                     Bit_model bm[], int symbol, const int num_bits )
  {
  int model = 1;
  int i;
  for( i = num_bits; i > 0; --i )
    {
    const bool bit = symbol & 1;
    symbol >>= 1;
    Re_encode_bit( renc, &bm[model], bit );
    model <<= 1; model |= bit;
    }
  }

static inline void Re_encode_matched( Range_encoder * const renc,
                                      Bit_model bm[], unsigned symbol,
                                      unsigned match_byte )
  {
  unsigned mask = 0x100;
  symbol |= mask;
  while( true )
    {
    const unsigned match_bit = ( match_byte <<= 1 ) & mask;
    const bool bit = ( symbol <<= 1 ) & 0x100;
    Re_encode_bit( renc, &bm[(symbol>>9)+match_bit+mask], bit );
    if( symbol >= 0x10000 ) break;
    mask &= ~(match_bit ^ symbol);	/* if( match_bit != bit ) mask = 0; */
    }
  }

static inline void Re_encode_len( Range_encoder * const renc,
                                  Len_model * const lm,
                                  int symbol, const int pos_state )
  {
  bool bit = ( symbol -= min_match_len ) >= len_low_symbols;
  Re_encode_bit( renc, &lm->choice1, bit );
  if( !bit )
    Re_encode_tree3( renc, lm->bm_low[pos_state], symbol );
  else
    {
    bit = ( symbol -= len_low_symbols ) >= len_mid_symbols;
    Re_encode_bit( renc, &lm->choice2, bit );
    if( !bit )
      Re_encode_tree3( renc, lm->bm_mid[pos_state], symbol );
    else
      Re_encode_tree8( renc, lm->bm_high, symbol - len_mid_symbols );
    }
  }


enum { max_marker_size = 16,		/* rd_min_available_bytes + 5 */
       num_rep_distances = 4 };		/* must be 4 */

typedef struct LZ_encoder_base
  {
  Matchfinder_base mb;
  unsigned long long member_size_limit;
  uint32_t crc;

  Bit_model bm_literal[1<<literal_context_bits][0x300];
  Bit_model bm_match[states][pos_states];
  Bit_model bm_rep[states];
  Bit_model bm_rep0[states];
  Bit_model bm_rep1[states];
  Bit_model bm_rep2[states];
  Bit_model bm_len[states][pos_states];
  Bit_model bm_dis_slot[len_states][1<<dis_slot_bits];
  Bit_model bm_dis[modeled_distances-end_dis_model+1];
  Bit_model bm_align[dis_align_size];
  Len_model match_len_model;
  Len_model rep_len_model;
  Range_encoder renc;
  int reps[num_rep_distances];		/* latest four distances */
  State state;
  bool member_finished;
  } LZ_encoder_base;

static void LZeb_reset( LZ_encoder_base * const eb,
                        const unsigned long long member_size );

static inline bool LZeb_init( LZ_encoder_base * const eb,
                              const int before_size, const int dict_size,
                              const int after_size, const int dict_factor,
                              const int num_prev_positions23,
                              const int pos_array_factor,
                              const unsigned min_free_bytes,
                              const unsigned long long member_size )
  {
  if( !Mb_init( &eb->mb, before_size, dict_size, after_size, dict_factor,
                num_prev_positions23, pos_array_factor ) ) return false;
  if( !Re_init( &eb->renc, eb->mb.dictionary_size, min_free_bytes ) )
    return false;
  LZeb_reset( eb, member_size );
  return true;
  }

static inline bool LZeb_member_finished( const LZ_encoder_base * const eb )
  { return eb->member_finished && Cb_empty( &eb->renc.cb ); }

static inline void LZeb_free( LZ_encoder_base * const eb )
  { Re_free( &eb->renc ); Mb_free( &eb->mb ); }

static inline unsigned LZeb_crc( const LZ_encoder_base * const eb )
  { return eb->crc ^ 0xFFFFFFFFU; }

static inline int LZeb_price_literal( const LZ_encoder_base * const eb,
                            const uint8_t prev_byte, const uint8_t symbol )
  { return price_symbol8( eb->bm_literal[get_lit_state(prev_byte)], symbol ); }

static inline int LZeb_price_matched( const LZ_encoder_base * const eb,
  const uint8_t prev_byte, const uint8_t symbol, const uint8_t match_byte )
  { return price_matched( eb->bm_literal[get_lit_state(prev_byte)], symbol,
                          match_byte ); }

static inline void LZeb_encode_literal( LZ_encoder_base * const eb,
                            const uint8_t prev_byte, const uint8_t symbol )
  { Re_encode_tree8( &eb->renc, eb->bm_literal[get_lit_state(prev_byte)], symbol ); }

static inline void LZeb_encode_matched( LZ_encoder_base * const eb,
  const uint8_t prev_byte, const uint8_t symbol, const uint8_t match_byte )
  { Re_encode_matched( &eb->renc, eb->bm_literal[get_lit_state(prev_byte)],
                       symbol, match_byte ); }

static inline void LZeb_encode_pair( LZ_encoder_base * const eb,
                                     const unsigned dis, const int len,
                                     const int pos_state )
  {
  Re_encode_len( &eb->renc, &eb->match_len_model, len, pos_state );
  const unsigned dis_slot = get_slot( dis );
  Re_encode_tree6( &eb->renc, eb->bm_dis_slot[get_len_state(len)], dis_slot );

  if( dis_slot >= start_dis_model )
    {
    const int direct_bits = ( dis_slot >> 1 ) - 1;
    const unsigned base = ( 2 | ( dis_slot & 1 ) ) << direct_bits;
    const unsigned direct_dis = dis - base;

    if( dis_slot < end_dis_model )
      Re_encode_tree_reversed( &eb->renc, eb->bm_dis + ( base - dis_slot ),
                               direct_dis, direct_bits );
    else
      {
      Re_encode( &eb->renc, direct_dis >> dis_align_bits,
                 direct_bits - dis_align_bits );
      Re_encode_tree_reversed( &eb->renc, eb->bm_align, direct_dis, dis_align_bits );
      }
    }
  }

static bool Mb_normalize_pos( Matchfinder_base * const mb )
  {
  if( mb->pos > mb->stream_pos )
    { mb->pos = mb->stream_pos; return false; }
  if( !mb->at_stream_end )
    {
    int i;
    /* offset is int32_t for the min below */
    const int32_t offset = mb->pos - mb->before_size - mb->dictionary_size;
    const int size = mb->stream_pos - offset;
    memmove( mb->buffer, mb->buffer + offset, size );
    mb->partial_data_pos += offset;
    mb->pos -= offset;		/* pos = before_size + dictionary_size */
    mb->stream_pos -= offset;
    for( i = 0; i < mb->num_prev_positions; ++i )
      mb->prev_positions[i] -= min( mb->prev_positions[i], offset );
    for( i = 0; i < mb->pos_array_size; ++i )
      mb->pos_array[i] -= min( mb->pos_array[i], offset );
    }
  return true;
  }


static bool Mb_init( Matchfinder_base * const mb, const int before_size,
                     const int dict_size, const int after_size,
                     const int dict_factor, const int num_prev_positions23,
                     const int pos_array_factor )
  {
  const int buffer_size_limit =
    ( dict_factor * dict_size ) + before_size + after_size;
  int i;

  mb->partial_data_pos = 0;
  mb->before_size = before_size;
  mb->after_size = after_size;
  mb->pos = 0;
  mb->cyclic_pos = 0;
  mb->stream_pos = 0;
  mb->num_prev_positions23 = num_prev_positions23;
  mb->at_stream_end = false;
  mb->sync_flush_pending = false;

  mb->buffer_size = max( 65536, buffer_size_limit );
  mb->buffer = (uint8_t *)malloc( mb->buffer_size );
  if( !mb->buffer ) return false;
  mb->saved_dictionary_size = dict_size;
  mb->dictionary_size = dict_size;
  mb->pos_limit = mb->buffer_size - after_size;
  unsigned size = 1 << max( 16, real_bits( mb->dictionary_size - 1 ) - 2 );
  if( mb->dictionary_size > 1 << 26 ) size >>= 1;	/* 64 MiB */
  mb->key4_mask = size - 1;		/* increases with dictionary size */
  size += num_prev_positions23;
  mb->num_prev_positions = size;

  mb->pos_array_size = pos_array_factor * ( mb->dictionary_size + 1 );
  size += mb->pos_array_size;
  if( size * sizeof mb->prev_positions[0] <= size ) mb->prev_positions = 0;
  else mb->prev_positions =
    (int32_t *)malloc( size * sizeof mb->prev_positions[0] );
  if( !mb->prev_positions ) { free( mb->buffer ); return false; }
  mb->pos_array = mb->prev_positions + mb->num_prev_positions;
  for( i = 0; i < mb->num_prev_positions; ++i ) mb->prev_positions[i] = 0;
  return true;
  }


static void Mb_adjust_array( Matchfinder_base * const mb )
  {
  int size = 1 << max( 16, real_bits( mb->dictionary_size - 1 ) - 2 );
  if( mb->dictionary_size > 1 << 26 ) size >>= 1;	/* 64 MiB */
  mb->key4_mask = size - 1;
  size += mb->num_prev_positions23;
  mb->num_prev_positions = size;
  mb->pos_array = mb->prev_positions + mb->num_prev_positions;
  }


static void Mb_adjust_dictionary_size( Matchfinder_base * const mb )
  {
  if( mb->stream_pos < mb->dictionary_size )
    {
    mb->dictionary_size = max( min_dictionary_size, mb->stream_pos );
    Mb_adjust_array( mb );
    mb->pos_limit = mb->buffer_size;
    }
  }


static void Mb_reset( Matchfinder_base * const mb )
  {
  int i;
  if( mb->stream_pos > mb->pos )
    memmove( mb->buffer, mb->buffer + mb->pos, mb->stream_pos - mb->pos );
  mb->partial_data_pos = 0;
  mb->stream_pos -= mb->pos;
  mb->pos = 0;
  mb->cyclic_pos = 0;
  mb->at_stream_end = false;
  mb->sync_flush_pending = false;
  mb->dictionary_size = mb->saved_dictionary_size;
  Mb_adjust_array( mb );
  mb->pos_limit = mb->buffer_size - mb->after_size;
  for( i = 0; i < mb->num_prev_positions; ++i ) mb->prev_positions[i] = 0;
  }


/* End Of Stream marker => (dis == 0xFFFF_FFFF, len == min_match_len) */
static void LZeb_try_full_flush( LZ_encoder_base * const eb )
  {
  if( eb->member_finished || Cb_free_bytes( &eb->renc.cb ) <
      max_marker_size + eb->renc.ff_count + Lt_size ) return;
  eb->member_finished = true;
  const int pos_state = Mb_data_position( &eb->mb ) & pos_state_mask;
  const State state = eb->state;
  Re_encode_bit( &eb->renc, &eb->bm_match[state][pos_state], 1 );
  Re_encode_bit( &eb->renc, &eb->bm_rep[state], 0 );
  LZeb_encode_pair( eb, 0xFFFFFFFFU, min_match_len, pos_state );
  Re_flush( &eb->renc );
  Lzip_trailer trailer;
  Lt_set_data_crc( trailer, LZeb_crc( eb ) );
  Lt_set_data_size( trailer, Mb_data_position( &eb->mb ) );
  Lt_set_member_size( trailer, Re_member_position( &eb->renc ) + Lt_size );
  int i; for( i = 0; i < Lt_size; ++i ) Cb_put_byte( &eb->renc.cb, trailer[i] );
  }


/* Sync Flush marker => (dis == 0xFFFF_FFFF, len == min_match_len + 1) */
static void LZeb_try_sync_flush( LZ_encoder_base * const eb )
  {
  const unsigned min_size = eb->renc.ff_count + max_marker_size;
  if( eb->member_finished ||
      Cb_free_bytes( &eb->renc.cb ) < min_size + max_marker_size ) return;
  eb->mb.sync_flush_pending = false;
  const unsigned long long old_mpos = Re_member_position( &eb->renc );
  const int pos_state = Mb_data_position( &eb->mb ) & pos_state_mask;
  const State state = eb->state;
  do {		/* size of markers must be >= rd_min_available_bytes + 5 */
    Re_encode_bit( &eb->renc, &eb->bm_match[state][pos_state], 1 );
    Re_encode_bit( &eb->renc, &eb->bm_rep[state], 0 );
    LZeb_encode_pair( eb, 0xFFFFFFFFU, min_match_len + 1, pos_state );
    Re_flush( &eb->renc );
    }
  while( Re_member_position( &eb->renc ) - old_mpos < min_size );
  }


static void LZeb_reset( LZ_encoder_base * const eb,
                        const unsigned long long member_size )
  {
  const unsigned long long min_member_size = min_dictionary_size;
  const unsigned long long max_member_size = 1ULL << 51;	/* 2 PiB */
  Mb_reset( &eb->mb );
  eb->member_size_limit = min( max( min_member_size, member_size ),
                          max_member_size ) - Lt_size - max_marker_size;
  eb->crc = 0xFFFFFFFFU;
  Bm_array_init( eb->bm_literal[0], (1 << literal_context_bits) * 0x300 );
  Bm_array_init( eb->bm_match[0], states * pos_states );
  Bm_array_init( eb->bm_rep, states );
  Bm_array_init( eb->bm_rep0, states );
  Bm_array_init( eb->bm_rep1, states );
  Bm_array_init( eb->bm_rep2, states );
  Bm_array_init( eb->bm_len[0], states * pos_states );
  Bm_array_init( eb->bm_dis_slot[0], len_states * (1 << dis_slot_bits) );
  Bm_array_init( eb->bm_dis, modeled_distances - end_dis_model + 1 );
  Bm_array_init( eb->bm_align, dis_align_size );
  Lm_init( &eb->match_len_model );
  Lm_init( &eb->rep_len_model );
  Re_reset( &eb->renc, eb->mb.dictionary_size );
  int i; for( i = 0; i < num_rep_distances; ++i ) eb->reps[i] = 0;
  eb->state = 0;
  eb->member_finished = false;
  }

typedef struct Len_prices
  {
  const Len_model * lm;
  int len_symbols;
  int count;
  int prices[pos_states][max_len_symbols];
  int counters[pos_states];			/* may decrement below 0 */
  } Len_prices;

static inline void Lp_update_low_mid_prices( Len_prices * const lp,
                                             const int pos_state )
  {
  int * const pps = lp->prices[pos_state];
  int tmp = price0( lp->lm->choice1 );
  int len = 0;
  for( ; len < len_low_symbols && len < lp->len_symbols; ++len )
    pps[len] = tmp + price_symbol3( lp->lm->bm_low[pos_state], len );
  if( len >= lp->len_symbols ) return;
  tmp = price1( lp->lm->choice1 ) + price0( lp->lm->choice2 );
  for( ; len < len_low_symbols + len_mid_symbols && len < lp->len_symbols; ++len )
    pps[len] = tmp +
               price_symbol3( lp->lm->bm_mid[pos_state], len - len_low_symbols );
    }

static inline void Lp_update_high_prices( Len_prices * const lp )
  {
  const int tmp = price1( lp->lm->choice1 ) + price1( lp->lm->choice2 );
  int len;
  for( len = len_low_symbols + len_mid_symbols; len < lp->len_symbols; ++len )
    /* using 4 slots per value makes "Lp_price" faster */
    lp->prices[3][len] = lp->prices[2][len] =
    lp->prices[1][len] = lp->prices[0][len] = tmp +
      price_symbol8( lp->lm->bm_high, len - len_low_symbols - len_mid_symbols );
  }

static inline void Lp_reset( Len_prices * const lp )
  { int i; for( i = 0; i < pos_states; ++i ) lp->counters[i] = 0; }

static inline void Lp_init( Len_prices * const lp, const Len_model * const lm,
                            const int match_len_limit )
  {
  lp->lm = lm;
  lp->len_symbols = match_len_limit + 1 - min_match_len;
  lp->count = (match_len_limit > 12) ? 1 : lp->len_symbols;
  Lp_reset( lp );
  }

static inline void Lp_decrement_counter( Len_prices * const lp,
                                         const int pos_state )
  { --lp->counters[pos_state]; }

static inline void Lp_update_prices( Len_prices * const lp )
  {
  int pos_state;
  bool high_pending = false;
  for( pos_state = 0; pos_state < pos_states; ++pos_state )
    if( lp->counters[pos_state] <= 0 )
      { lp->counters[pos_state] = lp->count;
        Lp_update_low_mid_prices( lp, pos_state ); high_pending = true; }
  if( high_pending && lp->len_symbols > len_low_symbols + len_mid_symbols )
    Lp_update_high_prices( lp );
  }

static inline int Lp_price( const Len_prices * const lp,
                            const int len, const int pos_state )
  { return lp->prices[pos_state][len - min_match_len]; }


typedef struct Pair		/* distance-length pair */
  {
  int dis;
  int len;
  } Pair;

enum { infinite_price = 0x0FFFFFFF,
       max_num_trials = 1 << 13,
       single_step_trial = -2,
       dual_step_trial = -1 };

typedef struct Trial
  {
  State state;
  int price;		/* dual use var; cumulative price, match length */
  int cdis;		/* -1 for literal, or rep, or match distance + 4 */
  int prev_index;	/* index of prev trial in trials[] */
  int prev_index2;	/*   -2  trial is single step */
			/*   -1  literal + rep0 */
			/* >= 0  ( rep or match ) + literal + rep0 */
  int reps[num_rep_distances];		/* latest four distances */
  } Trial;

static inline void Tr_update( Trial * const trial, const int pr,
                              const int cdi, const int p_i )
  {
  if( pr < trial->price )
    { trial->price = pr; trial->cdis = cdi; trial->prev_index = p_i;
      trial->prev_index2 = single_step_trial; }
  }

static inline void Tr_update2( Trial * const trial, const int pr,
                               const int p_i )
  {
  if( pr < trial->price )
    { trial->price = pr; trial->cdis = 0; trial->prev_index = p_i;
      trial->prev_index2 = dual_step_trial; }
  }

static inline void Tr_update3( Trial * const trial, const int pr,
                               const int cdi, const int p_i,
                               const int p_i2 )
  {
  if( pr < trial->price )
    { trial->price = pr; trial->cdis = cdi; trial->prev_index = p_i;
      trial->prev_index2 = p_i2; }
  }


typedef struct LZ_encoder
  {
  LZ_encoder_base eb;
  int cycles;
  int match_len_limit;
  Len_prices match_len_prices;
  Len_prices rep_len_prices;
  int pending_num_pairs;
  Pair pairs[max_match_len+1];
  Trial trials[max_num_trials];

  int dis_slot_prices[len_states][2*max_dictionary_bits];
  int dis_prices[len_states][modeled_distances];
  int align_prices[dis_align_size];
  int num_dis_slots;
  int price_counter;		/* counters may decrement below 0 */
  int dis_price_counter;
  int align_price_counter;
  bool been_flushed;
  } LZ_encoder;

static inline bool Mb_dec_pos( Matchfinder_base * const mb, const int ahead )
  {
  if( ahead < 0 || mb->pos < ahead ) return false;
  mb->pos -= ahead;
  if( mb->cyclic_pos < ahead ) mb->cyclic_pos += mb->dictionary_size + 1;
  mb->cyclic_pos -= ahead;
  return true;
  }

static int LZe_get_match_pairs( LZ_encoder * const e, Pair * pairs );

       /* move-to-front dis in/into reps; do nothing if( cdis <= 0 ) */
static inline void mtf_reps( const int cdis, int reps[num_rep_distances] )
  {
  if( cdis >= num_rep_distances )			/* match */
    {
    reps[3] = reps[2]; reps[2] = reps[1]; reps[1] = reps[0];
    reps[0] = cdis - num_rep_distances;
    }
  else if( cdis > 0 )				/* repeated match */
    {
    const int distance = reps[cdis];
    int i; for( i = cdis; i > 0; --i ) reps[i] = reps[i-1];
    reps[0] = distance;
    }
  }

static inline int LZeb_price_shortrep( const LZ_encoder_base * const eb,
                                       const State state, const int pos_state )
  {
  return price0( eb->bm_rep0[state] ) + price0( eb->bm_len[state][pos_state] );
  }

static inline int LZeb_price_rep( const LZ_encoder_base * const eb,
                                  const int rep, const State state,
                                  const int pos_state )
  {
  if( rep == 0 ) return price0( eb->bm_rep0[state] ) +
                        price1( eb->bm_len[state][pos_state] );
  int price = price1( eb->bm_rep0[state] );
  if( rep == 1 )
    price += price0( eb->bm_rep1[state] );
  else
    {
    price += price1( eb->bm_rep1[state] );
    price += price_bit( eb->bm_rep2[state], rep - 2 );
    }
  return price;
  }

static inline int LZe_price_rep0_len( const LZ_encoder * const e,
                                      const int len, const State state,
                                      const int pos_state )
  {
  return LZeb_price_rep( &e->eb, 0, state, pos_state ) +
         Lp_price( &e->rep_len_prices, len, pos_state );
  }

static inline int LZe_price_pair( const LZ_encoder * const e,
                                  const int dis, const int len,
                                  const int pos_state )
  {
  const int price = Lp_price( &e->match_len_prices, len, pos_state );
  const int len_state = get_len_state( len );
  if( dis < modeled_distances )
    return price + e->dis_prices[len_state][dis];
  else
    return price + e->dis_slot_prices[len_state][get_slot( dis )] +
           e->align_prices[dis & (dis_align_size - 1)];
  }

static inline int LZe_read_match_distances( LZ_encoder * const e )
  {
  const int num_pairs = LZe_get_match_pairs( e, e->pairs );
  if( num_pairs > 0 )
    {
    const int len = e->pairs[num_pairs-1].len;
    if( len == e->match_len_limit && len < max_match_len )
      e->pairs[num_pairs-1].len =
        Mb_true_match_len( &e->eb.mb, len, e->pairs[num_pairs-1].dis + 1 );
    }
  return num_pairs;
  }

static inline bool LZe_move_and_update( LZ_encoder * const e, int n )
  {
  while( true )
    {
    if( !Mb_move_pos( &e->eb.mb ) ) return false;
    if( --n <= 0 ) break;
    LZe_get_match_pairs( e, 0 );
    }
  return true;
  }

static inline void LZe_backward( LZ_encoder * const e, int cur )
  {
  int cdis = e->trials[cur].cdis;
  while( cur > 0 )
    {
    const int prev_index = e->trials[cur].prev_index;
    Trial * const prev_trial = &e->trials[prev_index];

    if( e->trials[cur].prev_index2 != single_step_trial )
      {
      prev_trial->cdis = -1;					/* literal */
      prev_trial->prev_index = prev_index - 1;
      prev_trial->prev_index2 = single_step_trial;
      if( e->trials[cur].prev_index2 >= 0 )
        {
        Trial * const prev_trial2 = &e->trials[prev_index-1];
        prev_trial2->cdis = cdis; cdis = 0;			/* rep0 */
        prev_trial2->prev_index = e->trials[cur].prev_index2;
        prev_trial2->prev_index2 = single_step_trial;
        }
      }
    prev_trial->price = cur - prev_index;			/* len */
    cur = cdis; cdis = prev_trial->cdis; prev_trial->cdis = cur;
    cur = prev_index;
    }
  }

enum { num_prev_positions3 = 1 << 16,
       num_prev_positions2 = 1 << 10 };

static inline bool LZe_init( LZ_encoder * const e,
                             const int dict_size, const int len_limit,
                             const unsigned long long member_size )
  {
  enum { before_size = max_num_trials,
         /* bytes to keep in buffer after pos */
         after_size = max_num_trials + ( 2 * max_match_len ) + 1,
         dict_factor = 2,
         num_prev_positions23 = num_prev_positions2 + num_prev_positions3,
         pos_array_factor = 2,
         min_free_bytes = 2 * max_num_trials };

  if( !LZeb_init( &e->eb, before_size, dict_size, after_size, dict_factor,
                  num_prev_positions23, pos_array_factor, min_free_bytes,
                  member_size ) ) return false;
  e->cycles = (len_limit < max_match_len) ? 16 + ( len_limit / 2 ) : 256;
  e->match_len_limit = len_limit;
  Lp_init( &e->match_len_prices, &e->eb.match_len_model, e->match_len_limit );
  Lp_init( &e->rep_len_prices, &e->eb.rep_len_model, e->match_len_limit );
  e->pending_num_pairs = 0;
  e->num_dis_slots = 2 * real_bits( e->eb.mb.dictionary_size - 1 );
  e->trials[1].prev_index = 0;
  e->trials[1].prev_index2 = single_step_trial;
  e->price_counter = 0;
  e->dis_price_counter = 0;
  e->align_price_counter = 0;
  e->been_flushed = false;
  return true;
  }

static inline void LZe_reset( LZ_encoder * const e,
                              const unsigned long long member_size )
  {
  LZeb_reset( &e->eb, member_size );
  Lp_reset( &e->match_len_prices );
  Lp_reset( &e->rep_len_prices );
  e->pending_num_pairs = 0;
  e->price_counter = 0;
  e->dis_price_counter = 0;
  e->align_price_counter = 0;
  e->been_flushed = false;
  }

static int LZe_get_match_pairs( LZ_encoder * const e, Pair * pairs )
  {
  int32_t * ptr0 = e->eb.mb.pos_array + ( e->eb.mb.cyclic_pos << 1 );
  int32_t * ptr1 = ptr0 + 1;
  int len_limit = e->match_len_limit;
  if( len_limit > Mb_available_bytes( &e->eb.mb ) )
    {
    e->been_flushed = true;
    len_limit = Mb_available_bytes( &e->eb.mb );
    if( len_limit < 4 ) { *ptr0 = *ptr1 = 0; return 0; }
    }

  int maxlen = 3;			/* only used if pairs != 0 */
  int num_pairs = 0;
  const int min_pos = (e->eb.mb.pos > e->eb.mb.dictionary_size) ?
                       e->eb.mb.pos - e->eb.mb.dictionary_size : 0;
  const uint8_t * const data = Mb_ptr_to_current_pos( &e->eb.mb );

  unsigned tmp = crc32[data[0]] ^ data[1];
  const int key2 = tmp & ( num_prev_positions2 - 1 );
  tmp ^= (unsigned)data[2] << 8;
  const int key3 = num_prev_positions2 + ( tmp & ( num_prev_positions3 - 1 ) );
  const int key4 = num_prev_positions2 + num_prev_positions3 +
                   ( ( tmp ^ ( crc32[data[3]] << 5 ) ) & e->eb.mb.key4_mask );

  if( pairs )
    {
    const int np2 = e->eb.mb.prev_positions[key2];
    const int np3 = e->eb.mb.prev_positions[key3];
    if( np2 > min_pos && e->eb.mb.buffer[np2-1] == data[0] )
      {
      pairs[0].dis = e->eb.mb.pos - np2;
      pairs[0].len = maxlen = 2 + ( np2 == np3 );
      num_pairs = 1;
      }
    if( np2 != np3 && np3 > min_pos && e->eb.mb.buffer[np3-1] == data[0] )
      {
      maxlen = 3;
      pairs[num_pairs++].dis = e->eb.mb.pos - np3;
      }
    if( num_pairs > 0 )
      {
      const int delta = pairs[num_pairs-1].dis + 1;
      while( maxlen < len_limit && data[maxlen-delta] == data[maxlen] )
        ++maxlen;
      pairs[num_pairs-1].len = maxlen;
      if( maxlen < 3 ) maxlen = 3;
      if( maxlen >= len_limit ) pairs = 0;	/* done. now just skip */
      }
    }

  const int pos1 = e->eb.mb.pos + 1;
  e->eb.mb.prev_positions[key2] = pos1;
  e->eb.mb.prev_positions[key3] = pos1;
  int newpos1 = e->eb.mb.prev_positions[key4];
  e->eb.mb.prev_positions[key4] = pos1;

  int len = 0, len0 = 0, len1 = 0;

  int count;
  for( count = e->cycles; ; )
    {
    if( newpos1 <= min_pos || --count < 0 ) { *ptr0 = *ptr1 = 0; break; }

    if( e->been_flushed ) len = 0;
    const int delta = pos1 - newpos1;
    int32_t * const newptr = e->eb.mb.pos_array +
      ( ( e->eb.mb.cyclic_pos - delta +
          ( (e->eb.mb.cyclic_pos >= delta) ? 0 : e->eb.mb.dictionary_size + 1 ) ) << 1 );
    if( data[len-delta] == data[len] )
      {
      while( ++len < len_limit && data[len-delta] == data[len] ) {}
      if( pairs && maxlen < len )
        {
        pairs[num_pairs].dis = delta - 1;
        pairs[num_pairs].len = maxlen = len;
        ++num_pairs;
        }
      if( len >= len_limit )
        {
        *ptr0 = newptr[0];
        *ptr1 = newptr[1];
        break;
        }
      }
    if( data[len-delta] < data[len] )
      {
      *ptr0 = newpos1;
      ptr0 = newptr + 1;
      newpos1 = *ptr0;
      len0 = len; if( len1 < len ) len = len1;
      }
    else
      {
      *ptr1 = newpos1;
      ptr1 = newptr;
      newpos1 = *ptr1;
      len1 = len; if( len0 < len ) len = len0;
      }
    }
  return num_pairs;
  }


static void LZe_update_distance_prices( LZ_encoder * const e )
  {
  int dis, len_state;
  for( dis = start_dis_model; dis < modeled_distances; ++dis )
    {
    const int dis_slot = dis_slots[dis];
    const int direct_bits = ( dis_slot >> 1 ) - 1;
    const int base = ( 2 | ( dis_slot & 1 ) ) << direct_bits;
    const int price = price_symbol_reversed( e->eb.bm_dis + ( base - dis_slot ),
                                             dis - base, direct_bits );
    for( len_state = 0; len_state < len_states; ++len_state )
      e->dis_prices[len_state][dis] = price;
    }

  for( len_state = 0; len_state < len_states; ++len_state )
    {
    int * const dsp = e->dis_slot_prices[len_state];
    const Bit_model * const bmds = e->eb.bm_dis_slot[len_state];
    int slot = 0;
    for( ; slot < end_dis_model; ++slot )
      dsp[slot] = price_symbol6( bmds, slot );
    for( ; slot < e->num_dis_slots; ++slot )
      dsp[slot] = price_symbol6( bmds, slot ) +
                  (((( slot >> 1 ) - 1 ) - dis_align_bits ) << price_shift_bits );

    int * const dp = e->dis_prices[len_state];
    for( dis = 0; dis < start_dis_model; ++dis )
      dp[dis] = dsp[dis];
    for( ; dis < modeled_distances; ++dis )
      dp[dis] += dsp[dis_slots[dis]];
    }
  }


/* Return the number of bytes advanced (ahead).
   trials[0]..trials[ahead-1] contain the steps to encode.
   ( trials[0].cdis == -1 ) means literal.
   A match/rep longer or equal than match_len_limit finishes the sequence.
*/
static int LZe_sequence_optimizer( LZ_encoder * const e,
                                   const int reps[num_rep_distances],
                                   const State state )
  {
  int num_pairs, num_trials;
  int i, rep, len;

  if( e->pending_num_pairs > 0 )		/* from previous call */
    {
    num_pairs = e->pending_num_pairs;
    e->pending_num_pairs = 0;
    }
  else
    num_pairs = LZe_read_match_distances( e );
  const int main_len = (num_pairs > 0) ? e->pairs[num_pairs-1].len : 0;

  int replens[num_rep_distances];
  int rep_index = 0;
  for( i = 0; i < num_rep_distances; ++i )
    {
    replens[i] = Mb_true_match_len( &e->eb.mb, 0, reps[i] + 1 );
    if( replens[i] > replens[rep_index] ) rep_index = i;
    }
  if( replens[rep_index] >= e->match_len_limit )
    {
    e->trials[0].price = replens[rep_index];
    e->trials[0].cdis = rep_index;
    if( !LZe_move_and_update( e, replens[rep_index] ) ) return 0;
    return replens[rep_index];
    }

  if( main_len >= e->match_len_limit )
    {
    e->trials[0].price = main_len;
    e->trials[0].cdis = e->pairs[num_pairs-1].dis + num_rep_distances;
    if( !LZe_move_and_update( e, main_len ) ) return 0;
    return main_len;
    }

  const int pos_state = Mb_data_position( &e->eb.mb ) & pos_state_mask;
  const uint8_t prev_byte = Mb_peek( &e->eb.mb, 1 );
  const uint8_t cur_byte = Mb_peek( &e->eb.mb, 0 );
  const uint8_t match_byte = Mb_peek( &e->eb.mb, reps[0] + 1 );

  e->trials[1].price = price0( e->eb.bm_match[state][pos_state] );
  if( St_is_char( state ) )
    e->trials[1].price += LZeb_price_literal( &e->eb, prev_byte, cur_byte );
  else
    e->trials[1].price += LZeb_price_matched( &e->eb, prev_byte, cur_byte, match_byte );
  e->trials[1].cdis = -1;				/* literal */

  const int match_price = price1( e->eb.bm_match[state][pos_state] );
  const int rep_match_price = match_price + price1( e->eb.bm_rep[state] );

  if( match_byte == cur_byte )
    Tr_update( &e->trials[1], rep_match_price +
               LZeb_price_shortrep( &e->eb, state, pos_state ), 0, 0 );

  num_trials = max( main_len, replens[rep_index] );

  if( num_trials < min_match_len )
    {
    e->trials[0].price = 1;
    e->trials[0].cdis = e->trials[1].cdis;
    if( !Mb_move_pos( &e->eb.mb ) ) return 0;
    return 1;
    }

  e->trials[0].state = state;
  for( i = 0; i < num_rep_distances; ++i )
    e->trials[0].reps[i] = reps[i];

  for( len = min_match_len; len <= num_trials; ++len )
    e->trials[len].price = infinite_price;

  for( rep = 0; rep < num_rep_distances; ++rep )
    {
    if( replens[rep] < min_match_len ) continue;
    const int price = rep_match_price + LZeb_price_rep( &e->eb, rep, state, pos_state );
    for( len = min_match_len; len <= replens[rep]; ++len )
      Tr_update( &e->trials[len], price +
                 Lp_price( &e->rep_len_prices, len, pos_state ), rep, 0 );
    }

  if( main_len > replens[0] )
    {
    const int normal_match_price = match_price + price0( e->eb.bm_rep[state] );
    int i = 0, len = max( replens[0] + 1, min_match_len );
    while( len > e->pairs[i].len ) ++i;
    while( true )
      {
      const int dis = e->pairs[i].dis;
      Tr_update( &e->trials[len], normal_match_price +
                 LZe_price_pair( e, dis, len, pos_state ),
                 dis + num_rep_distances, 0 );
      if( ++len > e->pairs[i].len && ++i >= num_pairs ) break;
      }
    }

  int cur = 0;
  while( true )				/* price optimization loop */
    {
    if( !Mb_move_pos( &e->eb.mb ) ) return 0;
    if( ++cur >= num_trials )		/* no more initialized trials */
      {
      LZe_backward( e, cur );
      return cur;
      }

    const int num_pairs = LZe_read_match_distances( e );
    const int newlen = (num_pairs > 0) ? e->pairs[num_pairs-1].len : 0;
    if( newlen >= e->match_len_limit )
      {
      e->pending_num_pairs = num_pairs;
      LZe_backward( e, cur );
      return cur;
      }

    /* give final values to current trial */
    Trial * cur_trial = &e->trials[cur];
    State cur_state;
    {
    const int cdis = cur_trial->cdis;
    int prev_index = cur_trial->prev_index;
    const int prev_index2 = cur_trial->prev_index2;

    if( prev_index2 == single_step_trial )
      {
      cur_state = e->trials[prev_index].state;
      if( prev_index + 1 == cur )			/* len == 1 */
        {
        if( cdis == 0 ) cur_state = St_set_shortrep( cur_state );
        else cur_state = St_set_char( cur_state );	/* literal */
        }
      else if( cdis < num_rep_distances ) cur_state = St_set_rep( cur_state );
      else cur_state = St_set_match( cur_state );
      }
    else
      {
      if( prev_index2 == dual_step_trial )	/* cdis == 0 (rep0) */
        --prev_index;
      else					/* prev_index2 >= 0 */
        prev_index = prev_index2;
      cur_state = St_set_char_rep();
      }
    cur_trial->state = cur_state;
    for( i = 0; i < num_rep_distances; ++i )
      cur_trial->reps[i] = e->trials[prev_index].reps[i];
    mtf_reps( cdis, cur_trial->reps );		/* literal is ignored */
    }

    const int pos_state = Mb_data_position( &e->eb.mb ) & pos_state_mask;
    const uint8_t prev_byte = Mb_peek( &e->eb.mb, 1 );
    const uint8_t cur_byte = Mb_peek( &e->eb.mb, 0 );
    const uint8_t match_byte = Mb_peek( &e->eb.mb, cur_trial->reps[0] + 1 );

    int next_price = cur_trial->price +
                     price0( e->eb.bm_match[cur_state][pos_state] );
    if( St_is_char( cur_state ) )
      next_price += LZeb_price_literal( &e->eb, prev_byte, cur_byte );
    else
      next_price += LZeb_price_matched( &e->eb, prev_byte, cur_byte, match_byte );

    /* try last updates to next trial */
    Trial * next_trial = &e->trials[cur+1];

    Tr_update( next_trial, next_price, -1, cur );	/* literal */

    const int match_price =
      cur_trial->price + price1( e->eb.bm_match[cur_state][pos_state] );
    const int rep_match_price = match_price + price1( e->eb.bm_rep[cur_state] );

    if( match_byte == cur_byte && next_trial->cdis != 0 &&
        next_trial->prev_index2 == single_step_trial )
      {
      const int price =
        rep_match_price + LZeb_price_shortrep( &e->eb, cur_state, pos_state );
      if( price <= next_trial->price )
        {
        next_trial->price = price;
        next_trial->cdis = 0;				/* rep0 */
        next_trial->prev_index = cur;
        }
      }

    const int triable_bytes =
      min( Mb_available_bytes( &e->eb.mb ), max_num_trials - 1 - cur );
    if( triable_bytes < min_match_len ) continue;

    const int len_limit = min( e->match_len_limit, triable_bytes );

    /* try literal + rep0 */
    if( match_byte != cur_byte && next_trial->prev_index != cur )
      {
      const uint8_t * const data = Mb_ptr_to_current_pos( &e->eb.mb );
      const int dis = cur_trial->reps[0] + 1;
      const int limit = min( e->match_len_limit + 1, triable_bytes );
      int len = 1;
      while( len < limit && data[len-dis] == data[len] ) ++len;
      if( --len >= min_match_len )
        {
        const int pos_state2 = ( pos_state + 1 ) & pos_state_mask;
        const State state2 = St_set_char( cur_state );
        const int price = next_price +
                          price1( e->eb.bm_match[state2][pos_state2] ) +
                          price1( e->eb.bm_rep[state2] ) +
                          LZe_price_rep0_len( e, len, state2, pos_state2 );
        while( num_trials < cur + 1 + len )
          e->trials[++num_trials].price = infinite_price;
        Tr_update2( &e->trials[cur+1+len], price, cur + 1 );
        }
      }

    int start_len = min_match_len;

    /* try rep distances */
    for( rep = 0; rep < num_rep_distances; ++rep )
      {
      const uint8_t * const data = Mb_ptr_to_current_pos( &e->eb.mb );
      const int dis = cur_trial->reps[rep] + 1;

      if( data[0-dis] != data[0] || data[1-dis] != data[1] ) continue;
      for( len = min_match_len; len < len_limit; ++len )
        if( data[len-dis] != data[len] ) break;
      while( num_trials < cur + len )
        e->trials[++num_trials].price = infinite_price;
      int price = rep_match_price + LZeb_price_rep( &e->eb, rep, cur_state, pos_state );
      for( i = min_match_len; i <= len; ++i )
        Tr_update( &e->trials[cur+i], price +
                   Lp_price( &e->rep_len_prices, i, pos_state ), rep, cur );

      if( rep == 0 ) start_len = len + 1;	/* discard shorter matches */

      /* try rep + literal + rep0 */
      int len2 = len + 1;
      const int limit = min( e->match_len_limit + len2, triable_bytes );
      while( len2 < limit && data[len2-dis] == data[len2] ) ++len2;
      len2 -= len + 1;
      if( len2 < min_match_len ) continue;

      int pos_state2 = ( pos_state + len ) & pos_state_mask;
      State state2 = St_set_rep( cur_state );
      price += Lp_price( &e->rep_len_prices, len, pos_state ) +
               price0( e->eb.bm_match[state2][pos_state2] ) +
               LZeb_price_matched( &e->eb, data[len-1], data[len], data[len-dis] );
      pos_state2 = ( pos_state2 + 1 ) & pos_state_mask;
      state2 = St_set_char( state2 );
      price += price1( e->eb.bm_match[state2][pos_state2] ) +
               price1( e->eb.bm_rep[state2] ) +
               LZe_price_rep0_len( e, len2, state2, pos_state2 );
      while( num_trials < cur + len + 1 + len2 )
        e->trials[++num_trials].price = infinite_price;
      Tr_update3( &e->trials[cur+len+1+len2], price, rep, cur + len + 1, cur );
      }

    /* try matches */
    if( newlen >= start_len && newlen <= len_limit )
      {
      const int normal_match_price = match_price +
                                     price0( e->eb.bm_rep[cur_state] );

      while( num_trials < cur + newlen )
        e->trials[++num_trials].price = infinite_price;

      int i = 0;
      while( e->pairs[i].len < start_len ) ++i;
      int dis = e->pairs[i].dis;
      for( len = start_len; ; ++len )
        {
        int price = normal_match_price + LZe_price_pair( e, dis, len, pos_state );
        Tr_update( &e->trials[cur+len], price, dis + num_rep_distances, cur );

        /* try match + literal + rep0 */
        if( len == e->pairs[i].len )
          {
          const uint8_t * const data = Mb_ptr_to_current_pos( &e->eb.mb );
          const int dis2 = dis + 1;
          int len2 = len + 1;
          const int limit = min( e->match_len_limit + len2, triable_bytes );
          while( len2 < limit && data[len2-dis2] == data[len2] ) ++len2;
          len2 -= len + 1;
          if( len2 >= min_match_len )
            {
            int pos_state2 = ( pos_state + len ) & pos_state_mask;
            State state2 = St_set_match( cur_state );
            price += price0( e->eb.bm_match[state2][pos_state2] ) +
                     LZeb_price_matched( &e->eb, data[len-1], data[len], data[len-dis2] );
            pos_state2 = ( pos_state2 + 1 ) & pos_state_mask;
            state2 = St_set_char( state2 );
            price += price1( e->eb.bm_match[state2][pos_state2] ) +
                     price1( e->eb.bm_rep[state2] ) +
                     LZe_price_rep0_len( e, len2, state2, pos_state2 );

            while( num_trials < cur + len + 1 + len2 )
              e->trials[++num_trials].price = infinite_price;
            Tr_update3( &e->trials[cur+len+1+len2], price,
                        dis + num_rep_distances, cur + len + 1, cur );
            }
          if( ++i >= num_pairs ) break;
          dis = e->pairs[i].dis;
          }
        }
      }
    }
  }


static bool LZe_encode_member( LZ_encoder * const e )
  {
  const bool best = e->match_len_limit > 12;
  const int dis_price_count = best ? 1 : 512;
  const int align_price_count = best ? 1 : dis_align_size;
  const int price_count = (e->match_len_limit > 36) ? 1013 : 4093;
  int i;
  State * const state = &e->eb.state;

  if( e->eb.member_finished ) return true;
  if( Re_member_position( &e->eb.renc ) >= e->eb.member_size_limit )
    { LZeb_try_full_flush( &e->eb ); return true; }

  if( Mb_data_position( &e->eb.mb ) == 0 &&
      !Mb_data_finished( &e->eb.mb ) )		/* encode first byte */
    {
    if( !Mb_enough_available_bytes( &e->eb.mb ) ||
        !Re_enough_free_bytes( &e->eb.renc ) ) return true;
    const uint8_t prev_byte = 0;
    const uint8_t cur_byte = Mb_peek( &e->eb.mb, 0 );
    Re_encode_bit( &e->eb.renc, &e->eb.bm_match[*state][0], 0 );
    LZeb_encode_literal( &e->eb, prev_byte, cur_byte );
    CRC32_update_byte( &e->eb.crc, cur_byte );
    LZe_get_match_pairs( e, 0 );
    if( !Mb_move_pos( &e->eb.mb ) ) return false;
    }

  while( !Mb_data_finished( &e->eb.mb ) )
    {
    if( !Mb_enough_available_bytes( &e->eb.mb ) ||
        !Re_enough_free_bytes( &e->eb.renc ) ) return true;
    if( e->price_counter <= 0 && e->pending_num_pairs == 0 )
      {
      e->price_counter = price_count;	/* recalculate prices every these bytes */
      if( e->dis_price_counter <= 0 )
        { e->dis_price_counter = dis_price_count; LZe_update_distance_prices( e ); }
      if( e->align_price_counter <= 0 )
        {
        e->align_price_counter = align_price_count;
        for( i = 0; i < dis_align_size; ++i )
          e->align_prices[i] = price_symbol_reversed( e->eb.bm_align, i, dis_align_bits );
        }
      Lp_update_prices( &e->match_len_prices );
      Lp_update_prices( &e->rep_len_prices );
      }

    int ahead = LZe_sequence_optimizer( e, e->eb.reps, *state );
    e->price_counter -= ahead;

    for( i = 0; ahead > 0; )
      {
      const int pos_state =
        ( Mb_data_position( &e->eb.mb ) - ahead ) & pos_state_mask;
      const int len = e->trials[i].price;
      int dis = e->trials[i].cdis;

      bool bit = dis < 0;
      Re_encode_bit( &e->eb.renc, &e->eb.bm_match[*state][pos_state], !bit );
      if( bit )					/* literal byte */
        {
        const uint8_t prev_byte = Mb_peek( &e->eb.mb, ahead + 1 );
        const uint8_t cur_byte = Mb_peek( &e->eb.mb, ahead );
        CRC32_update_byte( &e->eb.crc, cur_byte );
        if( ( *state = St_set_char( *state ) ) < 4 )
          LZeb_encode_literal( &e->eb, prev_byte, cur_byte );
        else
          {
          const uint8_t match_byte = Mb_peek( &e->eb.mb, ahead + e->eb.reps[0] + 1 );
          LZeb_encode_matched( &e->eb, prev_byte, cur_byte, match_byte );
          }
        }
      else					/* match or repeated match */
        {
        CRC32_update_buf( &e->eb.crc, Mb_ptr_to_current_pos( &e->eb.mb ) - ahead, len );
        mtf_reps( dis, e->eb.reps );
        bit = dis < num_rep_distances;
        Re_encode_bit( &e->eb.renc, &e->eb.bm_rep[*state], bit );
        if( bit )				/* repeated match */
          {
          bit = dis == 0;
          Re_encode_bit( &e->eb.renc, &e->eb.bm_rep0[*state], !bit );
          if( bit )
            Re_encode_bit( &e->eb.renc, &e->eb.bm_len[*state][pos_state], len > 1 );
          else
            {
            Re_encode_bit( &e->eb.renc, &e->eb.bm_rep1[*state], dis > 1 );
            if( dis > 1 )
              Re_encode_bit( &e->eb.renc, &e->eb.bm_rep2[*state], dis > 2 );
            }
          if( len == 1 ) *state = St_set_shortrep( *state );
          else
            {
            Re_encode_len( &e->eb.renc, &e->eb.rep_len_model, len, pos_state );
            Lp_decrement_counter( &e->rep_len_prices, pos_state );
            *state = St_set_rep( *state );
            }
          }
        else					/* match */
          {
          dis -= num_rep_distances;
          LZeb_encode_pair( &e->eb, dis, len, pos_state );
          if( dis >= modeled_distances ) --e->align_price_counter;
          --e->dis_price_counter;
          Lp_decrement_counter( &e->match_len_prices, pos_state );
          *state = St_set_match( *state );
          }
        }
      ahead -= len; i += len;
      if( Re_member_position( &e->eb.renc ) >= e->eb.member_size_limit )
        {
        if( !Mb_dec_pos( &e->eb.mb, ahead ) ) return false;
        LZeb_try_full_flush( &e->eb );
        return true;
        }
      }
    }
  LZeb_try_full_flush( &e->eb );
  return true;
  }

typedef struct FLZ_encoder
  {
  LZ_encoder_base eb;
  unsigned key4;			/* key made from latest 4 bytes */
  } FLZ_encoder;

static inline void FLZe_reset_key4( FLZ_encoder * const fe )
  {
  int i;
  fe->key4 = 0;
  for( i = 0; i < 3 && i < Mb_available_bytes( &fe->eb.mb ); ++i )
    fe->key4 = ( fe->key4 << 4 ) ^ fe->eb.mb.buffer[i];
  }

static inline bool FLZe_update_and_move( FLZ_encoder * const fe, int n )
  {
  Matchfinder_base * const mb = &fe->eb.mb;
  while( --n >= 0 )
    {
    if( Mb_available_bytes( mb ) >= 4 )
      {
      fe->key4 = ( ( fe->key4 << 4 ) ^ mb->buffer[mb->pos+3] ) & mb->key4_mask;
      mb->pos_array[mb->cyclic_pos] = mb->prev_positions[fe->key4];
      mb->prev_positions[fe->key4] = mb->pos + 1;
      }
    else mb->pos_array[mb->cyclic_pos] = 0;
    if( !Mb_move_pos( mb ) ) return false;
    }
  return true;
  }

static inline bool FLZe_init( FLZ_encoder * const fe,
                              const unsigned long long member_size )
  {
  enum { before_size = 0,
         dict_size = 65536,
         /* bytes to keep in buffer after pos */
         after_size = max_match_len,
         dict_factor = 16,
         min_free_bytes = max_marker_size,
         num_prev_positions23 = 0,
         pos_array_factor = 1 };

  return LZeb_init( &fe->eb, before_size, dict_size, after_size, dict_factor,
                    num_prev_positions23, pos_array_factor, min_free_bytes,
                    member_size );
  }

static inline void FLZe_reset( FLZ_encoder * const fe,
                               const unsigned long long member_size )
  { LZeb_reset( &fe->eb, member_size ); }

static int FLZe_longest_match_len( FLZ_encoder * const fe, int * const distance )
  {
  enum { len_limit = 16 };
  int32_t * ptr0 = fe->eb.mb.pos_array + fe->eb.mb.cyclic_pos;
  const int available = min( Mb_available_bytes( &fe->eb.mb ), max_match_len );
  if( available < len_limit ) { *ptr0 = 0; return 0; }

  const uint8_t * const data = Mb_ptr_to_current_pos( &fe->eb.mb );
  fe->key4 = ( ( fe->key4 << 4 ) ^ data[3] ) & fe->eb.mb.key4_mask;
  const int pos1 = fe->eb.mb.pos + 1;
  int newpos1 = fe->eb.mb.prev_positions[fe->key4];
  fe->eb.mb.prev_positions[fe->key4] = pos1;
  int maxlen = 0, count;

  for( count = 4; ; )
    {
    int delta;
    if( newpos1 <= 0 || --count < 0 ||
        ( delta = pos1 - newpos1 ) > fe->eb.mb.dictionary_size )
      { *ptr0 = 0; break; }
    int32_t * const newptr = fe->eb.mb.pos_array +
      ( fe->eb.mb.cyclic_pos - delta +
        ( ( fe->eb.mb.cyclic_pos >= delta ) ? 0 : fe->eb.mb.dictionary_size + 1 ) );

    if( data[maxlen-delta] == data[maxlen] )
      {
      int len = 0;
      while( len < available && data[len-delta] == data[len] ) ++len;
      if( maxlen < len )
        { maxlen = len; *distance = delta - 1;
          if( maxlen >= len_limit ) { *ptr0 = *newptr; break; } }
      }

    *ptr0 = newpos1;
    ptr0 = newptr;
    newpos1 = *ptr0;
    }
  return maxlen;
  }


static bool FLZe_encode_member( FLZ_encoder * const fe )
  {
  int rep = 0, i;
  State * const state = &fe->eb.state;

  if( fe->eb.member_finished ) return true;
  if( Re_member_position( &fe->eb.renc ) >= fe->eb.member_size_limit )
    { LZeb_try_full_flush( &fe->eb ); return true; }

  if( Mb_data_position( &fe->eb.mb ) == 0 &&
      !Mb_data_finished( &fe->eb.mb ) )		/* encode first byte */
    {
    if( !Mb_enough_available_bytes( &fe->eb.mb ) ||
        !Re_enough_free_bytes( &fe->eb.renc ) ) return true;
    const uint8_t prev_byte = 0;
    const uint8_t cur_byte = Mb_peek( &fe->eb.mb, 0 );
    Re_encode_bit( &fe->eb.renc, &fe->eb.bm_match[*state][0], 0 );
    LZeb_encode_literal( &fe->eb, prev_byte, cur_byte );
    CRC32_update_byte( &fe->eb.crc, cur_byte );
    FLZe_reset_key4( fe );
    if( !FLZe_update_and_move( fe, 1 ) ) return false;
    }

  while( !Mb_data_finished( &fe->eb.mb ) &&
         Re_member_position( &fe->eb.renc ) < fe->eb.member_size_limit )
    {
    if( !Mb_enough_available_bytes( &fe->eb.mb ) ||
        !Re_enough_free_bytes( &fe->eb.renc ) ) return true;
    int match_distance = 0;		/* avoid warning from gcc 6.1.0 */
    const int main_len = FLZe_longest_match_len( fe, &match_distance );
    const int pos_state = Mb_data_position( &fe->eb.mb ) & pos_state_mask;
    int len = 0;

    for( i = 0; i < num_rep_distances; ++i )
      {
      const int tlen = Mb_true_match_len( &fe->eb.mb, 0, fe->eb.reps[i] + 1 );
      if( tlen > len ) { len = tlen; rep = i; }
      }
    if( len > min_match_len && len + 3 > main_len )
      {
      CRC32_update_buf( &fe->eb.crc, Mb_ptr_to_current_pos( &fe->eb.mb ), len );
      Re_encode_bit( &fe->eb.renc, &fe->eb.bm_match[*state][pos_state], 1 );
      Re_encode_bit( &fe->eb.renc, &fe->eb.bm_rep[*state], 1 );
      Re_encode_bit( &fe->eb.renc, &fe->eb.bm_rep0[*state], rep != 0 );
      if( rep == 0 )
        Re_encode_bit( &fe->eb.renc, &fe->eb.bm_len[*state][pos_state], 1 );
      else
        {
        Re_encode_bit( &fe->eb.renc, &fe->eb.bm_rep1[*state], rep > 1 );
        if( rep > 1 )
          Re_encode_bit( &fe->eb.renc, &fe->eb.bm_rep2[*state], rep > 2 );
        const int distance = fe->eb.reps[rep];
        for( i = rep; i > 0; --i ) fe->eb.reps[i] = fe->eb.reps[i-1];
        fe->eb.reps[0] = distance;
        }
      *state = St_set_rep( *state );
      Re_encode_len( &fe->eb.renc, &fe->eb.rep_len_model, len, pos_state );
      if( !Mb_move_pos( &fe->eb.mb ) ) return false;
      if( !FLZe_update_and_move( fe, len - 1 ) ) return false;
      continue;
      }

    if( main_len > min_match_len )
      {
      CRC32_update_buf( &fe->eb.crc, Mb_ptr_to_current_pos( &fe->eb.mb ), main_len );
      Re_encode_bit( &fe->eb.renc, &fe->eb.bm_match[*state][pos_state], 1 );
      Re_encode_bit( &fe->eb.renc, &fe->eb.bm_rep[*state], 0 );
      *state = St_set_match( *state );
      for( i = num_rep_distances - 1; i > 0; --i ) fe->eb.reps[i] = fe->eb.reps[i-1];
      fe->eb.reps[0] = match_distance;
      LZeb_encode_pair( &fe->eb, match_distance, main_len, pos_state );
      if( !Mb_move_pos( &fe->eb.mb ) ) return false;
      if( !FLZe_update_and_move( fe, main_len - 1 ) ) return false;
      continue;
      }

    const uint8_t prev_byte = Mb_peek( &fe->eb.mb, 1 );
    const uint8_t cur_byte = Mb_peek( &fe->eb.mb, 0 );
    const uint8_t match_byte = Mb_peek( &fe->eb.mb, fe->eb.reps[0] + 1 );
    if( !Mb_move_pos( &fe->eb.mb ) ) return false;
    CRC32_update_byte( &fe->eb.crc, cur_byte );

    if( match_byte == cur_byte )
      {
      const int shortrep_price = price1( fe->eb.bm_match[*state][pos_state] ) +
                                 price1( fe->eb.bm_rep[*state] ) +
                                 price0( fe->eb.bm_rep0[*state] ) +
                                 price0( fe->eb.bm_len[*state][pos_state] );
      int price = price0( fe->eb.bm_match[*state][pos_state] );
      if( St_is_char( *state ) )
        price += LZeb_price_literal( &fe->eb, prev_byte, cur_byte );
      else
        price += LZeb_price_matched( &fe->eb, prev_byte, cur_byte, match_byte );
      if( shortrep_price < price )
        {
        Re_encode_bit( &fe->eb.renc, &fe->eb.bm_match[*state][pos_state], 1 );
        Re_encode_bit( &fe->eb.renc, &fe->eb.bm_rep[*state], 1 );
        Re_encode_bit( &fe->eb.renc, &fe->eb.bm_rep0[*state], 0 );
        Re_encode_bit( &fe->eb.renc, &fe->eb.bm_len[*state][pos_state], 0 );
        *state = St_set_shortrep( *state );
        continue;
        }
      }

    /* literal byte */
    Re_encode_bit( &fe->eb.renc, &fe->eb.bm_match[*state][pos_state], 0 );
    if( ( *state = St_set_char( *state ) ) < 4 )
      LZeb_encode_literal( &fe->eb, prev_byte, cur_byte );
    else
      LZeb_encode_matched( &fe->eb, prev_byte, cur_byte, match_byte );
    }

  LZeb_try_full_flush( &fe->eb );
  return true;
  }



struct LZ_Encoder
  {
  unsigned long long partial_in_size;
  unsigned long long partial_out_size;
  LZ_encoder_base * lz_encoder_base;		/* these 3 pointers make a */
  LZ_encoder * lz_encoder;			/* polymorphic encoder */
  FLZ_encoder * flz_encoder;
  LZ_Errno lz_errno;
  bool fatal;
  };

static void LZ_Encoder_init( LZ_Encoder * const e )
  {
  e->partial_in_size = 0;
  e->partial_out_size = 0;
  e->lz_encoder_base = 0;
  e->lz_encoder = 0;
  e->flz_encoder = 0;
  e->lz_errno = LZ_ok;
  e->fatal = false;
  }


struct LZ_Decoder
  {
  unsigned long long partial_in_size;
  unsigned long long partial_out_size;
  Range_decoder * rdec;
  LZ_decoder * lz_decoder;
  LZ_Errno lz_errno;
  Lzip_header member_header;		/* header of current member */
  bool fatal;
  bool first_header;			/* true until first header is read */
  bool seeking;
  };

static void LZ_Decoder_init( LZ_Decoder * const d )
  {
  int i;
  d->partial_in_size = 0;
  d->partial_out_size = 0;
  d->rdec = 0;
  d->lz_decoder = 0;
  d->lz_errno = LZ_ok;
  for( i = 0; i < Lh_size; ++i ) d->member_header[i] = 0;
  d->fatal = false;
  d->first_header = true;
  d->seeking = false;
  }


static bool check_encoder( LZ_Encoder * const e )
  {
  if( !e ) return false;
  if( !e->lz_encoder_base || ( !e->lz_encoder && !e->flz_encoder ) ||
      ( e->lz_encoder && e->flz_encoder ) )
    { e->lz_errno = LZ_bad_argument; return false; }
  return true;
  }


static bool check_decoder( LZ_Decoder * const d )
  {
  if( !d ) return false;
  if( !d->rdec )
    { d->lz_errno = LZ_bad_argument; return false; }
  return true;
  }


/* ------------------------- Misc Functions ------------------------- */

int LZ_api_version( void ) { return LZ_API_VERSION; }

const char * LZ_version( void ) { return LZ_version_string; }

const char * LZ_strerror( const LZ_Errno lz_errno )
  {
  switch( lz_errno )
    {
    case LZ_ok            : return "ok";
    case LZ_bad_argument  : return "Bad argument";
    case LZ_mem_error     : return "Not enough memory";
    case LZ_sequence_error: return "Sequence error";
    case LZ_header_error  : return "Header error";
    case LZ_unexpected_eof: return "Unexpected EOF";
    case LZ_data_error    : return "Data error";
    case LZ_library_error : return "Library error";
    }
  return "Invalid error code";
  }


int LZ_min_dictionary_bits( void ) { return min_dictionary_bits; }
int LZ_min_dictionary_size( void ) { return min_dictionary_size; }
int LZ_max_dictionary_bits( void ) { return max_dictionary_bits; }
int LZ_max_dictionary_size( void ) { return max_dictionary_size; }
int LZ_min_match_len_limit( void ) { return min_match_len_limit; }
int LZ_max_match_len_limit( void ) { return max_match_len; }


/* --------------------- Compression Functions --------------------- */

LZ_Encoder * LZ_compress_open( const int dictionary_size,
                               const int match_len_limit,
                               const unsigned long long member_size )
  {
  Lzip_header header;
  LZ_Encoder * const e = (LZ_Encoder *)malloc( sizeof (LZ_Encoder) );
  if( !e ) return 0;
  LZ_Encoder_init( e );
  if( !Lh_set_dictionary_size( header, dictionary_size ) ||
      match_len_limit < min_match_len_limit ||
      match_len_limit > max_match_len ||
      member_size < min_dictionary_size )
    e->lz_errno = LZ_bad_argument;
  else
    {
    if( dictionary_size == 65535 && match_len_limit == 16 )
      {
      e->flz_encoder = (FLZ_encoder *)malloc( sizeof (FLZ_encoder) );
      if( e->flz_encoder && FLZe_init( e->flz_encoder, member_size ) )
        { e->lz_encoder_base = &e->flz_encoder->eb; return e; }
      free( e->flz_encoder ); e->flz_encoder = 0;
      }
    else
      {
      e->lz_encoder = (LZ_encoder *)malloc( sizeof (LZ_encoder) );
      if( e->lz_encoder && LZe_init( e->lz_encoder, Lh_get_dictionary_size( header ),
                                     match_len_limit, member_size ) )
        { e->lz_encoder_base = &e->lz_encoder->eb; return e; }
      free( e->lz_encoder ); e->lz_encoder = 0;
      }
    e->lz_errno = LZ_mem_error;
    }
  e->fatal = true;
  return e;
  }


int LZ_compress_close( LZ_Encoder * const e )
  {
  if( !e ) return -1;
  if( e->lz_encoder_base )
    { LZeb_free( e->lz_encoder_base );
      free( e->lz_encoder ); free( e->flz_encoder ); }
  free( e );
  return 0;
  }


int LZ_compress_finish( LZ_Encoder * const e )
  {
  if( !check_encoder( e ) || e->fatal ) return -1;
  Mb_finish( &e->lz_encoder_base->mb );
  /* if (open --> write --> finish) use same dictionary size as lzip. */
  /* this does not save any memory. */
  if( Mb_data_position( &e->lz_encoder_base->mb ) == 0 &&
      Re_member_position( &e->lz_encoder_base->renc ) == Lh_size )
    {
    Mb_adjust_dictionary_size( &e->lz_encoder_base->mb );
    Lh_set_dictionary_size( e->lz_encoder_base->renc.header,
                            e->lz_encoder_base->mb.dictionary_size );
    e->lz_encoder_base->renc.cb.buffer[5] = e->lz_encoder_base->renc.header[5];
    }
  return 0;
  }


int LZ_compress_restart_member( LZ_Encoder * const e,
                                const unsigned long long member_size )
  {
  if( !check_encoder( e ) || e->fatal ) return -1;
  if( !LZeb_member_finished( e->lz_encoder_base ) )
    { e->lz_errno = LZ_sequence_error; return -1; }
  if( member_size < min_dictionary_size )
    { e->lz_errno = LZ_bad_argument; return -1; }

  e->partial_in_size += Mb_data_position( &e->lz_encoder_base->mb );
  e->partial_out_size += Re_member_position( &e->lz_encoder_base->renc );

  if( e->lz_encoder ) LZe_reset( e->lz_encoder, member_size );
  else FLZe_reset( e->flz_encoder, member_size );
  e->lz_errno = LZ_ok;
  return 0;
  }


int LZ_compress_sync_flush( LZ_Encoder * const e )
  {
  if( !check_encoder( e ) || e->fatal ) return -1;
  if( !e->lz_encoder_base->mb.at_stream_end )
    e->lz_encoder_base->mb.sync_flush_pending = true;
  return 0;
  }


int LZ_compress_read( LZ_Encoder * const e,
                      uint8_t * const buffer, const int size )
  {
  if( !check_encoder( e ) || e->fatal ) return -1;
  if( size < 0 ) return 0;

  { LZ_encoder_base * const eb = e->lz_encoder_base;
  int out_size = Re_read_data( &eb->renc, buffer, size );
  /* minimize number of calls to encode_member */
  if( out_size < size || size == 0 )
    {
    if( ( e->flz_encoder && !FLZe_encode_member( e->flz_encoder ) ) ||
        ( e->lz_encoder && !LZe_encode_member( e->lz_encoder ) ) )
      { e->lz_errno = LZ_library_error; e->fatal = true; return -1; }
    if( eb->mb.sync_flush_pending && Mb_available_bytes( &eb->mb ) <= 0 )
      LZeb_try_sync_flush( eb );
    out_size += Re_read_data( &eb->renc, buffer + out_size, size - out_size );
    }
  return out_size; }
  }


int LZ_compress_write( LZ_Encoder * const e,
                       const uint8_t * const buffer, const int size )
  {
  if( !check_encoder( e ) || e->fatal ) return -1;
  return Mb_write_data( &e->lz_encoder_base->mb, buffer, size );
  }


int LZ_compress_write_size( LZ_Encoder * const e )
  {
  if( !check_encoder( e ) || e->fatal ) return -1;
  return Mb_free_bytes( &e->lz_encoder_base->mb );
  }


LZ_Errno LZ_compress_errno( LZ_Encoder * const e )
  {
  if( !e ) return LZ_bad_argument;
  return e->lz_errno;
  }


int LZ_compress_finished( LZ_Encoder * const e )
  {
  if( !check_encoder( e ) ) return -1;
  return Mb_data_finished( &e->lz_encoder_base->mb ) &&
         LZeb_member_finished( e->lz_encoder_base );
  }


int LZ_compress_member_finished( LZ_Encoder * const e )
  {
  if( !check_encoder( e ) ) return -1;
  return LZeb_member_finished( e->lz_encoder_base );
  }


unsigned long long LZ_compress_data_position( LZ_Encoder * const e )
  {
  if( !check_encoder( e ) ) return 0;
  return Mb_data_position( &e->lz_encoder_base->mb );
  }


unsigned long long LZ_compress_member_position( LZ_Encoder * const e )
  {
  if( !check_encoder( e ) ) return 0;
  return Re_member_position( &e->lz_encoder_base->renc );
  }


unsigned long long LZ_compress_total_in_size( LZ_Encoder * const e )
  {
  if( !check_encoder( e ) ) return 0;
  return e->partial_in_size + Mb_data_position( &e->lz_encoder_base->mb );
  }


unsigned long long LZ_compress_total_out_size( LZ_Encoder * const e )
  {
  if( !check_encoder( e ) ) return 0;
  return e->partial_out_size + Re_member_position( &e->lz_encoder_base->renc );
  }


/* -------------------- Decompression Functions -------------------- */

LZ_Decoder * LZ_decompress_open( void )
  {
  LZ_Decoder * const d = (LZ_Decoder *)malloc( sizeof (LZ_Decoder) );
  if( !d ) return 0;
  LZ_Decoder_init( d );

  d->rdec = (Range_decoder *)malloc( sizeof (Range_decoder) );
  if( !d->rdec || !Rd_init( d->rdec ) )
    {
    if( d->rdec ) { Rd_free( d->rdec ); free( d->rdec ); d->rdec = 0; }
    d->lz_errno = LZ_mem_error; d->fatal = true;
    }
  return d;
  }


int LZ_decompress_close( LZ_Decoder * const d )
  {
  if( !d ) return -1;
  if( d->lz_decoder )
    { LZd_free( d->lz_decoder ); free( d->lz_decoder ); }
  if( d->rdec ) { Rd_free( d->rdec ); free( d->rdec ); }
  free( d );
  return 0;
  }


int LZ_decompress_finish( LZ_Decoder * const d )
  {
  if( !check_decoder( d ) || d->fatal ) return -1;
  if( d->seeking )
    { d->seeking = false; d->partial_in_size += Rd_purge( d->rdec ); }
  else Rd_finish( d->rdec );
  return 0;
  }


int LZ_decompress_reset( LZ_Decoder * const d )
  {
  if( !check_decoder( d ) ) return -1;
  if( d->lz_decoder )
    { LZd_free( d->lz_decoder ); free( d->lz_decoder ); d->lz_decoder = 0; }
  d->partial_in_size = 0;
  d->partial_out_size = 0;
  Rd_reset( d->rdec );
  d->lz_errno = LZ_ok;
  d->fatal = false;
  d->first_header = true;
  d->seeking = false;
  return 0;
  }


int LZ_decompress_sync_to_member( LZ_Decoder * const d )
  {
  unsigned skipped = 0;
  if( !check_decoder( d ) ) return -1;
  if( d->lz_decoder )
    { LZd_free( d->lz_decoder ); free( d->lz_decoder ); d->lz_decoder = 0; }
  if( Rd_find_header( d->rdec, &skipped ) ) d->seeking = false;
  else
    {
    if( !d->rdec->at_stream_end ) d->seeking = true;
    else { d->seeking = false; d->partial_in_size += Rd_purge( d->rdec ); }
    }
  d->partial_in_size += skipped;
  d->lz_errno = LZ_ok;
  d->fatal = false;
  return 0;
  }


int LZ_decompress_read( LZ_Decoder * const d,
                        uint8_t * const buffer, const int size )
  {
  int result;
  if( !check_decoder( d ) ) return -1;
  if( size < 0 ) return 0;
  if( d->fatal )	/* don't return error until pending bytes are read */
    { if( d->lz_decoder && !Cb_empty( &d->lz_decoder->cb ) ) goto get_data;
      return -1; }
  if( d->seeking ) return 0;

  if( d->lz_decoder && LZd_member_finished( d->lz_decoder ) )
    {
    d->partial_out_size += LZd_data_position( d->lz_decoder );
    LZd_free( d->lz_decoder ); free( d->lz_decoder ); d->lz_decoder = 0;
    }
  if( !d->lz_decoder )
    {
    int rd;
    d->partial_in_size += d->rdec->member_position;
    d->rdec->member_position = 0;
    if( Rd_available_bytes( d->rdec ) < Lh_size + 5 &&
        !d->rdec->at_stream_end ) return 0;
    if( Rd_finished( d->rdec ) && !d->first_header ) return 0;
    rd = Rd_read_data( d->rdec, d->member_header, Lh_size );
    if( rd < Lh_size || Rd_finished( d->rdec ) )	/* End Of File */
      {
      if( rd <= 0 || Lh_check_prefix( d->member_header, rd ) )
        d->lz_errno = LZ_unexpected_eof;
      else
        d->lz_errno = LZ_header_error;
      d->fatal = true;
      return -1;
      }
    if( !Lh_check_magic( d->member_header ) )
      {
      /* unreading the header prevents sync_to_member from skipping a member
         if leading garbage is shorter than a full header; "lgLZIP\x01\x0C" */
      if( Rd_unread_data( d->rdec, rd ) )
        {
        if( d->first_header || !Lh_check_corrupt( d->member_header ) )
          d->lz_errno = LZ_header_error;
        else
          d->lz_errno = LZ_data_error;		/* corrupt header */
        }
      else
        d->lz_errno = LZ_library_error;
      d->fatal = true;
      return -1;
      }
    if( !Lh_check_version( d->member_header ) ||
        !isvalid_ds( Lh_get_dictionary_size( d->member_header ) ) )
      {
      /* Skip a possible "LZIP" leading garbage; "LZIPLZIP\x01\x0C".
         Leave member_pos pointing to the first error. */
      if( Rd_unread_data( d->rdec, 1 + !Lh_check_version( d->member_header ) ) )
        d->lz_errno = LZ_data_error;	/* bad version or bad dict size */
      else
        d->lz_errno = LZ_library_error;
      d->fatal = true;
      return -1;
      }
    d->first_header = false;
    if( Rd_available_bytes( d->rdec ) < 5 )
      {
      /* set position at EOF */
      d->rdec->member_position += Cb_used_bytes( &d->rdec->cb );
      Cb_reset( &d->rdec->cb );
      d->lz_errno = LZ_unexpected_eof;
      d->fatal = true;
      return -1;
      }
    d->lz_decoder = (LZ_decoder *)malloc( sizeof (LZ_decoder) );
    if( !d->lz_decoder || !LZd_init( d->lz_decoder, d->rdec,
                             Lh_get_dictionary_size( d->member_header ) ) )
      {					/* not enough free memory */
      if( d->lz_decoder )
        { LZd_free( d->lz_decoder ); free( d->lz_decoder ); d->lz_decoder = 0; }
      d->lz_errno = LZ_mem_error;
      d->fatal = true;
      return -1;
      }
    d->rdec->reload_pending = true;
    }
  result = LZd_decode_member( d->lz_decoder );
  if( result != 0 )
    {
    if( result == 2 )			/* set input position at EOF */
      { d->rdec->member_position += Cb_used_bytes( &d->rdec->cb );
        Cb_reset( &d->rdec->cb );
        d->lz_errno = LZ_unexpected_eof; }
    else if( result == 6 ) d->lz_errno = LZ_library_error;
    else d->lz_errno = LZ_data_error;
    d->fatal = true;
    if( Cb_empty( &d->lz_decoder->cb ) ) return -1;
    }
get_data:
  return Cb_read_data( &d->lz_decoder->cb, buffer, size );
  }


int LZ_decompress_write( LZ_Decoder * const d,
                         const uint8_t * const buffer, const int size )
  {
  int result;
  if( !check_decoder( d ) || d->fatal ) return -1;
  if( size < 0 ) return 0;

  result = Rd_write_data( d->rdec, buffer, size );
  while( d->seeking )
    {
    int size2;
    unsigned skipped = 0;
    if( Rd_find_header( d->rdec, &skipped ) ) d->seeking = false;
    d->partial_in_size += skipped;
    if( result >= size ) break;
    size2 = Rd_write_data( d->rdec, buffer + result, size - result );
    if( size2 > 0 ) result += size2;
    else break;
    }
  return result;
  }


int LZ_decompress_write_size( LZ_Decoder * const d )
  {
  if( !check_decoder( d ) || d->fatal ) return -1;
  return Rd_free_bytes( d->rdec );
  }


LZ_Errno LZ_decompress_errno( LZ_Decoder * const d )
  {
  if( !d ) return LZ_bad_argument;
  return d->lz_errno;
  }


int LZ_decompress_finished( LZ_Decoder * const d )
  {
  if( !check_decoder( d ) || d->fatal ) return -1;
  return Rd_finished( d->rdec ) &&
         ( !d->lz_decoder || LZd_member_finished( d->lz_decoder ) );
  }


int LZ_decompress_member_finished( LZ_Decoder * const d )
  {
  if( !check_decoder( d ) || d->fatal ) return -1;
  return d->lz_decoder && LZd_member_finished( d->lz_decoder );
  }


int LZ_decompress_member_version( LZ_Decoder * const d )
  {
  if( !check_decoder( d ) ) return -1;
  return Lh_version( d->member_header );
  }


int LZ_decompress_dictionary_size( LZ_Decoder * const d )
  {
  if( !check_decoder( d ) ) return -1;
  return Lh_get_dictionary_size( d->member_header );
  }


unsigned LZ_decompress_data_crc( LZ_Decoder * const d )
  {
  if( check_decoder( d ) && d->lz_decoder )
    return LZd_crc( d->lz_decoder );
  return 0;
  }


unsigned long long LZ_decompress_data_position( LZ_Decoder * const d )
  {
  if( check_decoder( d ) && d->lz_decoder )
    return LZd_data_position( d->lz_decoder );
  return 0;
  }


unsigned long long LZ_decompress_member_position( LZ_Decoder * const d )
  {
  if( !check_decoder( d ) ) return 0;
  return d->rdec->member_position;
  }


unsigned long long LZ_decompress_total_in_size( LZ_Decoder * const d )
  {
  if( !check_decoder( d ) ) return 0;
  return d->partial_in_size + d->rdec->member_position;
  }


unsigned long long LZ_decompress_total_out_size( LZ_Decoder * const d )
  {
  if( !check_decoder( d ) ) return 0;
  if( d->lz_decoder )
    return d->partial_out_size + LZd_data_position( d->lz_decoder );
  return d->partial_out_size;
  }
