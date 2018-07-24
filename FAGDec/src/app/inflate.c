#include <zlib.h>
#define CHECK_ERR(err) { \
    if (err != Z_OK) { \
        return err;\
    } \
}
int test_large_inflate(Byte *compr, uLong comprLen, Byte *uncompr, uLong uncomprLen) {
    int err;
    z_stream d_stream; /* decompression stream */

    d_stream.zalloc = Z_NULL;
    d_stream.zfree = Z_NULL;
    d_stream.opaque = Z_NULL;

    d_stream.next_in  = compr;
    d_stream.avail_in = (uInt)comprLen;

    err = inflateInit(&d_stream);
    CHECK_ERR(err);

    for (;;) {
        d_stream.next_out = uncompr;            /* discard the output */
        d_stream.avail_out = (uInt)uncomprLen;
        err = inflate(&d_stream, Z_NO_FLUSH);
        if (err == Z_STREAM_END) break;
        CHECK_ERR(err);
    }

    err = inflateEnd(&d_stream);
    CHECK_ERR(err);
	return err;
}
