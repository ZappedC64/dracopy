/** @file
 * RLE codec for DraCopy's RLE file copy.
 *
 * Format (matches the original RLECOPY rle.asm $76-escape scheme):
 *   escape marker = $76
 *   any non-$76 byte          -> emitted verbatim
 *   a literal $76 in source   -> $76 $00
 *   a run of N>=3 equal bytes -> $76 N byte   (N is 3..255)
 *   runs of 1 or 2            -> emitted literally
 * The decoder is bounded by the original byte count, so trailing
 * partial tokens (e.g. from a truncated/overflowed buffer) are ignored.
 */
#ifndef RLE_H
#define RLE_H

#include "defines.h"

/* streaming encoder into the memory window [out, out+cap) */
void     rle_init(BYTE *out, unsigned cap);
void     rle_encode(BYTE b);     /* feed one source byte */
void     rle_finish(void);       /* flush the trailing run */
unsigned rle_outlen(void);       /* compressed bytes produced so far */
BYTE     rle_overflow(void);     /* nonzero once the window was exceeded */

/* decode 'origlen' bytes from 'comp' and cbm_write() them to channel 'lfn'.
   returns 0 on success, -1 on a write error. */
int      rle_decompress_to_cbm(const BYTE *comp, unsigned origlen, BYTE lfn);

/* one-shot compress 'len' bytes of 'src' into 'out' (capacity 'cap').
   returns the compressed length, or 0 if it didn't fit. */
unsigned rle_compress_block(const BYTE *src, unsigned len, BYTE *out, unsigned cap);

/* decode 'origlen' bytes from 'comp' into the memory buffer 'dst'. */
void     rle_decompress_to_mem(const BYTE *comp, BYTE *dst, unsigned origlen);

#endif
