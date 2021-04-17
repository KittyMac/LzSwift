/* Lzlib - Compression library for the lzip format
   Copyright (C) 2009-2021 Antonio Diaz Diaz.

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

static const char * const LZ_version_string = "1.12";

enum LZ_Errno { LZ_ok = 0, LZ_bad_argument, LZ_mem_error,
                LZ_sequence_error, LZ_header_error, LZ_unexpected_eof,
                LZ_data_error, LZ_library_error };


int LZ_api_version( void );
const char * LZ_version( void );
const char * LZ_strerror( const enum LZ_Errno lz_errno );

int LZ_min_dictionary_bits( void );
int LZ_min_dictionary_size( void );
int LZ_max_dictionary_bits( void );
int LZ_max_dictionary_size( void );
int LZ_min_match_len_limit( void );
int LZ_max_match_len_limit( void );




struct LZ_Encoder;

struct LZ_Encoder * LZ_compress_open( const int dictionary_size,
                                      const int match_len_limit,
                                      const unsigned long long member_size );
int LZ_compress_close( struct LZ_Encoder * const encoder );

int LZ_compress_finish( struct LZ_Encoder * const encoder );
int LZ_compress_restart_member( struct LZ_Encoder * const encoder,
                                const unsigned long long member_size );
int LZ_compress_sync_flush( struct LZ_Encoder * const encoder );

int LZ_compress_read( struct LZ_Encoder * const encoder,
                      uint8_t * const buffer, const int size );
int LZ_compress_write( struct LZ_Encoder * const encoder,
                       const uint8_t * const buffer, const int size );
int LZ_compress_write_size( struct LZ_Encoder * const encoder );

enum LZ_Errno LZ_compress_errno( struct LZ_Encoder * const encoder );
int LZ_compress_finished( struct LZ_Encoder * const encoder );
int LZ_compress_member_finished( struct LZ_Encoder * const encoder );

unsigned long long LZ_compress_data_position( struct LZ_Encoder * const encoder );
unsigned long long LZ_compress_member_position( struct LZ_Encoder * const encoder );
unsigned long long LZ_compress_total_in_size( struct LZ_Encoder * const encoder );
unsigned long long LZ_compress_total_out_size( struct LZ_Encoder * const encoder );




struct LZ_Decoder;

struct LZ_Decoder * LZ_decompress_open( void );
int LZ_decompress_close( struct LZ_Decoder * const decoder );

int LZ_decompress_finish( struct LZ_Decoder * const decoder );
int LZ_decompress_reset( struct LZ_Decoder * const decoder );
int LZ_decompress_sync_to_member( struct LZ_Decoder * const decoder );

int LZ_decompress_read( struct LZ_Decoder * const decoder,
                        uint8_t * const buffer, const int size );
int LZ_decompress_write( struct LZ_Decoder * const decoder,
                         const uint8_t * const buffer, const int size );
int LZ_decompress_write_size( struct LZ_Decoder * const decoder );

enum LZ_Errno LZ_decompress_errno( struct LZ_Decoder * const decoder );
int LZ_decompress_finished( struct LZ_Decoder * const decoder );
int LZ_decompress_member_finished( struct LZ_Decoder * const decoder );

int LZ_decompress_member_version( struct LZ_Decoder * const decoder );
int LZ_decompress_dictionary_size( struct LZ_Decoder * const decoder );
unsigned LZ_decompress_data_crc( struct LZ_Decoder * const decoder );

unsigned long long LZ_decompress_data_position( struct LZ_Decoder * const decoder );
unsigned long long LZ_decompress_member_position( struct LZ_Decoder * const decoder );
unsigned long long LZ_decompress_total_in_size( struct LZ_Decoder * const decoder );
unsigned long long LZ_decompress_total_out_size( struct LZ_Decoder * const decoder );

typedef int State;

enum { states = 12 };

static inline _Bool St_is_char( const State st ) { return st < 7; }

static inline State St_set_char( const State st )
  {
  static const State next[states] = { 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 4, 5 };
  return next[st];
  }

static inline State St_set_char_rep() { return 8; }

static inline State St_set_match( const State st )
  { return ( ( st < 7 ) ? 7 : 10 ); }

static inline State St_set_rep( const State st )
  { return ( ( st < 7 ) ? 8 : 11 ); }

static inline State St_set_short_rep( const State st )
  { return ( ( st < 7 ) ? 9 : 11 ); }


enum {
  min_dictionary_bits = 12,
  min_dictionary_size = 1 << min_dictionary_bits,
  max_dictionary_bits = 29,
  max_dictionary_size = 1 << max_dictionary_bits,
  literal_context_bits = 3,
  literal_pos_state_bits = 0,
  pos_state_bits = 2,
  pos_states = 1 << pos_state_bits,
  pos_state_mask = pos_states - 1,

  len_states = 4,
  dis_slot_bits = 6,
  start_dis_model = 4,
  end_dis_model = 14,
  modeled_distances = 1 << (end_dis_model / 2),
  dis_align_bits = 4,
  dis_align_size = 1 << dis_align_bits,

  len_low_bits = 3,
  len_mid_bits = 3,
  len_high_bits = 8,
  len_low_symbols = 1 << len_low_bits,
  len_mid_symbols = 1 << len_mid_bits,
  len_high_symbols = 1 << len_high_bits,
  max_len_symbols = len_low_symbols + len_mid_symbols + len_high_symbols,

  min_match_len = 2,
  max_match_len = min_match_len + max_len_symbols - 1,
  min_match_len_limit = 5 };

static inline int get_len_state( const int len )
  { return ((len - min_match_len) <= (len_states - 1) ? (len - min_match_len) : (len_states - 1)); }

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

struct Len_model
  {
  Bit_model choice1;
  Bit_model choice2;
  Bit_model bm_low[pos_states][len_low_symbols];
  Bit_model bm_mid[pos_states][len_mid_symbols];
  Bit_model bm_high[len_high_symbols];
  };

static inline void Lm_init( struct Len_model * const lm )
  {
  Bm_init( &lm->choice1 );
  Bm_init( &lm->choice2 );
  Bm_array_init( lm->bm_low[0], pos_states * len_low_symbols );
  Bm_array_init( lm->bm_mid[0], pos_states * len_mid_symbols );
  Bm_array_init( lm->bm_high, len_high_symbols );
  }



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

static inline void CRC32_update_buf( uint32_t * const crc,
                                     const uint8_t * const buffer,
                                     const int size )
  {
  int i;
  uint32_t c = *crc;
  for( i = 0; i < size; ++i )
    c = crc32[(c^buffer[i])&0xFF] ^ ( c >> 8 );
  *crc = c;
  }


static inline _Bool isvalid_ds( const unsigned dictionary_size )
  { return ( dictionary_size >= min_dictionary_size &&
             dictionary_size <= max_dictionary_size ); }


static inline int real_bits( unsigned value )
  {
  int bits = 0;
  while( value > 0 ) { value >>= 1; ++bits; }
  return bits;
  }


static const uint8_t lzip_magic[4] = { 0x4C, 0x5A, 0x49, 0x50 };

typedef uint8_t Lzip_header[6];


enum { Lh_size = 6 };

static inline void Lh_set_magic( Lzip_header data )
  { __builtin___memcpy_chk (data, lzip_magic, 4, __builtin_object_size (data, 0)); data[4] = 1; }

static inline _Bool Lh_verify_magic( const Lzip_header data )
  { return ( memcmp( data, lzip_magic, 4 ) == 0 ); }


static inline _Bool Lh_verify_prefix( const Lzip_header data, const int sz )
  {
  int i; for( i = 0; i < sz && i < 4; ++i )
    if( data[i] != lzip_magic[i] ) return 0;
  return ( sz > 0 );
  }


static inline _Bool Lh_verify_corrupt( const Lzip_header data )
  {
  int matches = 0;
  int i; for( i = 0; i < 4; ++i )
    if( data[i] == lzip_magic[i] ) ++matches;
  return ( matches > 1 && matches < 4 );
  }

static inline uint8_t Lh_version( const Lzip_header data )
  { return data[4]; }

static inline _Bool Lh_verify_version( const Lzip_header data )
  { return ( data[4] == 1 ); }

static inline unsigned Lh_get_dictionary_size( const Lzip_header data )
  {
  unsigned sz = ( 1 << ( data[5] & 0x1F ) );
  if( sz > min_dictionary_size )
    sz -= ( sz / 16 ) * ( ( data[5] >> 5 ) & 7 );
  return sz;
  }

static inline _Bool Lh_set_dictionary_size( Lzip_header data, const unsigned sz )
  {
  if( !isvalid_ds( sz ) ) return 0;
  data[5] = real_bits( sz - 1 );
  if( sz > min_dictionary_size )
    {
    const unsigned base_size = 1 << data[5];
    const unsigned fraction = base_size / 16;
    unsigned i;
    for( i = 7; i >= 1; --i )
      if( base_size - ( i * fraction ) >= sz )
        { data[5] |= ( i << 5 ); break; }
    }
  return 1;
  }

static inline _Bool Lh_verify( const Lzip_header data )
  {
  return Lh_verify_magic( data ) && Lh_verify_version( data ) &&
         isvalid_ds( Lh_get_dictionary_size( data ) );
  }


typedef uint8_t Lzip_trailer[20];



enum { Lt_size = 20 };

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

struct Circular_buffer
  {
  uint8_t * buffer;
  unsigned buffer_size;
  unsigned get;
  unsigned put;
  };

static inline _Bool Cb_init( struct Circular_buffer * const cb,
                            const unsigned buf_size )
  {
  cb->buffer_size = buf_size + 1;
  cb->get = 0;
  cb->put = 0;
  cb->buffer =
    ( cb->buffer_size > 1 ) ? (uint8_t *)malloc( cb->buffer_size ) : 0;
  return ( cb->buffer != 0 );
  }

static inline void Cb_free( struct Circular_buffer * const cb )
  { free( cb->buffer ); cb->buffer = 0; }

static inline void Cb_reset( struct Circular_buffer * const cb )
  { cb->get = 0; cb->put = 0; }

static inline unsigned Cb_empty( const struct Circular_buffer * const cb )
  { return cb->get == cb->put; }

static inline unsigned Cb_used_bytes( const struct Circular_buffer * const cb )
  { return ( (cb->get <= cb->put) ? 0 : cb->buffer_size ) + cb->put - cb->get; }

static inline unsigned Cb_free_bytes( const struct Circular_buffer * const cb )
  { return ( (cb->get <= cb->put) ? cb->buffer_size : 0 ) - cb->put + cb->get - 1; }

static inline uint8_t Cb_get_byte( struct Circular_buffer * const cb )
  {
  const uint8_t b = cb->buffer[cb->get];
  if( ++cb->get >= cb->buffer_size ) cb->get = 0;
  return b;
  }

static inline void Cb_put_byte( struct Circular_buffer * const cb,
                                const uint8_t b )
  {
  cb->buffer[cb->put] = b;
  if( ++cb->put >= cb->buffer_size ) cb->put = 0;
  }


static _Bool Cb_unread_data( struct Circular_buffer * const cb,
                            const unsigned size )
  {
  if( size > Cb_free_bytes( cb ) ) return 0;
  if( cb->get >= size ) cb->get -= size;
  else cb->get = cb->buffer_size - size + cb->get;
  return 1;
  }






static unsigned Cb_read_data( struct Circular_buffer * const cb,
                              uint8_t * const out_buffer,
                              const unsigned out_size )
  {
  unsigned size = 0;
  if( out_size == 0 ) return 0;
  if( cb->get > cb->put )
    {
    size = ((cb->buffer_size - cb->get) <= (out_size) ? (cb->buffer_size - cb->get) : (out_size));
    if( size > 0 )
      {
      if( out_buffer ) __builtin___memcpy_chk (out_buffer, cb->buffer + cb->get, size, __builtin_object_size (out_buffer, 0));
      cb->get += size;
      if( cb->get >= cb->buffer_size ) cb->get = 0;
      }
    }
  if( cb->get < cb->put )
    {
    const unsigned size2 = ((cb->put - cb->get) <= (out_size - size) ? (cb->put - cb->get) : (out_size - size));
    if( size2 > 0 )
      {
      if( out_buffer ) __builtin___memcpy_chk (out_buffer + size, cb->buffer + cb->get, size2, __builtin_object_size (out_buffer + size, 0));
      cb->get += size2;
      size += size2;
      }
    }
  return size;
  }





static unsigned Cb_write_data( struct Circular_buffer * const cb,
                               const uint8_t * const in_buffer,
                               const unsigned in_size )
  {
  unsigned size = 0;
  if( in_size == 0 ) return 0;
  if( cb->put >= cb->get )
    {
    size = ((cb->buffer_size - cb->put - (cb->get == 0)) <= (in_size) ? (cb->buffer_size - cb->put - (cb->get == 0)) : (in_size));
    if( size > 0 )
      {
      __builtin___memcpy_chk (cb->buffer + cb->put, in_buffer, size, __builtin_object_size (cb->buffer + cb->put, 0));
      cb->put += size;
      if( cb->put >= cb->buffer_size ) cb->put = 0;
      }
    }
  if( cb->put < cb->get )
    {
    const unsigned size2 = ((cb->get - cb->put - 1) <= (in_size - size) ? (cb->get - cb->put - 1) : (in_size - size));
    if( size2 > 0 )
      {
      __builtin___memcpy_chk (cb->buffer + cb->put, in_buffer + size, size2, __builtin_object_size (cb->buffer + cb->put, 0));
      cb->put += size2;
      size += size2;
      }
    }
  return size;
  }

enum { rd_min_available_bytes = 10 };

struct Range_decoder
  {
  struct Circular_buffer cb;
  unsigned long long member_position;
  uint32_t code;
  uint32_t range;
  _Bool at_stream_end;
  _Bool reload_pending;
  };

static inline _Bool Rd_init( struct Range_decoder * const rdec )
  {
  if( !Cb_init( &rdec->cb, 65536 + rd_min_available_bytes ) ) return 0;
  rdec->member_position = 0;
  rdec->code = 0;
  rdec->range = 0xFFFFFFFFU;
  rdec->at_stream_end = 0;
  rdec->reload_pending = 0;
  return 1;
  }

static inline void Rd_free( struct Range_decoder * const rdec )
  { Cb_free( &rdec->cb ); }

static inline _Bool Rd_finished( const struct Range_decoder * const rdec )
  { return rdec->at_stream_end && Cb_empty( &rdec->cb ); }

static inline void Rd_finish( struct Range_decoder * const rdec )
  { rdec->at_stream_end = 1; }

static inline _Bool Rd_enough_available_bytes( const struct Range_decoder * const rdec )
  { return ( Cb_used_bytes( &rdec->cb ) >= rd_min_available_bytes ); }

static inline unsigned Rd_available_bytes( const struct Range_decoder * const rdec )
  { return Cb_used_bytes( &rdec->cb ); }

static inline unsigned Rd_free_bytes( const struct Range_decoder * const rdec )
  { return rdec->at_stream_end ? 0 : Cb_free_bytes( &rdec->cb ); }

static inline unsigned long long Rd_purge( struct Range_decoder * const rdec )
  {
  const unsigned long long size =
    rdec->member_position + Cb_used_bytes( &rdec->cb );
  Cb_reset( &rdec->cb );
  rdec->member_position = 0; rdec->at_stream_end = 1;
  return size;
  }

static inline void Rd_reset( struct Range_decoder * const rdec )
  { Cb_reset( &rdec->cb );
    rdec->member_position = 0; rdec->at_stream_end = 0; }





static _Bool Rd_find_header( struct Range_decoder * const rdec,
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
        if( get == rdec->cb.put ) return 0;
        header[i] = rdec->cb.buffer[get];
        if( ++get >= rdec->cb.buffer_size ) get = 0;
        }
      if( Lh_verify( header ) ) return 1;
      }
    if( ++rdec->cb.get >= rdec->cb.buffer_size ) rdec->cb.get = 0;
    ++*skippedp;
    }
  return 0;
  }


static inline int Rd_write_data( struct Range_decoder * const rdec,
                                 const uint8_t * const inbuf, const int size )
  {
  if( rdec->at_stream_end || size <= 0 ) return 0;
  return Cb_write_data( &rdec->cb, inbuf, size );
  }

static inline uint8_t Rd_get_byte( struct Range_decoder * const rdec )
  {

  if( Rd_finished( rdec ) ) return 0xFF;
  ++rdec->member_position;
  return Cb_get_byte( &rdec->cb );
  }

static inline int Rd_read_data( struct Range_decoder * const rdec,
                                uint8_t * const outbuf, const int size )
  {
  const int sz = Cb_read_data( &rdec->cb, outbuf, size );
  if( sz > 0 ) rdec->member_position += sz;
  return sz;
  }

static inline _Bool Rd_unread_data( struct Range_decoder * const rdec,
                                   const unsigned size )
  {
  if( size > rdec->member_position || !Cb_unread_data( &rdec->cb, size ) )
    return 0;
  rdec->member_position -= size;
  return 1;
  }

static _Bool Rd_try_reload( struct Range_decoder * const rdec )
  {
  if( rdec->reload_pending && Rd_available_bytes( rdec ) >= 5 )
    {
    int i;
    rdec->reload_pending = 0;
    rdec->code = 0;
    for( i = 0; i < 5; ++i )
      rdec->code = (rdec->code << 8) | Rd_get_byte( rdec );
    rdec->range = 0xFFFFFFFFU;
    rdec->code &= rdec->range;
    }
  return !rdec->reload_pending;
  }

static inline void Rd_normalize( struct Range_decoder * const rdec )
  {
  if( rdec->range <= 0x00FFFFFFU )
    { rdec->range <<= 8; rdec->code = (rdec->code << 8) | Rd_get_byte( rdec ); }
  }

static inline unsigned Rd_decode( struct Range_decoder * const rdec,
                                  const int num_bits )
  {
  unsigned symbol = 0;
  int i;
  for( i = num_bits; i > 0; --i )
    {
    _Bool bit;
    Rd_normalize( rdec );
    rdec->range >>= 1;


    bit = ( rdec->code >= rdec->range );
    symbol <<= 1; symbol += bit;
    rdec->code -= rdec->range & ( 0U - bit );
    }
  return symbol;
  }

static inline unsigned Rd_decode_bit( struct Range_decoder * const rdec,
                                      Bit_model * const probability )
  {
  uint32_t bound;
  Rd_normalize( rdec );
  bound = ( rdec->range >> bit_model_total_bits ) * *probability;
  if( rdec->code < bound )
    {
    *probability += (bit_model_total - *probability) >> bit_model_move_bits;
    rdec->range = bound;
    return 0;
    }
  else
    {
    *probability -= *probability >> bit_model_move_bits;
    rdec->code -= bound;
    rdec->range -= bound;
    return 1;
    }
  }

static inline unsigned Rd_decode_tree3( struct Range_decoder * const rdec,
                                        Bit_model bm[] )
  {
  unsigned symbol = 2 | Rd_decode_bit( rdec, &bm[1] );
  symbol = ( symbol << 1 ) | Rd_decode_bit( rdec, &bm[symbol] );
  symbol = ( symbol << 1 ) | Rd_decode_bit( rdec, &bm[symbol] );
  return symbol & 7;
  }

static inline unsigned Rd_decode_tree6( struct Range_decoder * const rdec,
                                        Bit_model bm[] )
  {
  unsigned symbol = 2 | Rd_decode_bit( rdec, &bm[1] );
  symbol = ( symbol << 1 ) | Rd_decode_bit( rdec, &bm[symbol] );
  symbol = ( symbol << 1 ) | Rd_decode_bit( rdec, &bm[symbol] );
  symbol = ( symbol << 1 ) | Rd_decode_bit( rdec, &bm[symbol] );
  symbol = ( symbol << 1 ) | Rd_decode_bit( rdec, &bm[symbol] );
  symbol = ( symbol << 1 ) | Rd_decode_bit( rdec, &bm[symbol] );
  return symbol & 0x3F;
  }

static inline unsigned Rd_decode_tree8( struct Range_decoder * const rdec,
                                        Bit_model bm[] )
  {
  unsigned symbol = 1;
  int i;
  for( i = 0; i < 8; ++i )
    symbol = ( symbol << 1 ) | Rd_decode_bit( rdec, &bm[symbol] );
  return symbol & 0xFF;
  }

static inline unsigned
Rd_decode_tree_reversed( struct Range_decoder * const rdec,
                         Bit_model bm[], const int num_bits )
  {
  unsigned model = 1;
  unsigned symbol = 0;
  int i;
  for( i = 0; i < num_bits; ++i )
    {
    const unsigned bit = Rd_decode_bit( rdec, &bm[model] );
    model <<= 1; model += bit;
    symbol |= ( bit << i );
    }
  return symbol;
  }

static inline unsigned
Rd_decode_tree_reversed4( struct Range_decoder * const rdec, Bit_model bm[] )
  {
  unsigned symbol = Rd_decode_bit( rdec, &bm[1] );
  symbol += Rd_decode_bit( rdec, &bm[2+symbol] ) << 1;
  symbol += Rd_decode_bit( rdec, &bm[4+symbol] ) << 2;
  symbol += Rd_decode_bit( rdec, &bm[8+symbol] ) << 3;
  return symbol;
  }

static inline unsigned Rd_decode_matched( struct Range_decoder * const rdec,
                                          Bit_model bm[], unsigned match_byte )
  {
  unsigned symbol = 1;
  unsigned mask = 0x100;
  while( 1 )
    {
    const unsigned match_bit = ( match_byte <<= 1 ) & mask;
    const unsigned bit = Rd_decode_bit( rdec, &bm[symbol+match_bit+mask] );
    symbol <<= 1; symbol += bit;
    if( symbol > 0xFF ) return symbol & 0xFF;
    mask &= ~(match_bit ^ (bit << 8));
    }
  }

static inline unsigned Rd_decode_len( struct Range_decoder * const rdec,
                                      struct Len_model * const lm,
                                      const int pos_state )
  {
  if( Rd_decode_bit( rdec, &lm->choice1 ) == 0 )
    return Rd_decode_tree3( rdec, lm->bm_low[pos_state] );
  if( Rd_decode_bit( rdec, &lm->choice2 ) == 0 )
    return len_low_symbols + Rd_decode_tree3( rdec, lm->bm_mid[pos_state] );
  return len_low_symbols + len_mid_symbols + Rd_decode_tree8( rdec, lm->bm_high );
  }


enum { lzd_min_free_bytes = max_match_len };

struct LZ_decoder
  {
  struct Circular_buffer cb;
  unsigned long long partial_data_pos;
  struct Range_decoder * rdec;
  unsigned dictionary_size;
  uint32_t crc;
  _Bool member_finished;
  _Bool verify_trailer_pending;
  _Bool pos_wrapped;
  unsigned rep0;
  unsigned rep1;
  unsigned rep2;
  unsigned rep3;
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

  struct Len_model match_len_model;
  struct Len_model rep_len_model;
  };

static inline _Bool LZd_enough_free_bytes( const struct LZ_decoder * const d )
  { return Cb_free_bytes( &d->cb ) >= lzd_min_free_bytes; }

static inline uint8_t LZd_peek_prev( const struct LZ_decoder * const d )
  { return d->cb.buffer[((d->cb.put > 0) ? d->cb.put : d->cb.buffer_size)-1]; }

static inline uint8_t LZd_peek( const struct LZ_decoder * const d,
                                const unsigned distance )
  {
  const unsigned i = ( ( d->cb.put > distance ) ? 0 : d->cb.buffer_size ) +
                     d->cb.put - distance - 1;
  return d->cb.buffer[i];
  }

static inline void LZd_put_byte( struct LZ_decoder * const d, const uint8_t b )
  {
  CRC32_update_byte( &d->crc, b );
  d->cb.buffer[d->cb.put] = b;
  if( ++d->cb.put >= d->cb.buffer_size )
    { d->partial_data_pos += d->cb.put; d->cb.put = 0; d->pos_wrapped = 1; }
  }

static inline void LZd_copy_block( struct LZ_decoder * const d,
                                   const unsigned distance, unsigned len )
  {
  unsigned lpos = d->cb.put, i = lpos - distance - 1;
  _Bool fast, fast2;
  if( lpos > distance )
    {
    fast = ( len < d->cb.buffer_size - lpos );
    fast2 = ( fast && len <= lpos - i );
    }
  else
    {
    i += d->cb.buffer_size;
    fast = ( len < d->cb.buffer_size - i );
    fast2 = ( fast && len <= i - lpos );
    }
  if( fast )
    {
    const unsigned tlen = len;
    if( fast2 )
      __builtin___memcpy_chk (d->cb.buffer + lpos, d->cb.buffer + i, len, __builtin_object_size (d->cb.buffer + lpos, 0));
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

static inline _Bool LZd_init( struct LZ_decoder * const d,
                             struct Range_decoder * const rde,
                             const unsigned dict_size )
  {
  if( !Cb_init( &d->cb, ((65536) >= (dict_size) ? (65536) : (dict_size)) + lzd_min_free_bytes ) )
    return 0;
  d->partial_data_pos = 0;
  d->rdec = rde;
  d->dictionary_size = dict_size;
  d->crc = 0xFFFFFFFFU;
  d->member_finished = 0;
  d->verify_trailer_pending = 0;
  d->pos_wrapped = 0;

  d->cb.buffer[d->cb.buffer_size-1] = 0;
  d->rep0 = 0;
  d->rep1 = 0;
  d->rep2 = 0;
  d->rep3 = 0;
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
  return 1;
  }

static inline void LZd_free( struct LZ_decoder * const d )
  { Cb_free( &d->cb ); }

static inline _Bool LZd_member_finished( const struct LZ_decoder * const d )
  { return ( d->member_finished && Cb_empty( &d->cb ) ); }

static inline unsigned LZd_crc( const struct LZ_decoder * const d )
  { return d->crc ^ 0xFFFFFFFFU; }

static inline unsigned long long
LZd_data_position( const struct LZ_decoder * const d )
  { return d->partial_data_pos + d->cb.put; }

static int LZd_try_verify_trailer( struct LZ_decoder * const d )
  {
  Lzip_trailer trailer;
  if( Rd_available_bytes( d->rdec ) < Lt_size )
    { if( !d->rdec->at_stream_end ) return 0; else return 2; }
  d->verify_trailer_pending = 0;
  d->member_finished = 1;

  if( Rd_read_data( d->rdec, trailer, Lt_size ) == Lt_size &&
      Lt_get_data_crc( trailer ) == LZd_crc( d ) &&
      Lt_get_data_size( trailer ) == LZd_data_position( d ) &&
      Lt_get_member_size( trailer ) == d->rdec->member_position ) return 0;
  return 3;
  }





static int LZd_decode_member( struct LZ_decoder * const d )
  {
  struct Range_decoder * const rdec = d->rdec;
  State * const state = &d->state;


  if( d->member_finished ) return 0;
  if( !Rd_try_reload( rdec ) )
    { if( !rdec->at_stream_end ) return 0; else return 2; }
  if( d->verify_trailer_pending ) return LZd_try_verify_trailer( d );

  while( !Rd_finished( rdec ) )
    {
    int len;
    const int pos_state = LZd_data_position( d ) & pos_state_mask;



    if( !Rd_enough_available_bytes( rdec ) )
      { if( !rdec->at_stream_end ) return 0;
        if( Cb_empty( &rdec->cb ) ) break; }
    if( !LZd_enough_free_bytes( d ) ) return 0;
    if( Rd_decode_bit( rdec, &d->bm_match[*state][pos_state] ) == 0 )
      {

      Bit_model * const bm = d->bm_literal[get_lit_state(LZd_peek_prev( d ))];
      if( St_is_char( *state ) )
        {
        *state -= ( *state < 4 ) ? *state : 3;
        LZd_put_byte( d, Rd_decode_tree8( rdec, bm ) );
        }
      else
        {
        *state -= ( *state < 10 ) ? 3 : 6;
        LZd_put_byte( d, Rd_decode_matched( rdec, bm, LZd_peek( d, d->rep0 ) ) );
        }
      continue;
      }

    if( Rd_decode_bit( rdec, &d->bm_rep[*state] ) != 0 )
      {
      if( Rd_decode_bit( rdec, &d->bm_rep0[*state] ) == 0 )
        {
        if( Rd_decode_bit( rdec, &d->bm_len[*state][pos_state] ) == 0 )
          { *state = St_set_short_rep( *state );
            LZd_put_byte( d, LZd_peek( d, d->rep0 ) ); continue; }
        }
      else
        {
        unsigned distance;
        if( Rd_decode_bit( rdec, &d->bm_rep1[*state] ) == 0 )
          distance = d->rep1;
        else
          {
          if( Rd_decode_bit( rdec, &d->bm_rep2[*state] ) == 0 )
            distance = d->rep2;
          else
            { distance = d->rep3; d->rep3 = d->rep2; }
          d->rep2 = d->rep1;
          }
        d->rep1 = d->rep0;
        d->rep0 = distance;
        }
      *state = St_set_rep( *state );
      len = min_match_len + Rd_decode_len( rdec, &d->rep_len_model, pos_state );
      }
    else
      {
      unsigned distance;
      len = min_match_len + Rd_decode_len( rdec, &d->match_len_model, pos_state );
      distance = Rd_decode_tree6( rdec, d->bm_dis_slot[get_len_state(len)] );
      if( distance >= start_dis_model )
        {
        const unsigned dis_slot = distance;
        const int direct_bits = ( dis_slot >> 1 ) - 1;
        distance = ( 2 | ( dis_slot & 1 ) ) << direct_bits;
        if( dis_slot < end_dis_model )
          distance += Rd_decode_tree_reversed( rdec,
                      d->bm_dis + ( distance - dis_slot ), direct_bits );
        else
          {
          distance +=
            Rd_decode( rdec, direct_bits - dis_align_bits ) << dis_align_bits;
          distance += Rd_decode_tree_reversed4( rdec, d->bm_align );
          if( distance == 0xFFFFFFFFU )
            {
            Rd_normalize( rdec );



            if( len == min_match_len )
              {
              d->verify_trailer_pending = 1;
              return LZd_try_verify_trailer( d );
              }
            if( len == min_match_len + 1 )
              {
              rdec->reload_pending = 1;
              if( Rd_try_reload( rdec ) ) continue;
              else { if( !rdec->at_stream_end ) return 0; else break; }
              }
            return 4;
            }
          }
        }
      d->rep3 = d->rep2; d->rep2 = d->rep1; d->rep1 = d->rep0; d->rep0 = distance;
      *state = St_set_match( *state );
      if( d->rep0 >= d->dictionary_size ||
          ( d->rep0 >= d->cb.put && !d->pos_wrapped ) ) return 1;
      }
    LZd_copy_block( d, d->rep0, len );
    }
  return 2;
  }

enum { price_shift_bits = 6,
       price_step_bits = 2 };

static const uint8_t dis_slots[1<<10] =
  {
   0, 1, 2, 3, 4, 4, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7,
   8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9,
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
108, 107, 106, 106, 105, 105, 104, 104, 103, 103, 102, 101, 101, 100, 100, 99,
 99, 98, 98, 97, 97, 96, 96, 95, 95, 94, 94, 93, 93, 92, 92, 91,
 91, 90, 90, 89, 89, 88, 88, 88, 87, 87, 86, 86, 85, 85, 84, 84,
 83, 83, 83, 82, 82, 81, 81, 80, 80, 80, 79, 79, 78, 78, 77, 77,
 77, 76, 76, 75, 75, 75, 74, 74, 73, 73, 73, 72, 72, 71, 71, 71,
 70, 70, 70, 69, 69, 68, 68, 68, 67, 67, 67, 66, 66, 65, 65, 65,
 64, 64, 64, 63, 63, 63, 62, 62, 61, 61, 61, 60, 60, 60, 59, 59,
 59, 58, 58, 58, 57, 57, 57, 56, 56, 56, 55, 55, 55, 54, 54, 54,
 53, 53, 53, 53, 52, 52, 52, 51, 51, 51, 50, 50, 50, 49, 49, 49,
 48, 48, 48, 48, 47, 47, 47, 46, 46, 46, 45, 45, 45, 45, 44, 44,
 44, 43, 43, 43, 43, 42, 42, 42, 41, 41, 41, 41, 40, 40, 40, 40,
 39, 39, 39, 38, 38, 38, 38, 37, 37, 37, 37, 36, 36, 36, 35, 35,
 35, 35, 34, 34, 34, 34, 33, 33, 33, 33, 32, 32, 32, 32, 31, 31,
 31, 31, 30, 30, 30, 30, 29, 29, 29, 29, 28, 28, 28, 28, 27, 27,
 27, 27, 26, 26, 26, 26, 26, 25, 25, 25, 25, 24, 24, 24, 24, 23,
 23, 23, 23, 22, 22, 22, 22, 22, 21, 21, 21, 21, 20, 20, 20, 20,
 20, 19, 19, 19, 19, 18, 18, 18, 18, 18, 17, 17, 17, 17, 17, 16,
 16, 16, 16, 15, 15, 15, 15, 15, 14, 14, 14, 14, 14, 13, 13, 13,
 13, 13, 12, 12, 12, 12, 12, 11, 11, 11, 11, 10, 10, 10, 10, 10,
  9, 9, 9, 9, 9, 9, 8, 8, 8, 8, 8, 7, 7, 7, 7, 7,
  6, 6, 6, 6, 6, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4,
  3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1 };

static inline int get_price( const int probability )
  { return prob_prices[probability >> price_step_bits]; }


static inline int price0( const Bit_model probability )
  { return get_price( probability ); }

static inline int price1( const Bit_model probability )
  { return get_price( bit_model_total - probability ); }

static inline int price_bit( const Bit_model bm, const _Bool bit )
  { return ( bit ? price1( bm ) : price0( bm ) ); }


static inline int price_symbol3( const Bit_model bm[], int symbol )
  {
  int price;
  _Bool bit = symbol & 1;
  symbol |= 8; symbol >>= 1;
  price = price_bit( bm[symbol], bit );
  bit = symbol & 1; symbol >>= 1; price += price_bit( bm[symbol], bit );
  return price + price_bit( bm[1], symbol & 1 );
  }


static inline int price_symbol6( const Bit_model bm[], unsigned symbol )
  {
  int price;
  _Bool bit = symbol & 1;
  symbol |= 64; symbol >>= 1;
  price = price_bit( bm[symbol], bit );
  bit = symbol & 1; symbol >>= 1; price += price_bit( bm[symbol], bit );
  bit = symbol & 1; symbol >>= 1; price += price_bit( bm[symbol], bit );
  bit = symbol & 1; symbol >>= 1; price += price_bit( bm[symbol], bit );
  bit = symbol & 1; symbol >>= 1; price += price_bit( bm[symbol], bit );
  return price + price_bit( bm[1], symbol & 1 );
  }


static inline int price_symbol8( const Bit_model bm[], int symbol )
  {
  int price;
  _Bool bit = symbol & 1;
  symbol |= 0x100; symbol >>= 1;
  price = price_bit( bm[symbol], bit );
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
    const _Bool bit = symbol & 1;
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
  while( 1 )
    {
    const unsigned match_bit = ( match_byte <<= 1 ) & mask;
    const _Bool bit = ( symbol <<= 1 ) & 0x100;
    price += price_bit( bm[(symbol>>9)+match_bit+mask], bit );
    if( symbol >= 0x10000 ) return price;
    mask &= ~(match_bit ^ symbol);
    }
  }


struct Matchfinder_base
  {
  unsigned long long partial_data_pos;
  uint8_t * buffer;
  int32_t * prev_positions;
  int32_t * pos_array;
  int before_size;
  int after_size;
  int buffer_size;
  int dictionary_size;
  int pos;
  int cyclic_pos;
  int stream_pos;
  int pos_limit;
  int key4_mask;
  int num_prev_positions23;
  int num_prev_positions;
  int pos_array_size;
  int saved_dictionary_size;
  _Bool at_stream_end;
  _Bool sync_flush_pending;
  };

static _Bool Mb_normalize_pos( struct Matchfinder_base * const mb );

static _Bool Mb_init( struct Matchfinder_base * const mb, const int before_size,
                     const int dict_size, const int after_size,
                     const int dict_factor, const int num_prev_positions23,
                     const int pos_array_factor );

static inline void Mb_free( struct Matchfinder_base * const mb )
  { free( mb->prev_positions ); free( mb->buffer ); }

static inline uint8_t Mb_peek( const struct Matchfinder_base * const mb,
                               const int distance )
  { return mb->buffer[mb->pos-distance]; }

static inline int Mb_available_bytes( const struct Matchfinder_base * const mb )
  { return mb->stream_pos - mb->pos; }

static inline unsigned long long
Mb_data_position( const struct Matchfinder_base * const mb )
  { return mb->partial_data_pos + mb->pos; }

static inline void Mb_finish( struct Matchfinder_base * const mb )
  { mb->at_stream_end = 1; mb->sync_flush_pending = 0; }

static inline _Bool Mb_data_finished( const struct Matchfinder_base * const mb )
  { return mb->at_stream_end && mb->pos >= mb->stream_pos; }

static inline _Bool Mb_flushing_or_end( const struct Matchfinder_base * const mb )
  { return mb->at_stream_end || mb->sync_flush_pending; }

static inline int Mb_free_bytes( const struct Matchfinder_base * const mb )
  { if( Mb_flushing_or_end( mb ) ) return 0;
    return mb->buffer_size - mb->stream_pos; }

static inline _Bool
Mb_enough_available_bytes( const struct Matchfinder_base * const mb )
  { return ( mb->pos + mb->after_size <= mb->stream_pos ||
             ( Mb_flushing_or_end( mb ) && mb->pos < mb->stream_pos ) ); }

static inline const uint8_t *
Mb_ptr_to_current_pos( const struct Matchfinder_base * const mb )
  { return mb->buffer + mb->pos; }

static int Mb_write_data( struct Matchfinder_base * const mb,
                          const uint8_t * const inbuf, const int size )
  {
  const int sz = ((mb->buffer_size - mb->stream_pos) <= (size) ? (mb->buffer_size - mb->stream_pos) : (size));
  if( Mb_flushing_or_end( mb ) || sz <= 0 ) return 0;
  __builtin___memcpy_chk (mb->buffer + mb->stream_pos, inbuf, sz, __builtin_object_size (mb->buffer + mb->stream_pos, 0));
  mb->stream_pos += sz;
  return sz;
  }

static inline int Mb_true_match_len( const struct Matchfinder_base * const mb,
                                     const int index, const int distance )
  {
  const uint8_t * const data = mb->buffer + mb->pos;
  int i = index;
  const int len_limit = ((Mb_available_bytes( mb )) <= (max_match_len) ? (Mb_available_bytes( mb )) : (max_match_len));
  while( i < len_limit && data[i-distance] == data[i] ) ++i;
  return i;
  }

static inline _Bool Mb_move_pos( struct Matchfinder_base * const mb )
  {
  if( ++mb->cyclic_pos > mb->dictionary_size ) mb->cyclic_pos = 0;
  if( ++mb->pos >= mb->pos_limit ) return Mb_normalize_pos( mb );
  return 1;
  }


struct Range_encoder
  {
  struct Circular_buffer cb;
  unsigned min_free_bytes;
  uint64_t low;
  unsigned long long partial_member_pos;
  uint32_t range;
  unsigned ff_count;
  uint8_t cache;
  Lzip_header header;
  };

static inline void Re_shift_low( struct Range_encoder * const renc )
  {
  if( renc->low >> 24 != 0xFF )
    {
    const _Bool carry = ( renc->low > 0xFFFFFFFFU );
    Cb_put_byte( &renc->cb, renc->cache + carry );
    for( ; renc->ff_count > 0; --renc->ff_count )
      Cb_put_byte( &renc->cb, 0xFF + carry );
    renc->cache = renc->low >> 24;
    }
  else ++renc->ff_count;
  renc->low = ( renc->low & 0x00FFFFFFU ) << 8;
  }

static inline void Re_reset( struct Range_encoder * const renc,
                             const unsigned dictionary_size )
  {
  int i;
  Cb_reset( &renc->cb );
  renc->low = 0;
  renc->partial_member_pos = 0;
  renc->range = 0xFFFFFFFFU;
  renc->ff_count = 0;
  renc->cache = 0;
  Lh_set_dictionary_size( renc->header, dictionary_size );
  for( i = 0; i < Lh_size; ++i )
    Cb_put_byte( &renc->cb, renc->header[i] );
  }

static inline _Bool Re_init( struct Range_encoder * const renc,
                            const unsigned dictionary_size,
                            const unsigned min_free_bytes )
  {
  if( !Cb_init( &renc->cb, 65536 + min_free_bytes ) ) return 0;
  renc->min_free_bytes = min_free_bytes;
  Lh_set_magic( renc->header );
  Re_reset( renc, dictionary_size );
  return 1;
  }

static inline void Re_free( struct Range_encoder * const renc )
  { Cb_free( &renc->cb ); }

static inline unsigned long long
Re_member_position( const struct Range_encoder * const renc )
  { return renc->partial_member_pos + Cb_used_bytes( &renc->cb ) + renc->ff_count; }

static inline _Bool Re_enough_free_bytes( const struct Range_encoder * const renc )
  { return Cb_free_bytes( &renc->cb ) >= renc->min_free_bytes + renc->ff_count; }

static inline int Re_read_data( struct Range_encoder * const renc,
                                uint8_t * const out_buffer, const int out_size )
  {
  const int size = Cb_read_data( &renc->cb, out_buffer, out_size );
  if( size > 0 ) renc->partial_member_pos += size;
  return size;
  }

static inline void Re_flush( struct Range_encoder * const renc )
  {
  int i; for( i = 0; i < 5; ++i ) Re_shift_low( renc );
  renc->low = 0;
  renc->range = 0xFFFFFFFFU;
  renc->ff_count = 0;
  renc->cache = 0;
  }

static inline void Re_encode( struct Range_encoder * const renc,
                              const int symbol, const int num_bits )
  {
  unsigned mask;
  for( mask = 1 << ( num_bits - 1 ); mask > 0; mask >>= 1 )
    {
    renc->range >>= 1;
    if( symbol & mask ) renc->low += renc->range;
    if( renc->range <= 0x00FFFFFFU )
      { renc->range <<= 8; Re_shift_low( renc ); }
    }
  }

static inline void Re_encode_bit( struct Range_encoder * const renc,
                                  Bit_model * const probability, const _Bool bit )
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

static inline void Re_encode_tree3( struct Range_encoder * const renc,
                                    Bit_model bm[], const int symbol )
  {
  int model;
  _Bool bit = ( symbol >> 2 ) & 1;
  Re_encode_bit( renc, &bm[1], bit );
  model = 2 | bit;
  bit = ( symbol >> 1 ) & 1;
  Re_encode_bit( renc, &bm[model], bit ); model <<= 1; model |= bit;
  Re_encode_bit( renc, &bm[model], symbol & 1 );
  }

static inline void Re_encode_tree6( struct Range_encoder * const renc,
                                    Bit_model bm[], const unsigned symbol )
  {
  int model;
  _Bool bit = ( symbol >> 5 ) & 1;
  Re_encode_bit( renc, &bm[1], bit );
  model = 2 | bit;
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

static inline void Re_encode_tree8( struct Range_encoder * const renc,
                                    Bit_model bm[], const int symbol )
  {
  int model = 1;
  int i;
  for( i = 7; i >= 0; --i )
    {
    const _Bool bit = ( symbol >> i ) & 1;
    Re_encode_bit( renc, &bm[model], bit );
    model <<= 1; model |= bit;
    }
  }

static inline void Re_encode_tree_reversed( struct Range_encoder * const renc,
                     Bit_model bm[], int symbol, const int num_bits )
  {
  int model = 1;
  int i;
  for( i = num_bits; i > 0; --i )
    {
    const _Bool bit = symbol & 1;
    symbol >>= 1;
    Re_encode_bit( renc, &bm[model], bit );
    model <<= 1; model |= bit;
    }
  }

static inline void Re_encode_matched( struct Range_encoder * const renc,
                                      Bit_model bm[], unsigned symbol,
                                      unsigned match_byte )
  {
  unsigned mask = 0x100;
  symbol |= mask;
  while( 1 )
    {
    const unsigned match_bit = ( match_byte <<= 1 ) & mask;
    const _Bool bit = ( symbol <<= 1 ) & 0x100;
    Re_encode_bit( renc, &bm[(symbol>>9)+match_bit+mask], bit );
    if( symbol >= 0x10000 ) break;
    mask &= ~(match_bit ^ symbol);
    }
  }

static inline void Re_encode_len( struct Range_encoder * const renc,
                                  struct Len_model * const lm,
                                  int symbol, const int pos_state )
  {
  _Bool bit = ( ( symbol -= min_match_len ) >= len_low_symbols );
  Re_encode_bit( renc, &lm->choice1, bit );
  if( !bit )
    Re_encode_tree3( renc, lm->bm_low[pos_state], symbol );
  else
    {
    bit = ( ( symbol -= len_low_symbols ) >= len_mid_symbols );
    Re_encode_bit( renc, &lm->choice2, bit );
    if( !bit )
      Re_encode_tree3( renc, lm->bm_mid[pos_state], symbol );
    else
      Re_encode_tree8( renc, lm->bm_high, symbol - len_mid_symbols );
    }
  }


enum { max_marker_size = 16,
       num_rep_distances = 4 };

struct LZ_encoder_base
  {
  struct Matchfinder_base mb;
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
  struct Len_model match_len_model;
  struct Len_model rep_len_model;
  struct Range_encoder renc;
  int reps[num_rep_distances];
  State state;
  _Bool member_finished;
  };

static void LZeb_reset( struct LZ_encoder_base * const eb,
                        const unsigned long long member_size );

static inline _Bool LZeb_init( struct LZ_encoder_base * const eb,
                              const int before_size, const int dict_size,
                              const int after_size, const int dict_factor,
                              const int num_prev_positions23,
                              const int pos_array_factor,
                              const unsigned min_free_bytes,
                              const unsigned long long member_size )
  {
  if( !Mb_init( &eb->mb, before_size, dict_size, after_size, dict_factor,
                num_prev_positions23, pos_array_factor ) ) return 0;
  if( !Re_init( &eb->renc, eb->mb.dictionary_size, min_free_bytes ) )
    return 0;
  LZeb_reset( eb, member_size );
  return 1;
  }

static inline _Bool LZeb_member_finished( const struct LZ_encoder_base * const eb )
  { return ( eb->member_finished && Cb_empty( &eb->renc.cb ) ); }

static inline void LZeb_free( struct LZ_encoder_base * const eb )
  { Re_free( &eb->renc ); Mb_free( &eb->mb ); }

static inline unsigned LZeb_crc( const struct LZ_encoder_base * const eb )
  { return eb->crc ^ 0xFFFFFFFFU; }

static inline int LZeb_price_literal( const struct LZ_encoder_base * const eb,
                            const uint8_t prev_byte, const uint8_t symbol )
  { return price_symbol8( eb->bm_literal[get_lit_state(prev_byte)], symbol ); }

static inline int LZeb_price_matched( const struct LZ_encoder_base * const eb,
  const uint8_t prev_byte, const uint8_t symbol, const uint8_t match_byte )
  { return price_matched( eb->bm_literal[get_lit_state(prev_byte)], symbol,
                          match_byte ); }

static inline void LZeb_encode_literal( struct LZ_encoder_base * const eb,
                            const uint8_t prev_byte, const uint8_t symbol )
  { Re_encode_tree8( &eb->renc, eb->bm_literal[get_lit_state(prev_byte)],
                     symbol ); }

static inline void LZeb_encode_matched( struct LZ_encoder_base * const eb,
  const uint8_t prev_byte, const uint8_t symbol, const uint8_t match_byte )
  { Re_encode_matched( &eb->renc, eb->bm_literal[get_lit_state(prev_byte)],
                       symbol, match_byte ); }

static inline void LZeb_encode_pair( struct LZ_encoder_base * const eb,
                                     const unsigned dis, const int len,
                                     const int pos_state )
  {
  const unsigned dis_slot = get_slot( dis );
  Re_encode_len( &eb->renc, &eb->match_len_model, len, pos_state );
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

static _Bool Mb_normalize_pos( struct Matchfinder_base * const mb )
  {
  if( mb->pos > mb->stream_pos )
    { mb->pos = mb->stream_pos; return 0; }
  if( !mb->at_stream_end )
    {
    int i;

    const int32_t offset = mb->pos - mb->before_size - mb->dictionary_size;
    const int size = mb->stream_pos - offset;
    __builtin___memmove_chk (mb->buffer, mb->buffer + offset, size, __builtin_object_size (mb->buffer, 0));
    mb->partial_data_pos += offset;
    mb->pos -= offset;
    mb->stream_pos -= offset;
    for( i = 0; i < mb->num_prev_positions; ++i )
      mb->prev_positions[i] -= ((mb->prev_positions[i]) <= (offset) ? (mb->prev_positions[i]) : (offset));
    for( i = 0; i < mb->pos_array_size; ++i )
      mb->pos_array[i] -= ((mb->pos_array[i]) <= (offset) ? (mb->pos_array[i]) : (offset));
    }
  return 1;
  }


static _Bool Mb_init( struct Matchfinder_base * const mb, const int before_size,
                     const int dict_size, const int after_size,
                     const int dict_factor, const int num_prev_positions23,
                     const int pos_array_factor )
  {
  const int buffer_size_limit =
    ( dict_factor * dict_size ) + before_size + after_size;
  unsigned size;
  int i;

  mb->partial_data_pos = 0;
  mb->before_size = before_size;
  mb->after_size = after_size;
  mb->pos = 0;
  mb->cyclic_pos = 0;
  mb->stream_pos = 0;
  mb->num_prev_positions23 = num_prev_positions23;
  mb->at_stream_end = 0;
  mb->sync_flush_pending = 0;

  mb->buffer_size = ((65536) >= (buffer_size_limit) ? (65536) : (buffer_size_limit));
  mb->buffer = (uint8_t *)malloc( mb->buffer_size );
  if( !mb->buffer ) return 0;
  mb->saved_dictionary_size = dict_size;
  mb->dictionary_size = dict_size;
  mb->pos_limit = mb->buffer_size - after_size;
  size = 1 << ((16) >= (real_bits( mb->dictionary_size - 1 ) - 2) ? (16) : (real_bits( mb->dictionary_size - 1 ) - 2));
  if( mb->dictionary_size > 1 << 26 )
    size >>= 1;
  mb->key4_mask = size - 1;
  size += num_prev_positions23;
  mb->num_prev_positions = size;

  mb->pos_array_size = pos_array_factor * ( mb->dictionary_size + 1 );
  size += mb->pos_array_size;
  if( size * sizeof mb->prev_positions[0] <= size ) mb->prev_positions = 0;
  else mb->prev_positions =
    (int32_t *)malloc( size * sizeof mb->prev_positions[0] );
  if( !mb->prev_positions ) { free( mb->buffer ); return 0; }
  mb->pos_array = mb->prev_positions + mb->num_prev_positions;
  for( i = 0; i < mb->num_prev_positions; ++i ) mb->prev_positions[i] = 0;
  return 1;
  }


static void Mb_adjust_array( struct Matchfinder_base * const mb )
  {
  int size = 1 << ((16) >= (real_bits( mb->dictionary_size - 1 ) - 2) ? (16) : (real_bits( mb->dictionary_size - 1 ) - 2));
  if( mb->dictionary_size > 1 << 26 )
    size >>= 1;
  mb->key4_mask = size - 1;
  size += mb->num_prev_positions23;
  mb->num_prev_positions = size;
  mb->pos_array = mb->prev_positions + mb->num_prev_positions;
  }


static void Mb_adjust_dictionary_size( struct Matchfinder_base * const mb )
  {
  if( mb->stream_pos < mb->dictionary_size )
    {
    mb->dictionary_size = ((min_dictionary_size) >= (mb->stream_pos) ? (min_dictionary_size) : (mb->stream_pos));
    Mb_adjust_array( mb );
    mb->pos_limit = mb->buffer_size;
    }
  }


static void Mb_reset( struct Matchfinder_base * const mb )
  {
  int i;
  if( mb->stream_pos > mb->pos )
    __builtin___memmove_chk (mb->buffer, mb->buffer + mb->pos, mb->stream_pos - mb->pos, __builtin_object_size (mb->buffer, 0));
  mb->partial_data_pos = 0;
  mb->stream_pos -= mb->pos;
  mb->pos = 0;
  mb->cyclic_pos = 0;
  mb->at_stream_end = 0;
  mb->sync_flush_pending = 0;
  mb->dictionary_size = mb->saved_dictionary_size;
  Mb_adjust_array( mb );
  mb->pos_limit = mb->buffer_size - mb->after_size;
  for( i = 0; i < mb->num_prev_positions; ++i ) mb->prev_positions[i] = 0;
  }



static void LZeb_try_full_flush( struct LZ_encoder_base * const eb )
  {
  int i;
  const int pos_state = Mb_data_position( &eb->mb ) & pos_state_mask;
  const State state = eb->state;
  Lzip_trailer trailer;
  if( eb->member_finished ||
      Cb_free_bytes( &eb->renc.cb ) < max_marker_size + eb->renc.ff_count + Lt_size )
    return;
  eb->member_finished = 1;
  Re_encode_bit( &eb->renc, &eb->bm_match[state][pos_state], 1 );
  Re_encode_bit( &eb->renc, &eb->bm_rep[state], 0 );
  LZeb_encode_pair( eb, 0xFFFFFFFFU, min_match_len, pos_state );
  Re_flush( &eb->renc );
  Lt_set_data_crc( trailer, LZeb_crc( eb ) );
  Lt_set_data_size( trailer, Mb_data_position( &eb->mb ) );
  Lt_set_member_size( trailer, Re_member_position( &eb->renc ) + Lt_size );
  for( i = 0; i < Lt_size; ++i )
    Cb_put_byte( &eb->renc.cb, trailer[i] );
  }



static void LZeb_try_sync_flush( struct LZ_encoder_base * const eb )
  {
  const int pos_state = Mb_data_position( &eb->mb ) & pos_state_mask;
  const State state = eb->state;
  const unsigned min_size = eb->renc.ff_count + max_marker_size;
  if( eb->member_finished ||
      Cb_free_bytes( &eb->renc.cb ) < min_size + max_marker_size ) return;
  eb->mb.sync_flush_pending = 0;
  const unsigned long long old_mpos = Re_member_position( &eb->renc );
  do {
    Re_encode_bit( &eb->renc, &eb->bm_match[state][pos_state], 1 );
    Re_encode_bit( &eb->renc, &eb->bm_rep[state], 0 );
    LZeb_encode_pair( eb, 0xFFFFFFFFU, min_match_len + 1, pos_state );
    Re_flush( &eb->renc );
    }
  while( Re_member_position( &eb->renc ) - old_mpos < min_size );
  }


static void LZeb_reset( struct LZ_encoder_base * const eb,
                        const unsigned long long member_size )
  {
  const unsigned long long min_member_size = min_dictionary_size;
  const unsigned long long max_member_size = 0x0008000000000000ULL;
  int i;
  Mb_reset( &eb->mb );
  eb->member_size_limit =
    ((((min_member_size) >= (member_size) ? (min_member_size) : (member_size))) <= (max_member_size) ? (((min_member_size) >= (member_size) ? (min_member_size) : (member_size))) : (max_member_size)) -
    Lt_size - max_marker_size;
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
  for( i = 0; i < num_rep_distances; ++i ) eb->reps[i] = 0;
  eb->state = 0;
  eb->member_finished = 0;
  }

struct Len_prices
  {
  const struct Len_model * lm;
  int len_symbols;
  int count;
  int prices[pos_states][max_len_symbols];
  int counters[pos_states];
  };

static inline void Lp_update_low_mid_prices( struct Len_prices * const lp,
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

static inline void Lp_update_high_prices( struct Len_prices * const lp )
  {
  const int tmp = price1( lp->lm->choice1 ) + price1( lp->lm->choice2 );
  int len;
  for( len = len_low_symbols + len_mid_symbols; len < lp->len_symbols; ++len )

    lp->prices[3][len] = lp->prices[2][len] =
    lp->prices[1][len] = lp->prices[0][len] = tmp +
      price_symbol8( lp->lm->bm_high, len - len_low_symbols - len_mid_symbols );
  }

static inline void Lp_reset( struct Len_prices * const lp )
  { int i; for( i = 0; i < pos_states; ++i ) lp->counters[i] = 0; }

static inline void Lp_init( struct Len_prices * const lp,
                            const struct Len_model * const lm,
                            const int match_len_limit )
  {
  lp->lm = lm;
  lp->len_symbols = match_len_limit + 1 - min_match_len;
  lp->count = ( match_len_limit > 12 ) ? 1 : lp->len_symbols;
  Lp_reset( lp );
  }

static inline void Lp_decrement_counter( struct Len_prices * const lp,
                                         const int pos_state )
  { --lp->counters[pos_state]; }

static inline void Lp_update_prices( struct Len_prices * const lp )
  {
  int pos_state;
  _Bool high_pending = 0;
  for( pos_state = 0; pos_state < pos_states; ++pos_state )
    if( lp->counters[pos_state] <= 0 )
      { lp->counters[pos_state] = lp->count;
        Lp_update_low_mid_prices( lp, pos_state ); high_pending = 1; }
  if( high_pending && lp->len_symbols > len_low_symbols + len_mid_symbols )
    Lp_update_high_prices( lp );
  }

static inline int Lp_price( const struct Len_prices * const lp,
                            const int len, const int pos_state )
  { return lp->prices[pos_state][len - min_match_len]; }


struct Pair
  {
  int dis;
  int len;
  };

enum { infinite_price = 0x0FFFFFFF,
       max_num_trials = 1 << 13,
       single_step_trial = -2,
       dual_step_trial = -1 };

struct Trial
  {
  State state;
  int price;
  int dis4;
  int prev_index;
  int prev_index2;


  int reps[num_rep_distances];
  };

static inline void Tr_update( struct Trial * const trial, const int pr,
                              const int distance4, const int p_i )
  {
  if( pr < trial->price )
    { trial->price = pr; trial->dis4 = distance4; trial->prev_index = p_i;
      trial->prev_index2 = single_step_trial; }
  }

static inline void Tr_update2( struct Trial * const trial, const int pr,
                               const int p_i )
  {
  if( pr < trial->price )
    { trial->price = pr; trial->dis4 = 0; trial->prev_index = p_i;
      trial->prev_index2 = dual_step_trial; }
  }

static inline void Tr_update3( struct Trial * const trial, const int pr,
                               const int distance4, const int p_i,
                               const int p_i2 )
  {
  if( pr < trial->price )
    { trial->price = pr; trial->dis4 = distance4; trial->prev_index = p_i;
      trial->prev_index2 = p_i2; }
  }


struct LZ_encoder
  {
  struct LZ_encoder_base eb;
  int cycles;
  int match_len_limit;
  struct Len_prices match_len_prices;
  struct Len_prices rep_len_prices;
  int pending_num_pairs;
  struct Pair pairs[max_match_len+1];
  struct Trial trials[max_num_trials];

  int dis_slot_prices[len_states][2*max_dictionary_bits];
  int dis_prices[len_states][modeled_distances];
  int align_prices[dis_align_size];
  int num_dis_slots;
  int price_counter;
  int dis_price_counter;
  int align_price_counter;
  _Bool been_flushed;
  };

static inline _Bool Mb_dec_pos( struct Matchfinder_base * const mb,
                               const int ahead )
  {
  if( ahead < 0 || mb->pos < ahead ) return 0;
  mb->pos -= ahead;
  if( mb->cyclic_pos < ahead ) mb->cyclic_pos += mb->dictionary_size + 1;
  mb->cyclic_pos -= ahead;
  return 1;
  }

static int LZe_get_match_pairs( struct LZ_encoder * const e, struct Pair * pairs );


static inline void mtf_reps( const int dis4, int reps[num_rep_distances] )
  {
  if( dis4 >= num_rep_distances )
    {
    reps[3] = reps[2]; reps[2] = reps[1]; reps[1] = reps[0];
    reps[0] = dis4 - num_rep_distances;
    }
  else if( dis4 > 0 )
    {
    const int distance = reps[dis4];
    int i; for( i = dis4; i > 0; --i ) reps[i] = reps[i-1];
    reps[0] = distance;
    }
  }

static inline int LZeb_price_shortrep( const struct LZ_encoder_base * const eb,
                                       const State state, const int pos_state )
  {
  return price0( eb->bm_rep0[state] ) + price0( eb->bm_len[state][pos_state] );
  }

static inline int LZeb_price_rep( const struct LZ_encoder_base * const eb,
                                  const int rep, const State state,
                                  const int pos_state )
  {
  int price;
  if( rep == 0 ) return price0( eb->bm_rep0[state] ) +
                        price1( eb->bm_len[state][pos_state] );
  price = price1( eb->bm_rep0[state] );
  if( rep == 1 )
    price += price0( eb->bm_rep1[state] );
  else
    {
    price += price1( eb->bm_rep1[state] );
    price += price_bit( eb->bm_rep2[state], rep - 2 );
    }
  return price;
  }

static inline int LZe_price_rep0_len( const struct LZ_encoder * const e,
                                      const int len, const State state,
                                      const int pos_state )
  {
  return LZeb_price_rep( &e->eb, 0, state, pos_state ) +
         Lp_price( &e->rep_len_prices, len, pos_state );
  }

static inline int LZe_price_pair( const struct LZ_encoder * const e,
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

static inline int LZe_read_match_distances( struct LZ_encoder * const e )
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

static inline _Bool LZe_move_and_update( struct LZ_encoder * const e, int n )
  {
  while( 1 )
    {
    if( !Mb_move_pos( &e->eb.mb ) ) return 0;
    if( --n <= 0 ) break;
    LZe_get_match_pairs( e, 0 );
    }
  return 1;
  }

static inline void LZe_backward( struct LZ_encoder * const e, int cur )
  {
  int dis4 = e->trials[cur].dis4;
  while( cur > 0 )
    {
    const int prev_index = e->trials[cur].prev_index;
    struct Trial * const prev_trial = &e->trials[prev_index];

    if( e->trials[cur].prev_index2 != single_step_trial )
      {
      prev_trial->dis4 = -1;
      prev_trial->prev_index = prev_index - 1;
      prev_trial->prev_index2 = single_step_trial;
      if( e->trials[cur].prev_index2 >= 0 )
        {
        struct Trial * const prev_trial2 = &e->trials[prev_index-1];
        prev_trial2->dis4 = dis4; dis4 = 0;
        prev_trial2->prev_index = e->trials[cur].prev_index2;
        prev_trial2->prev_index2 = single_step_trial;
        }
      }
    prev_trial->price = cur - prev_index;
    cur = dis4; dis4 = prev_trial->dis4; prev_trial->dis4 = cur;
    cur = prev_index;
    }
  }

enum { num_prev_positions3 = 1 << 16,
       num_prev_positions2 = 1 << 10 };

static inline _Bool LZe_init( struct LZ_encoder * const e,
                             const int dict_size, const int len_limit,
                             const unsigned long long member_size )
  {
  enum { before_size = max_num_trials,

         after_size = max_num_trials + ( 2 * max_match_len ) + 1,
         dict_factor = 2,
         num_prev_positions23 = num_prev_positions2 + num_prev_positions3,
         pos_array_factor = 2,
         min_free_bytes = 2 * max_num_trials };

  if( !LZeb_init( &e->eb, before_size, dict_size, after_size, dict_factor,
                  num_prev_positions23, pos_array_factor, min_free_bytes,
                  member_size ) ) return 0;
  e->cycles = ( len_limit < max_match_len ) ? 16 + ( len_limit / 2 ) : 256;
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
  e->been_flushed = 0;
  return 1;
  }

static inline void LZe_reset( struct LZ_encoder * const e,
                              const unsigned long long member_size )
  {
  LZeb_reset( &e->eb, member_size );
  Lp_reset( &e->match_len_prices );
  Lp_reset( &e->rep_len_prices );
  e->pending_num_pairs = 0;
  e->price_counter = 0;
  e->dis_price_counter = 0;
  e->align_price_counter = 0;
  e->been_flushed = 0;
  }

static int LZe_get_match_pairs( struct LZ_encoder * const e, struct Pair * pairs )
  {
  int32_t * ptr0 = e->eb.mb.pos_array + ( e->eb.mb.cyclic_pos << 1 );
  int32_t * ptr1 = ptr0 + 1;
  int32_t * newptr;
  int len = 0, len0 = 0, len1 = 0;
  int maxlen = 3;
  int num_pairs = 0;
  const int pos1 = e->eb.mb.pos + 1;
  const int min_pos = ( e->eb.mb.pos > e->eb.mb.dictionary_size ) ?
                        e->eb.mb.pos - e->eb.mb.dictionary_size : 0;
  const uint8_t * const data = Mb_ptr_to_current_pos( &e->eb.mb );
  int count, key2, key3, key4, newpos1;
  unsigned tmp;
  int len_limit = e->match_len_limit;

  if( len_limit > Mb_available_bytes( &e->eb.mb ) )
    {
    e->been_flushed = 1;
    len_limit = Mb_available_bytes( &e->eb.mb );
    if( len_limit < 4 ) { *ptr0 = *ptr1 = 0; return 0; }
    }

  tmp = crc32[data[0]] ^ data[1];
  key2 = tmp & ( num_prev_positions2 - 1 );
  tmp ^= (unsigned)data[2] << 8;
  key3 = num_prev_positions2 + ( tmp & ( num_prev_positions3 - 1 ) );
  key4 = num_prev_positions2 + num_prev_positions3 +
         ( ( tmp ^ ( crc32[data[3]] << 5 ) ) & e->eb.mb.key4_mask );

  if( pairs )
    {
    const int np2 = e->eb.mb.prev_positions[key2];
    const int np3 = e->eb.mb.prev_positions[key3];
    if( np2 > min_pos && e->eb.mb.buffer[np2-1] == data[0] )
      {
      pairs[0].dis = e->eb.mb.pos - np2;
      pairs[0].len = maxlen = 2;
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
      if( maxlen >= len_limit ) pairs = 0;
      }
    }

  e->eb.mb.prev_positions[key2] = pos1;
  e->eb.mb.prev_positions[key3] = pos1;
  newpos1 = e->eb.mb.prev_positions[key4];
  e->eb.mb.prev_positions[key4] = pos1;

  for( count = e->cycles; ; )
    {
    int delta;
    if( newpos1 <= min_pos || --count < 0 ) { *ptr0 = *ptr1 = 0; break; }

    if( e->been_flushed ) len = 0;
    delta = pos1 - newpos1;
    newptr = e->eb.mb.pos_array +
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


static void LZe_update_distance_prices( struct LZ_encoder * const e )
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
    int * const dp = e->dis_prices[len_state];
    const Bit_model * const bmds = e->eb.bm_dis_slot[len_state];
    int slot = 0;
    for( ; slot < end_dis_model; ++slot )
      dsp[slot] = price_symbol6( bmds, slot );
    for( ; slot < e->num_dis_slots; ++slot )
      dsp[slot] = price_symbol6( bmds, slot ) +
                  (((( slot >> 1 ) - 1 ) - dis_align_bits ) << price_shift_bits );

    for( dis = 0; dis < start_dis_model; ++dis )
      dp[dis] = dsp[dis];
    for( ; dis < modeled_distances; ++dis )
      dp[dis] += dsp[dis_slots[dis]];
    }
  }







static int LZe_sequence_optimizer( struct LZ_encoder * const e,
                                   const int reps[num_rep_distances],
                                   const State state )
  {
  int main_len, num_pairs, i, rep, num_trials, len;
  int rep_index = 0, cur = 0;
  int replens[num_rep_distances];

  if( e->pending_num_pairs > 0 )
    {
    num_pairs = e->pending_num_pairs;
    e->pending_num_pairs = 0;
    }
  else
    num_pairs = LZe_read_match_distances( e );
  main_len = ( num_pairs > 0 ) ? e->pairs[num_pairs-1].len : 0;

  for( i = 0; i < num_rep_distances; ++i )
    {
    replens[i] = Mb_true_match_len( &e->eb.mb, 0, reps[i] + 1 );
    if( replens[i] > replens[rep_index] ) rep_index = i;
    }
  if( replens[rep_index] >= e->match_len_limit )
    {
    e->trials[0].price = replens[rep_index];
    e->trials[0].dis4 = rep_index;
    if( !LZe_move_and_update( e, replens[rep_index] ) ) return 0;
    return replens[rep_index];
    }

  if( main_len >= e->match_len_limit )
    {
    e->trials[0].price = main_len;
    e->trials[0].dis4 = e->pairs[num_pairs-1].dis + num_rep_distances;
    if( !LZe_move_and_update( e, main_len ) ) return 0;
    return main_len;
    }

  {
  const int pos_state = Mb_data_position( &e->eb.mb ) & pos_state_mask;
  const int match_price = price1( e->eb.bm_match[state][pos_state] );
  const int rep_match_price = match_price + price1( e->eb.bm_rep[state] );
  const uint8_t prev_byte = Mb_peek( &e->eb.mb, 1 );
  const uint8_t cur_byte = Mb_peek( &e->eb.mb, 0 );
  const uint8_t match_byte = Mb_peek( &e->eb.mb, reps[0] + 1 );

  e->trials[1].price = price0( e->eb.bm_match[state][pos_state] );
  if( St_is_char( state ) )
    e->trials[1].price += LZeb_price_literal( &e->eb, prev_byte, cur_byte );
  else
    e->trials[1].price += LZeb_price_matched( &e->eb, prev_byte, cur_byte, match_byte );
  e->trials[1].dis4 = -1;

  if( match_byte == cur_byte )
    Tr_update( &e->trials[1], rep_match_price +
               LZeb_price_shortrep( &e->eb, state, pos_state ), 0, 0 );

  num_trials = ((main_len) >= (replens[rep_index]) ? (main_len) : (replens[rep_index]));

  if( num_trials < min_match_len )
    {
    e->trials[0].price = 1;
    e->trials[0].dis4 = e->trials[1].dis4;
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
    int price;
    if( replens[rep] < min_match_len ) continue;
    price = rep_match_price + LZeb_price_rep( &e->eb, rep, state, pos_state );
    for( len = min_match_len; len <= replens[rep]; ++len )
      Tr_update( &e->trials[len], price +
                 Lp_price( &e->rep_len_prices, len, pos_state ), rep, 0 );
    }

  if( main_len > replens[0] )
    {
    const int normal_match_price = match_price + price0( e->eb.bm_rep[state] );
    int i = 0, len = ((replens[0] + 1) >= (min_match_len) ? (replens[0] + 1) : (min_match_len));
    while( len > e->pairs[i].len ) ++i;
    while( 1 )
      {
      const int dis = e->pairs[i].dis;
      Tr_update( &e->trials[len], normal_match_price +
                 LZe_price_pair( e, dis, len, pos_state ),
                 dis + num_rep_distances, 0 );
      if( ++len > e->pairs[i].len && ++i >= num_pairs ) break;
      }
    }
  }

  while( 1 )
    {
    struct Trial *cur_trial, *next_trial;
    int newlen, pos_state, triable_bytes, len_limit;
    int start_len = min_match_len;
    int next_price, match_price, rep_match_price;
    State cur_state;
    uint8_t prev_byte, cur_byte, match_byte;

    if( !Mb_move_pos( &e->eb.mb ) ) return 0;
    if( ++cur >= num_trials )
      {
      LZe_backward( e, cur );
      return cur;
      }

    num_pairs = LZe_read_match_distances( e );
    newlen = ( num_pairs > 0 ) ? e->pairs[num_pairs-1].len : 0;
    if( newlen >= e->match_len_limit )
      {
      e->pending_num_pairs = num_pairs;
      LZe_backward( e, cur );
      return cur;
      }


    cur_trial = &e->trials[cur];
    {
    const int dis4 = cur_trial->dis4;
    int prev_index = cur_trial->prev_index;
    const int prev_index2 = cur_trial->prev_index2;

    if( prev_index2 == single_step_trial )
      {
      cur_state = e->trials[prev_index].state;
      if( prev_index + 1 == cur )
        {
        if( dis4 == 0 ) cur_state = St_set_short_rep( cur_state );
        else cur_state = St_set_char( cur_state );
        }
      else if( dis4 < num_rep_distances ) cur_state = St_set_rep( cur_state );
      else cur_state = St_set_match( cur_state );
      }
    else
      {
      if( prev_index2 == dual_step_trial )
        --prev_index;
      else
        prev_index = prev_index2;
      cur_state = St_set_char_rep();
      }
    cur_trial->state = cur_state;
    for( i = 0; i < num_rep_distances; ++i )
      cur_trial->reps[i] = e->trials[prev_index].reps[i];
    mtf_reps( dis4, cur_trial->reps );
    }

    pos_state = Mb_data_position( &e->eb.mb ) & pos_state_mask;
    prev_byte = Mb_peek( &e->eb.mb, 1 );
    cur_byte = Mb_peek( &e->eb.mb, 0 );
    match_byte = Mb_peek( &e->eb.mb, cur_trial->reps[0] + 1 );

    next_price = cur_trial->price +
                 price0( e->eb.bm_match[cur_state][pos_state] );
    if( St_is_char( cur_state ) )
      next_price += LZeb_price_literal( &e->eb, prev_byte, cur_byte );
    else
      next_price += LZeb_price_matched( &e->eb, prev_byte, cur_byte, match_byte );


    next_trial = &e->trials[cur+1];

    Tr_update( next_trial, next_price, -1, cur );

    match_price = cur_trial->price + price1( e->eb.bm_match[cur_state][pos_state] );
    rep_match_price = match_price + price1( e->eb.bm_rep[cur_state] );

    if( match_byte == cur_byte && next_trial->dis4 != 0 &&
        next_trial->prev_index2 == single_step_trial )
      {
      const int price = rep_match_price +
                        LZeb_price_shortrep( &e->eb, cur_state, pos_state );
      if( price <= next_trial->price )
        {
        next_trial->price = price;
        next_trial->dis4 = 0;
        next_trial->prev_index = cur;
        }
      }

    triable_bytes =
      ((Mb_available_bytes( &e->eb.mb )) <= (max_num_trials - 1 - cur) ? (Mb_available_bytes( &e->eb.mb )) : (max_num_trials - 1 - cur));
    if( triable_bytes < min_match_len ) continue;

    len_limit = ((e->match_len_limit) <= (triable_bytes) ? (e->match_len_limit) : (triable_bytes));


    if( match_byte != cur_byte && next_trial->prev_index != cur )
      {
      const uint8_t * const data = Mb_ptr_to_current_pos( &e->eb.mb );
      const int dis = cur_trial->reps[0] + 1;
      const int limit = ((e->match_len_limit + 1) <= (triable_bytes) ? (e->match_len_limit + 1) : (triable_bytes));
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


    for( rep = 0; rep < num_rep_distances; ++rep )
      {
      const uint8_t * const data = Mb_ptr_to_current_pos( &e->eb.mb );
      const int dis = cur_trial->reps[rep] + 1;
      int price;

      if( data[0-dis] != data[0] || data[1-dis] != data[1] ) continue;
      for( len = min_match_len; len < len_limit; ++len )
        if( data[len-dis] != data[len] ) break;
      while( num_trials < cur + len )
        e->trials[++num_trials].price = infinite_price;
      price = rep_match_price + LZeb_price_rep( &e->eb, rep, cur_state, pos_state );
      for( i = min_match_len; i <= len; ++i )
        Tr_update( &e->trials[cur+i], price +
                   Lp_price( &e->rep_len_prices, i, pos_state ), rep, cur );

      if( rep == 0 ) start_len = len + 1;


      {
      int len2 = len + 1;
      const int limit = ((e->match_len_limit + len2) <= (triable_bytes) ? (e->match_len_limit + len2) : (triable_bytes));
      int pos_state2;
      State state2;
      while( len2 < limit && data[len2-dis] == data[len2] ) ++len2;
      len2 -= len + 1;
      if( len2 < min_match_len ) continue;

      pos_state2 = ( pos_state + len ) & pos_state_mask;
      state2 = St_set_rep( cur_state );
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
      }


    if( newlen >= start_len && newlen <= len_limit )
      {
      int dis;
      const int normal_match_price = match_price +
                                     price0( e->eb.bm_rep[cur_state] );

      while( num_trials < cur + newlen )
        e->trials[++num_trials].price = infinite_price;

      i = 0;
      while( e->pairs[i].len < start_len ) ++i;
      dis = e->pairs[i].dis;
      for( len = start_len; ; ++len )
        {
        int price = normal_match_price + LZe_price_pair( e, dis, len, pos_state );
        Tr_update( &e->trials[cur+len], price, dis + num_rep_distances, cur );


        if( len == e->pairs[i].len )
          {
          const uint8_t * const data = Mb_ptr_to_current_pos( &e->eb.mb );
          const int dis2 = dis + 1;
          int len2 = len + 1;
          const int limit = ((e->match_len_limit + len2) <= (triable_bytes) ? (e->match_len_limit + len2) : (triable_bytes));
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


static _Bool LZe_encode_member( struct LZ_encoder * const e )
  {
  const _Bool best = ( e->match_len_limit > 12 );
  const int dis_price_count = best ? 1 : 512;
  const int align_price_count = best ? 1 : dis_align_size;
  const int price_count = ( e->match_len_limit > 36 ) ? 1013 : 4093;
  int ahead, i;
  State * const state = &e->eb.state;

  if( e->eb.member_finished ) return 1;
  if( Re_member_position( &e->eb.renc ) >= e->eb.member_size_limit )
    { LZeb_try_full_flush( &e->eb ); return 1; }

  if( Mb_data_position( &e->eb.mb ) == 0 &&
      !Mb_data_finished( &e->eb.mb ) )
    {
    const uint8_t prev_byte = 0;
    uint8_t cur_byte;
    if( !Mb_enough_available_bytes( &e->eb.mb ) ||
        !Re_enough_free_bytes( &e->eb.renc ) ) return 1;
    cur_byte = Mb_peek( &e->eb.mb, 0 );
    Re_encode_bit( &e->eb.renc, &e->eb.bm_match[*state][0], 0 );
    LZeb_encode_literal( &e->eb, prev_byte, cur_byte );
    CRC32_update_byte( &e->eb.crc, cur_byte );
    LZe_get_match_pairs( e, 0 );
    if( !Mb_move_pos( &e->eb.mb ) ) return 0;
    }

  while( !Mb_data_finished( &e->eb.mb ) )
    {
    if( !Mb_enough_available_bytes( &e->eb.mb ) ||
        !Re_enough_free_bytes( &e->eb.renc ) ) return 1;
    if( e->price_counter <= 0 && e->pending_num_pairs == 0 )
      {
      e->price_counter = price_count;
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

    ahead = LZe_sequence_optimizer( e, e->eb.reps, *state );
    e->price_counter -= ahead;

    for( i = 0; ahead > 0; )
      {
      const int pos_state =
        ( Mb_data_position( &e->eb.mb ) - ahead ) & pos_state_mask;
      const int len = e->trials[i].price;
      int dis = e->trials[i].dis4;

      _Bool bit = ( dis < 0 );
      Re_encode_bit( &e->eb.renc, &e->eb.bm_match[*state][pos_state], !bit );
      if( bit )
        {
        const uint8_t prev_byte = Mb_peek( &e->eb.mb, ahead + 1 );
        const uint8_t cur_byte = Mb_peek( &e->eb.mb, ahead );
        CRC32_update_byte( &e->eb.crc, cur_byte );
        if( St_is_char( *state ) )
          LZeb_encode_literal( &e->eb, prev_byte, cur_byte );
        else
          {
          const uint8_t match_byte = Mb_peek( &e->eb.mb, ahead + e->eb.reps[0] + 1 );
          LZeb_encode_matched( &e->eb, prev_byte, cur_byte, match_byte );
          }
        *state = St_set_char( *state );
        }
      else
        {
        CRC32_update_buf( &e->eb.crc, Mb_ptr_to_current_pos( &e->eb.mb ) - ahead, len );
        mtf_reps( dis, e->eb.reps );
        bit = ( dis < num_rep_distances );
        Re_encode_bit( &e->eb.renc, &e->eb.bm_rep[*state], bit );
        if( bit )
          {
          bit = ( dis == 0 );
          Re_encode_bit( &e->eb.renc, &e->eb.bm_rep0[*state], !bit );
          if( bit )
            Re_encode_bit( &e->eb.renc, &e->eb.bm_len[*state][pos_state], len > 1 );
          else
            {
            Re_encode_bit( &e->eb.renc, &e->eb.bm_rep1[*state], dis > 1 );
            if( dis > 1 )
              Re_encode_bit( &e->eb.renc, &e->eb.bm_rep2[*state], dis > 2 );
            }
          if( len == 1 ) *state = St_set_short_rep( *state );
          else
            {
            Re_encode_len( &e->eb.renc, &e->eb.rep_len_model, len, pos_state );
            Lp_decrement_counter( &e->rep_len_prices, pos_state );
            *state = St_set_rep( *state );
            }
          }
        else
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
        if( !Mb_dec_pos( &e->eb.mb, ahead ) ) return 0;
        LZeb_try_full_flush( &e->eb );
        return 1;
        }
      }
    }
  LZeb_try_full_flush( &e->eb );
  return 1;
  }

struct FLZ_encoder
  {
  struct LZ_encoder_base eb;
  unsigned key4;
  };

static inline void FLZe_reset_key4( struct FLZ_encoder * const fe )
  {
  int i;
  fe->key4 = 0;
  for( i = 0; i < 3 && i < Mb_available_bytes( &fe->eb.mb ); ++i )
    fe->key4 = ( fe->key4 << 4 ) ^ fe->eb.mb.buffer[i];
  }

static inline _Bool FLZe_update_and_move( struct FLZ_encoder * const fe, int n )
  {
  struct Matchfinder_base * const mb = &fe->eb.mb;
  while( --n >= 0 )
    {
    if( Mb_available_bytes( mb ) >= 4 )
      {
      fe->key4 = ( ( fe->key4 << 4 ) ^ mb->buffer[mb->pos+3] ) & mb->key4_mask;
      mb->pos_array[mb->cyclic_pos] = mb->prev_positions[fe->key4];
      mb->prev_positions[fe->key4] = mb->pos + 1;
      }
    else mb->pos_array[mb->cyclic_pos] = 0;
    if( !Mb_move_pos( mb ) ) return 0;
    }
  return 1;
  }

static inline _Bool FLZe_init( struct FLZ_encoder * const fe,
                              const unsigned long long member_size )
  {
  enum { before_size = 0,
         dict_size = 65536,

         after_size = max_match_len,
         dict_factor = 16,
         min_free_bytes = max_marker_size,
         num_prev_positions23 = 0,
         pos_array_factor = 1 };

  return LZeb_init( &fe->eb, before_size, dict_size, after_size, dict_factor,
                    num_prev_positions23, pos_array_factor, min_free_bytes,
                    member_size );
  }

static inline void FLZe_reset( struct FLZ_encoder * const fe,
                               const unsigned long long member_size )
  { LZeb_reset( &fe->eb, member_size ); }

static int FLZe_longest_match_len( struct FLZ_encoder * const fe, int * const distance )
  {
  enum { len_limit = 16 };
  const uint8_t * const data = Mb_ptr_to_current_pos( &fe->eb.mb );
  int32_t * ptr0 = fe->eb.mb.pos_array + fe->eb.mb.cyclic_pos;
  const int pos1 = fe->eb.mb.pos + 1;
  int maxlen = 0, newpos1, count;
  const int available = ((Mb_available_bytes( &fe->eb.mb )) <= (max_match_len) ? (Mb_available_bytes( &fe->eb.mb )) : (max_match_len));
  if( available < len_limit ) { *ptr0 = 0; return 0; }

  fe->key4 = ( ( fe->key4 << 4 ) ^ data[3] ) & fe->eb.mb.key4_mask;
  newpos1 = fe->eb.mb.prev_positions[fe->key4];
  fe->eb.mb.prev_positions[fe->key4] = pos1;

  for( count = 4; ; )
    {
    int32_t * newptr;
    int delta;
    if( newpos1 <= 0 || --count < 0 ||
        ( delta = pos1 - newpos1 ) > fe->eb.mb.dictionary_size )
      { *ptr0 = 0; break; }
    newptr = fe->eb.mb.pos_array +
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


static _Bool FLZe_encode_member( struct FLZ_encoder * const fe )
  {
  int rep = 0, i;
  State * const state = &fe->eb.state;

  if( fe->eb.member_finished ) return 1;
  if( Re_member_position( &fe->eb.renc ) >= fe->eb.member_size_limit )
    { LZeb_try_full_flush( &fe->eb ); return 1; }

  if( Mb_data_position( &fe->eb.mb ) == 0 &&
      !Mb_data_finished( &fe->eb.mb ) )
    {
    const uint8_t prev_byte = 0;
    uint8_t cur_byte;
    if( !Mb_enough_available_bytes( &fe->eb.mb ) ||
        !Re_enough_free_bytes( &fe->eb.renc ) ) return 1;
    cur_byte = Mb_peek( &fe->eb.mb, 0 );
    Re_encode_bit( &fe->eb.renc, &fe->eb.bm_match[*state][0], 0 );
    LZeb_encode_literal( &fe->eb, prev_byte, cur_byte );
    CRC32_update_byte( &fe->eb.crc, cur_byte );
    FLZe_reset_key4( fe );
    if( !FLZe_update_and_move( fe, 1 ) ) return 0;
    }

  while( !Mb_data_finished( &fe->eb.mb ) &&
         Re_member_position( &fe->eb.renc ) < fe->eb.member_size_limit )
    {
    int match_distance = 0;
    int main_len, pos_state;
    int len = 0;
    if( !Mb_enough_available_bytes( &fe->eb.mb ) ||
        !Re_enough_free_bytes( &fe->eb.renc ) ) return 1;
    main_len = FLZe_longest_match_len( fe, &match_distance );
    pos_state = Mb_data_position( &fe->eb.mb ) & pos_state_mask;

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
        int distance;
        Re_encode_bit( &fe->eb.renc, &fe->eb.bm_rep1[*state], rep > 1 );
        if( rep > 1 )
          Re_encode_bit( &fe->eb.renc, &fe->eb.bm_rep2[*state], rep > 2 );
        distance = fe->eb.reps[rep];
        for( i = rep; i > 0; --i ) fe->eb.reps[i] = fe->eb.reps[i-1];
        fe->eb.reps[0] = distance;
        }
      *state = St_set_rep( *state );
      Re_encode_len( &fe->eb.renc, &fe->eb.rep_len_model, len, pos_state );
      if( !Mb_move_pos( &fe->eb.mb ) ) return 0;
      if( !FLZe_update_and_move( fe, len - 1 ) ) return 0;
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
      if( !Mb_move_pos( &fe->eb.mb ) ) return 0;
      if( !FLZe_update_and_move( fe, main_len - 1 ) ) return 0;
      continue;
      }

    {
    const uint8_t prev_byte = Mb_peek( &fe->eb.mb, 1 );
    const uint8_t cur_byte = Mb_peek( &fe->eb.mb, 0 );
    const uint8_t match_byte = Mb_peek( &fe->eb.mb, fe->eb.reps[0] + 1 );
    if( !Mb_move_pos( &fe->eb.mb ) ) return 0;
    CRC32_update_byte( &fe->eb.crc, cur_byte );

    if( match_byte == cur_byte )
      {
      const int short_rep_price = price1( fe->eb.bm_match[*state][pos_state] ) +
                                  price1( fe->eb.bm_rep[*state] ) +
                                  price0( fe->eb.bm_rep0[*state] ) +
                                  price0( fe->eb.bm_len[*state][pos_state] );
      int price = price0( fe->eb.bm_match[*state][pos_state] );
      if( St_is_char( *state ) )
        price += LZeb_price_literal( &fe->eb, prev_byte, cur_byte );
      else
        price += LZeb_price_matched( &fe->eb, prev_byte, cur_byte, match_byte );
      if( short_rep_price < price )
        {
        Re_encode_bit( &fe->eb.renc, &fe->eb.bm_match[*state][pos_state], 1 );
        Re_encode_bit( &fe->eb.renc, &fe->eb.bm_rep[*state], 1 );
        Re_encode_bit( &fe->eb.renc, &fe->eb.bm_rep0[*state], 0 );
        Re_encode_bit( &fe->eb.renc, &fe->eb.bm_len[*state][pos_state], 0 );
        *state = St_set_short_rep( *state );
        continue;
        }
      }


    Re_encode_bit( &fe->eb.renc, &fe->eb.bm_match[*state][pos_state], 0 );
    if( St_is_char( *state ) )
      LZeb_encode_literal( &fe->eb, prev_byte, cur_byte );
    else
      LZeb_encode_matched( &fe->eb, prev_byte, cur_byte, match_byte );
    *state = St_set_char( *state );
    }
    }

  LZeb_try_full_flush( &fe->eb );
  return 1;
  }


struct LZ_Encoder
  {
  unsigned long long partial_in_size;
  unsigned long long partial_out_size;
  struct LZ_encoder_base * lz_encoder_base;
  struct LZ_encoder * lz_encoder;
  struct FLZ_encoder * flz_encoder;
  enum LZ_Errno lz_errno;
  _Bool fatal;
  };

static void LZ_Encoder_init( struct LZ_Encoder * const e )
  {
  e->partial_in_size = 0;
  e->partial_out_size = 0;
  e->lz_encoder_base = 0;
  e->lz_encoder = 0;
  e->flz_encoder = 0;
  e->lz_errno = LZ_ok;
  e->fatal = 0;
  }


struct LZ_Decoder
  {
  unsigned long long partial_in_size;
  unsigned long long partial_out_size;
  struct Range_decoder * rdec;
  struct LZ_decoder * lz_decoder;
  enum LZ_Errno lz_errno;
  Lzip_header member_header;
  _Bool fatal;
  _Bool first_header;
  _Bool seeking;
  };

static void LZ_Decoder_init( struct LZ_Decoder * const d )
  {
  int i;
  d->partial_in_size = 0;
  d->partial_out_size = 0;
  d->rdec = 0;
  d->lz_decoder = 0;
  d->lz_errno = LZ_ok;
  for( i = 0; i < Lh_size; ++i ) d->member_header[i] = 0;
  d->fatal = 0;
  d->first_header = 1;
  d->seeking = 0;
  }


static _Bool verify_encoder( struct LZ_Encoder * const e )
  {
  if( !e ) return 0;
  if( !e->lz_encoder_base || ( !e->lz_encoder && !e->flz_encoder ) ||
      ( e->lz_encoder && e->flz_encoder ) )
    { e->lz_errno = LZ_bad_argument; return 0; }
  return 1;
  }


static _Bool verify_decoder( struct LZ_Decoder * const d )
  {
  if( !d ) return 0;
  if( !d->rdec )
    { d->lz_errno = LZ_bad_argument; return 0; }
  return 1;
  }




int LZ_api_version( void ) { return 1012; }

const char * LZ_version( void ) { return LZ_version_string; }

const char * LZ_strerror( const enum LZ_Errno lz_errno )
  {
  switch( lz_errno )
    {
    case LZ_ok : return "ok";
    case LZ_bad_argument : return "Bad argument";
    case LZ_mem_error : return "Not enough memory";
    case LZ_sequence_error: return "Sequence error";
    case LZ_header_error : return "Header error";
    case LZ_unexpected_eof: return "Unexpected EOF";
    case LZ_data_error : return "Data error";
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




struct LZ_Encoder * LZ_compress_open( const int dictionary_size,
                                      const int match_len_limit,
                                      const unsigned long long member_size )
  {
  Lzip_header header;
  struct LZ_Encoder * const e =
    (struct LZ_Encoder *)malloc( sizeof (struct LZ_Encoder) );
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
      e->flz_encoder = (struct FLZ_encoder *)malloc( sizeof (struct FLZ_encoder) );
      if( e->flz_encoder && FLZe_init( e->flz_encoder, member_size ) )
        { e->lz_encoder_base = &e->flz_encoder->eb; return e; }
      free( e->flz_encoder ); e->flz_encoder = 0;
      }
    else
      {
      e->lz_encoder = (struct LZ_encoder *)malloc( sizeof (struct LZ_encoder) );
      if( e->lz_encoder && LZe_init( e->lz_encoder, Lh_get_dictionary_size( header ),
                                     match_len_limit, member_size ) )
        { e->lz_encoder_base = &e->lz_encoder->eb; return e; }
      free( e->lz_encoder ); e->lz_encoder = 0;
      }
    e->lz_errno = LZ_mem_error;
    }
  e->fatal = 1;
  return e;
  }


int LZ_compress_close( struct LZ_Encoder * const e )
  {
  if( !e ) return -1;
  if( e->lz_encoder_base )
    { LZeb_free( e->lz_encoder_base );
      free( e->lz_encoder ); free( e->flz_encoder ); }
  free( e );
  return 0;
  }


int LZ_compress_finish( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) || e->fatal ) return -1;
  Mb_finish( &e->lz_encoder_base->mb );


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


int LZ_compress_restart_member( struct LZ_Encoder * const e,
                                const unsigned long long member_size )
  {
  if( !verify_encoder( e ) || e->fatal ) return -1;
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


int LZ_compress_sync_flush( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) || e->fatal ) return -1;
  if( !e->lz_encoder_base->mb.at_stream_end )
    e->lz_encoder_base->mb.sync_flush_pending = 1;
  return 0;
  }


int LZ_compress_read( struct LZ_Encoder * const e,
                      uint8_t * const buffer, const int size )
  {
  if( !verify_encoder( e ) || e->fatal ) return -1;
  if( size < 0 ) return 0;

  { struct LZ_encoder_base * const eb = e->lz_encoder_base;
  int out_size = Re_read_data( &eb->renc, buffer, size );

  if( out_size < size || size == 0 )
    {
    if( ( e->flz_encoder && !FLZe_encode_member( e->flz_encoder ) ) ||
        ( e->lz_encoder && !LZe_encode_member( e->lz_encoder ) ) )
      { e->lz_errno = LZ_library_error; e->fatal = 1; return -1; }
    if( eb->mb.sync_flush_pending && Mb_available_bytes( &eb->mb ) <= 0 )
      LZeb_try_sync_flush( eb );
    out_size += Re_read_data( &eb->renc, buffer + out_size, size - out_size );
    }
  return out_size; }
  }


int LZ_compress_write( struct LZ_Encoder * const e,
                       const uint8_t * const buffer, const int size )
  {
  if( !verify_encoder( e ) || e->fatal ) return -1;
  return Mb_write_data( &e->lz_encoder_base->mb, buffer, size );
  }


int LZ_compress_write_size( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) || e->fatal ) return -1;
  return Mb_free_bytes( &e->lz_encoder_base->mb );
  }


enum LZ_Errno LZ_compress_errno( struct LZ_Encoder * const e )
  {
  if( !e ) return LZ_bad_argument;
  return e->lz_errno;
  }


int LZ_compress_finished( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) ) return -1;
  return ( Mb_data_finished( &e->lz_encoder_base->mb ) &&
           LZeb_member_finished( e->lz_encoder_base ) );
  }


int LZ_compress_member_finished( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) ) return -1;
  return LZeb_member_finished( e->lz_encoder_base );
  }


unsigned long long LZ_compress_data_position( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) ) return 0;
  return Mb_data_position( &e->lz_encoder_base->mb );
  }


unsigned long long LZ_compress_member_position( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) ) return 0;
  return Re_member_position( &e->lz_encoder_base->renc );
  }


unsigned long long LZ_compress_total_in_size( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) ) return 0;
  return e->partial_in_size + Mb_data_position( &e->lz_encoder_base->mb );
  }


unsigned long long LZ_compress_total_out_size( struct LZ_Encoder * const e )
  {
  if( !verify_encoder( e ) ) return 0;
  return e->partial_out_size + Re_member_position( &e->lz_encoder_base->renc );
  }




struct LZ_Decoder * LZ_decompress_open( void )
  {
  struct LZ_Decoder * const d =
    (struct LZ_Decoder *)malloc( sizeof (struct LZ_Decoder) );
  if( !d ) return 0;
  LZ_Decoder_init( d );

  d->rdec = (struct Range_decoder *)malloc( sizeof (struct Range_decoder) );
  if( !d->rdec || !Rd_init( d->rdec ) )
    {
    if( d->rdec ) { Rd_free( d->rdec ); free( d->rdec ); d->rdec = 0; }
    d->lz_errno = LZ_mem_error; d->fatal = 1;
    }
  return d;
  }


int LZ_decompress_close( struct LZ_Decoder * const d )
  {
  if( !d ) return -1;
  if( d->lz_decoder )
    { LZd_free( d->lz_decoder ); free( d->lz_decoder ); }
  if( d->rdec ) { Rd_free( d->rdec ); free( d->rdec ); }
  free( d );
  return 0;
  }


int LZ_decompress_finish( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) || d->fatal ) return -1;
  if( d->seeking )
    { d->seeking = 0; d->partial_in_size += Rd_purge( d->rdec ); }
  else Rd_finish( d->rdec );
  return 0;
  }


int LZ_decompress_reset( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return -1;
  if( d->lz_decoder )
    { LZd_free( d->lz_decoder ); free( d->lz_decoder ); d->lz_decoder = 0; }
  d->partial_in_size = 0;
  d->partial_out_size = 0;
  Rd_reset( d->rdec );
  d->lz_errno = LZ_ok;
  d->fatal = 0;
  d->first_header = 1;
  d->seeking = 0;
  return 0;
  }


int LZ_decompress_sync_to_member( struct LZ_Decoder * const d )
  {
  unsigned skipped = 0;
  if( !verify_decoder( d ) ) return -1;
  if( d->lz_decoder )
    { LZd_free( d->lz_decoder ); free( d->lz_decoder ); d->lz_decoder = 0; }
  if( Rd_find_header( d->rdec, &skipped ) ) d->seeking = 0;
  else
    {
    if( !d->rdec->at_stream_end ) d->seeking = 1;
    else { d->seeking = 0; d->partial_in_size += Rd_purge( d->rdec ); }
    }
  d->partial_in_size += skipped;
  d->lz_errno = LZ_ok;
  d->fatal = 0;
  return 0;
  }


int LZ_decompress_read( struct LZ_Decoder * const d,
                        uint8_t * const buffer, const int size )
  {
  int result;
  if( !verify_decoder( d ) ) return -1;
  if( size < 0 ) return 0;
  if( d->fatal )
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
    if( rd < Lh_size || Rd_finished( d->rdec ) )
      {
      if( rd <= 0 || Lh_verify_prefix( d->member_header, rd ) )
        d->lz_errno = LZ_unexpected_eof;
      else
        d->lz_errno = LZ_header_error;
      d->fatal = 1;
      return -1;
      }
    if( !Lh_verify_magic( d->member_header ) )
      {


      if( Rd_unread_data( d->rdec, rd ) )
        {
        if( d->first_header || !Lh_verify_corrupt( d->member_header ) )
          d->lz_errno = LZ_header_error;
        else
          d->lz_errno = LZ_data_error;
        }
      else
        d->lz_errno = LZ_library_error;
      d->fatal = 1;
      return -1;
      }
    if( !Lh_verify_version( d->member_header ) ||
        !isvalid_ds( Lh_get_dictionary_size( d->member_header ) ) )
      {


      if( Rd_unread_data( d->rdec, 1 + !Lh_verify_version( d->member_header ) ) )
        d->lz_errno = LZ_data_error;
      else
        d->lz_errno = LZ_library_error;
      d->fatal = 1;
      return -1;
      }
    d->first_header = 0;
    if( Rd_available_bytes( d->rdec ) < 5 )
      {

      d->rdec->member_position += Cb_used_bytes( &d->rdec->cb );
      Cb_reset( &d->rdec->cb );
      d->lz_errno = LZ_unexpected_eof;
      d->fatal = 1;
      return -1;
      }
    d->lz_decoder = (struct LZ_decoder *)malloc( sizeof (struct LZ_decoder) );
    if( !d->lz_decoder || !LZd_init( d->lz_decoder, d->rdec,
                             Lh_get_dictionary_size( d->member_header ) ) )
      {
      if( d->lz_decoder )
        { LZd_free( d->lz_decoder ); free( d->lz_decoder ); d->lz_decoder = 0; }
      d->lz_errno = LZ_mem_error;
      d->fatal = 1;
      return -1;
      }
    d->rdec->reload_pending = 1;
    }
  result = LZd_decode_member( d->lz_decoder );
  if( result != 0 )
    {
    if( result == 2 )
      { d->rdec->member_position += Cb_used_bytes( &d->rdec->cb );
        Cb_reset( &d->rdec->cb );
        d->lz_errno = LZ_unexpected_eof; }
    else if( result == 5 ) d->lz_errno = LZ_library_error;
    else d->lz_errno = LZ_data_error;
    d->fatal = 1;
    if( Cb_empty( &d->lz_decoder->cb ) ) return -1;
    }
get_data:
  return Cb_read_data( &d->lz_decoder->cb, buffer, size );
  }


int LZ_decompress_write( struct LZ_Decoder * const d,
                         const uint8_t * const buffer, const int size )
  {
  int result;
  if( !verify_decoder( d ) || d->fatal ) return -1;
  if( size < 0 ) return 0;

  result = Rd_write_data( d->rdec, buffer, size );
  while( d->seeking )
    {
    int size2;
    unsigned skipped = 0;
    if( Rd_find_header( d->rdec, &skipped ) ) d->seeking = 0;
    d->partial_in_size += skipped;
    if( result >= size ) break;
    size2 = Rd_write_data( d->rdec, buffer + result, size - result );
    if( size2 > 0 ) result += size2;
    else break;
    }
  return result;
  }


int LZ_decompress_write_size( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) || d->fatal ) return -1;
  return Rd_free_bytes( d->rdec );
  }


enum LZ_Errno LZ_decompress_errno( struct LZ_Decoder * const d )
  {
  if( !d ) return LZ_bad_argument;
  return d->lz_errno;
  }


int LZ_decompress_finished( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) || d->fatal ) return -1;
  return ( Rd_finished( d->rdec ) &&
           ( !d->lz_decoder || LZd_member_finished( d->lz_decoder ) ) );
  }


int LZ_decompress_member_finished( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) || d->fatal ) return -1;
  return ( d->lz_decoder && LZd_member_finished( d->lz_decoder ) );
  }


int LZ_decompress_member_version( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return -1;
  return Lh_version( d->member_header );
  }


int LZ_decompress_dictionary_size( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return -1;
  return Lh_get_dictionary_size( d->member_header );
  }


unsigned LZ_decompress_data_crc( struct LZ_Decoder * const d )
  {
  if( verify_decoder( d ) && d->lz_decoder )
    return LZd_crc( d->lz_decoder );
  return 0;
  }


unsigned long long LZ_decompress_data_position( struct LZ_Decoder * const d )
  {
  if( verify_decoder( d ) && d->lz_decoder )
    return LZd_data_position( d->lz_decoder );
  return 0;
  }


unsigned long long LZ_decompress_member_position( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return 0;
  return d->rdec->member_position;
  }


unsigned long long LZ_decompress_total_in_size( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return 0;
  return d->partial_in_size + d->rdec->member_position;
  }


unsigned long long LZ_decompress_total_out_size( struct LZ_Decoder * const d )
  {
  if( !verify_decoder( d ) ) return 0;
  if( d->lz_decoder )
    return d->partial_out_size + LZd_data_position( d->lz_decoder );
  return d->partial_out_size;
  }

