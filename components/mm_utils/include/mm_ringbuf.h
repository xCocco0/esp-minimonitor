/**
 * @file mm_ringbuf.h
 * @mainpage Ring Buffer
 *
 * @brief
 * Basic ring buffer implementation with buffer wrapping and overwriting behavior
 *
 *******************************************************************************
 *
 * # List of functions
 *
 * - @ref MM_RINGBUF_DECLARE
 * - @ref MM_RINGBUF_DEFINE
 * - @ref MM_RINGBUF_DEFINE_FROM_BUF
 * - @ref MM_RINGBUF_BUFSIZE
 * - @ref MM_RINGBUF_USED_BYTES
 * - @ref MM_RINGBUF_WRITE_BACK
 * - @ref MM_RINGBUF_PEEK
 * - @ref MM_RINGBUF_READ
 *
 *******************************************************************************
 *
 * # How to Use
 *
 * ## Define the ring buffer
 *
 * Define with buffer:
 * mm_ringbuf_t my_ringbuf = MM_RINGBUF_INIT(buffer, length);
 * ```
 *
 * Or add buffer later on (e.g. malloc):
 * ```
 * MM_RINGBUF_SET_BUFFER(&my_ringbuf, buffer, 8);
 * ```
 *
 * ## Examples
 *
 * This example shows how to allocate a buffer:
 * ```c
 * #define N 8
 *
 * mm_ringbuf_t my_ringbuf1, my_ringbuf2;
 *
 * // Option 1: global:
 * static char buf[N];
 * mm_ringbuf_t my_ringbuf1 = MM_RINGBUF_DEFINE_FROM_BUF(N, buf);
 *
 * // Option 2: malloc
 * int main(int argc, char *argv[])
 * {
 *     char *buf = malloc(N);
 *     if(!buf) return -1;
 *     MM_RINGBUF_SET_BUFFER(&my_ringbuf2, buf, 8);
 *     // ...
 * }
 * ```
 *
 * ## Buffer layout
 *
 * ```
 * Buffer without wrapping:
 *
 *  0       8         19     25
 *  |       |          |     |
 * [--------###########------]
 *  |       |          |     |
 * buf      |       buf+end  |
 *       buf+begin        buf+bufsize
 *
 * Buffer with wrapping:
 *
 * [#####-------------#######]
 *       |            |
 *       end          begin
 *
 * Empty buffer:
 *
 * [-----------]
 *      |
 *      begin
 *      end
 *
 * Buffer with 1 byte:
 *
 * [----#------]
 *      ||
 *  begin|
 *      end
 *
 * Full buffer:
 *
 * [####-######]
 *      ||
 *      |begin
 *     end
 *
 * [##########-]
 *  |         |
 *  begin     end
 *
 * ```
 */

#include <string.h>

#define __MM_RINGBUF_MIN(a,b) ((a) < (b) ? (a) : (b))

struct mm_ringbuf {
        size_t begin;
        size_t end;
        size_t bufsize;
        char *buf;
};

typedef struct mm_ringbuf mm_ringbuf_t;

/** @brief Initializes an empty ring buffer from a pre-allocated buffer.
 * @param len Size of the buffer in bytes.
 * @param buffer The buffer to use in the ring buffer structure.
 */
#define MM_RINGBUF_INIT(buffer, len) {.begin = 0, .end = 0, .bufsize = (len), .buf = (buffer) }

/** @brief Assigns a buffer to a ring buffer.
 * The ring buffer should not be already initialized with another buffer, or should be initialized with a buffer size
 * of 0.
 * @param len Size of the buffer in bytes.
 * @param buffer The buffer to use in the ring buffer structure.
 */
static inline mm_ringbuf_t *MM_RINGBUF_SET_BUFFER(mm_ringbuf_t *rb, void *buf, size_t len)
{
        *rb = (struct mm_ringbuf) MM_RINGBUF_INIT((char *) buf, len);
        return rb;
}

/** @brief Get the length of the ring buffer's interal buffer.
 * @param rb (mm_ringbuf_t *) The ring buffer.
 */
#define MM_RINGBUF_BUF_SIZE(rb) (((struct mm_ringbuf *) rb)->bufsize)

/** @brief Retrieves the currently used bytes in the ring buffer's buffer.
 * @param rb The ring buffer.
 */
static inline size_t MM_RINGBUF_USED_BYTES(mm_ringbuf_t *rb)
{
        struct mm_ringbuf *b = (struct mm_ringbuf *) rb;
        if (b->end >= b->begin) {
                return b->end - b->begin;
        } else {
                return b->bufsize - (b->begin - b->end);
        }

}

/** @brief Retrieves the currently available bytes in the ring buffer's buffer.
 * @param rb (mm_ringbuf_t *) The ring buffer.
 */
#define MM_RINGBUF_AVAILABLE_BYTES(rb) ((MM_RINGBUF_BUFSIZE(rb) - 1) - MM_RINGBUF_USED_BYTES(rb))


/** @brief Writes at the end of the ring buffer.
 * @param rb The ring buffer.
 * @param data The pointer of the data to copy to buffer.
 * @param datasize The amount of data to copy to buffer.
 * @return The number of bytes that were written in the ring buffer.
 */
static inline size_t MM_RINGBUF_WRITE_BACK(mm_ringbuf_t *rb, const char *data, size_t datasize)
{
        size_t right_space;
        size_t block_size;
        size_t bytes_left;
        struct mm_ringbuf *b = (struct mm_ringbuf *) rb;

        if(datasize <= 0) {
                return 0;
        }

        size_t available_before = MM_RINGBUF_USED_BYTES(rb);

        /* input buffer too large */
        if(datasize > b->bufsize - 1) {
                data += (datasize - (b->bufsize - 1));
                datasize = b->bufsize - 1;
        }

        bytes_left = datasize;
        while(bytes_left > 0) {
                right_space = b->bufsize - b->end;
                block_size = __MM_RINGBUF_MIN(right_space, bytes_left);

                memcpy(b->buf + b->end, data, block_size);

                b->end = (b->end + block_size) % b->bufsize;
                data += block_size;
                bytes_left -= block_size;
        }

        /* shift begin index */
        if(available_before + datasize > b->bufsize - 1) {
                size_t overflow = (available_before + datasize) - (b->bufsize - 1);
                b->begin = (b->begin + overflow) % b->bufsize;
        }

        return datasize;
}

/** @brief Read from buffer without consuming data.
 * @param rb The ring buffer.
 * @param data A pointer to the buffer where data will be stored.
 * @param datasize The maximum number of bytes to read.
 * @return The number of bytes that were written to data.
 */
static inline size_t MM_RINGBUF_PEEK(mm_ringbuf_t *rb, char *data, size_t datasize)
{
        struct mm_ringbuf *b = (struct mm_ringbuf *) rb;
        size_t blocksize, ret = 0;
        if(b->begin <= b->end) {
                /* send whole block */
                blocksize = __MM_RINGBUF_MIN(b->end - b->begin, datasize);
                memcpy(data, b->buf + b->begin, blocksize);
                ret += blocksize;
        } else {
                /* send with right part */
                blocksize = __MM_RINGBUF_MIN(b->bufsize - b->begin, datasize);
                memcpy(data, b->buf + b->begin, blocksize);
                ret += blocksize;
                if(blocksize < datasize) {
                        /* send left part */
                        data += blocksize;
                        datasize -= blocksize;
                        blocksize = __MM_RINGBUF_MIN(b->end, datasize);
                        memcpy(data, b->buf, blocksize);
                        ret += blocksize;
                }
        }
        return ret;
}

/** @brief Read and consume from buffer.
 * A call to this function actually removes data in the ring buffer.
 * @param rb The ring buffer.
 * @param data A pointer to the buffer where data will be stored.
 * @param datasize The maximum number of bytes to write to read.
 * @return The number of bytes that were written to data.
 */
static inline size_t MM_RINGBUF_READ(mm_ringbuf_t *rb, char *data, size_t datasize)
{
        struct mm_ringbuf *b = (struct mm_ringbuf *) rb;
        size_t bytes_read;

        bytes_read = MM_RINGBUF_PEEK(rb, data, datasize);
        b->begin = (b->begin + bytes_read) % b->bufsize;

        return bytes_read;
}
