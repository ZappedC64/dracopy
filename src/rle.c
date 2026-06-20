/** @file
 * RLE codec for DraCopy's RLE file copy.  See rle.h for the format.
 * Ported from the original RLECOPY rle.asm so the byte stream is identical.
 */

#include "rle.h"
#include <cbm.h>

#define ESCAPE 0x76

/* ---- streaming encoder -> memory window ---- */
static BYTE     *o_base;
static BYTE     *o_ptr;
static BYTE     *o_end;
static BYTE      o_full;
static BYTE      run_byte;
static unsigned  run_count;      /* 0..255 */

static void
emit(BYTE b)
{
  if (o_ptr < o_end)
    *o_ptr++ = b;
  else
    o_full = 1;
}

static void
flush_run(void)
{
  if (run_count == 0)
    return;
  if (run_count >= 3)
    {
      emit(ESCAPE);
      emit((BYTE)run_count);
      emit(run_byte);
    }
  else
    {
      unsigned i;
      for (i = 0; i < run_count; ++i)
        {
          if (run_byte == ESCAPE)
            {
              emit(ESCAPE);
              emit(0);
            }
          else
            {
              emit(run_byte);
            }
        }
    }
  run_count = 0;
}

void
rle_init(BYTE *out, unsigned cap)
{
  o_base = o_ptr = out;
  o_end = out + cap;
  o_full = 0;
  run_count = 0;
}

void
rle_encode(BYTE b)
{
  if (run_count == 0)
    {
      run_byte = b;
      run_count = 1;
      return;
    }
  if (b == run_byte)
    {
      if (run_count == 255)
        {
          flush_run();          /* emit the maxed-out run ... */
          run_byte = b;         /* ... and start a fresh one */
          run_count = 1;
        }
      else
        {
          ++run_count;
        }
      return;
    }
  flush_run();
  run_byte = b;
  run_count = 1;
}

void
rle_finish(void)
{
  flush_run();
}

unsigned
rle_outlen(void)
{
  return (unsigned)(o_ptr - o_base);
}

BYTE
rle_overflow(void)
{
  return o_full;
}

/* ---- decoder -> open CBM channel ---- */
static BYTE     ob[254];
static unsigned op;
static BYTE     o_lfn;
static BYTE     o_wrerr;

static void
oput(BYTE b)
{
  ob[op++] = b;
  if (op == sizeof(ob))
    {
      if (cbm_write(o_lfn, ob, sizeof(ob)) != sizeof(ob))
        o_wrerr = 1;
      op = 0;
    }
}

int
rle_decompress_to_cbm(const BYTE *comp, unsigned origlen, BYTE lfn)
{
  unsigned ci = 0;

  op = 0;
  o_lfn = lfn;
  o_wrerr = 0;

  while (origlen && !o_wrerr)
    {
      BYTE c = comp[ci++];
      if (c == ESCAPE)
        {
          BYTE n = comp[ci++];
          if (n == 0)
            {
              oput(ESCAPE);
              --origlen;
            }
          else
            {
              BYTE v = comp[ci++];
              while (n-- && origlen)
                {
                  oput(v);
                  --origlen;
                }
            }
        }
      else
        {
          oput(c);
          --origlen;
        }
    }

  if (op && !o_wrerr)
    {
      if (cbm_write(o_lfn, ob, op) != (int)op)
        o_wrerr = 1;
    }
  return o_wrerr ? -1 : 0;
}

/* ---- one-shot block helpers (used by the disk-sector copy) ---- */
unsigned
rle_compress_block(const BYTE *src, unsigned len, BYTE *out, unsigned cap)
{
  unsigned i;
  rle_init(out, cap);
  for (i = 0; i < len; ++i)
    rle_encode(src[i]);
  rle_finish();
  if (rle_overflow())
    return 0;
  return rle_outlen();
}

void
rle_decompress_to_mem(const BYTE *comp, BYTE *dst, unsigned origlen)
{
  unsigned ci = 0;
  unsigned di = 0;
  while (di < origlen)
    {
      BYTE c = comp[ci++];
      if (c == ESCAPE)
        {
          BYTE n = comp[ci++];
          if (n == 0)
            {
              dst[di++] = ESCAPE;
            }
          else
            {
              BYTE v = comp[ci++];
              while (n-- && di < origlen)
                dst[di++] = v;
            }
        }
      else
        {
          dst[di++] = c;
        }
    }
}
