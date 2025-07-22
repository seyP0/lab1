#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h> // for ntohl, htonl
#include "lab_png.h"
#include "crc.h"

// Check PNG signature
int is_png(U8 *buf, size_t n) {
    if (n < PNG_SIG_SIZE) return 0;
    U8 expected_signature[PNG_SIG_SIZE] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    return memcmp(buf, expected_signature, PNG_SIG_SIZE) == 0;
}

// Read IHDR data
int get_png_data_IHDR(struct data_IHDR *out, FILE *fp, long offset, int whence) {
    if (!out || fseek(fp, offset, whence) != 0) return 0;

    U32 length;
    if (fread(&length, sizeof(U32), 1, fp) != 1) return 0;
    length = ntohl(length);

    U8 type[CHUNK_TYPE_SIZE];
    if (fread(type, 1, CHUNK_TYPE_SIZE, fp) != CHUNK_TYPE_SIZE) return 0;
    if (memcmp(type, "IHDR", CHUNK_TYPE_SIZE) != 0 || length != DATA_IHDR_SIZE) return 0;

    U8 buf[DATA_IHDR_SIZE];
    if (fread(buf, 1, DATA_IHDR_SIZE, fp) != DATA_IHDR_SIZE) return 0;

    out->width      = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    out->height     = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
    out->bit_depth  = buf[8];
    out->color_type = buf[9];
    out->compression = buf[10];
    out->filter     = buf[11];
    out->interlace  = buf[12];

    fseek(fp, CHUNK_CRC_SIZE, SEEK_CUR); // Skip CRC
    return 1;
}

// Read a chunk from file
chunk_p get_chunk(FILE *fp) {
    if (!fp) return NULL;

    chunk_p c = malloc(sizeof(struct chunk));
    if (!c) return NULL;

    U32 net_len;
    if (fread(&net_len, 4, 1, fp) != 1) { free(c); return NULL; }
    c->length = ntohl(net_len);

    if (fread(c->type, 1, CHUNK_TYPE_SIZE, fp) != CHUNK_TYPE_SIZE) { free(c); return NULL; }

    c->p_data = malloc(c->length);
    if (!c->p_data || fread(c->p_data, 1, c->length, fp) != c->length) {
        free_chunk(c);
        return NULL;
    }

    U32 net_crc;
    if (fread(&net_crc, 4, 1, fp) != 1) { free_chunk(c); return NULL; }
    c->crc = ntohl(net_crc);

    return c;
}

// Populate a simple_PNG struct from file
int get_png_chunks(simple_PNG_p out, FILE *fp, long offset, int whence) {
    if (!out || fseek(fp, offset, whence) != 0) return 0;

    out->p_IHDR = get_chunk(fp);
    out->p_IDAT = get_chunk(fp);
    out->p_IEND = get_chunk(fp);

    if (!out->p_IHDR || !out->p_IDAT || !out->p_IEND) return 0;
    return 1;
}

// Allocate a PNG container
simple_PNG_p mallocPNG() {
    simple_PNG_p p = malloc(sizeof(struct simple_PNG));
    if (p) p->p_IHDR = p->p_IDAT = p->p_IEND = NULL;
    return p;
}

// Free all memory in a PNG container
void free_png(simple_PNG_p in) {
    if (!in) return;
    free_chunk(in->p_IHDR);
    free_chunk(in->p_IDAT);
    free_chunk(in->p_IEND);
    free(in);
}

// Free a chunk and its data
void free_chunk(chunk_p in) {
    if (!in) return;
    free(in->p_data);
    free(in);
}

// Chunk CRC functions
U32 get_chunk_crc(chunk_p in) {
    return in ? in->crc : 0;
}

U32 calculate_chunk_crc(chunk_p in) {
    if (!in) return 0;
    U8 *buf = malloc(in->length + CHUNK_TYPE_SIZE);
    if (!buf) return 0;
    memcpy(buf, in->type, CHUNK_TYPE_SIZE);
    memcpy(buf + CHUNK_TYPE_SIZE, in->p_data, in->length);
    U32 crc_val = crc(buf, in->length + CHUNK_TYPE_SIZE);
    free(buf);
    return crc_val;
}

// Write a chunk to a file
int write_chunk(FILE* fp, chunk_p in) {
    if (!fp || !in) return 0;

    U32 net_len = htonl(in->length);
    U32 net_crc = htonl(in->crc);

    fwrite(&net_len, sizeof(U32), 1, fp);
    fwrite(in->type, sizeof(U8), CHUNK_TYPE_SIZE, fp);
    fwrite(in->p_data, sizeof(U8), in->length, fp);
    fwrite(&net_crc, sizeof(U32), 1, fp);

    return 1;
}

// Write a PNG to a file
// In src/lab_png.c, replace the write_PNG function (around line 141)
int write_PNG(char* filepath, simple_PNG_p in) {
    if (!filepath || !in || !in->p_IHDR || !in->p_IDAT || !in->p_IEND) return 0;

    FILE *fp = fopen(filepath, "wb");
    if (!fp) return 0;

    // Write PNG signature
    U8 sig[PNG_SIG_SIZE] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    if (fwrite(sig, 1, PNG_SIG_SIZE, fp) != PNG_SIG_SIZE) {
        fclose(fp);
        return 0;
    }

    // Write IHDR, IDAT, and IEND chunks
    int success = write_chunk(fp, in->p_IHDR) &&
                  write_chunk(fp, in->p_IDAT) &&
                  write_chunk(fp, in->p_IEND);

    fclose(fp);
    return success ? 1 : 0;
}
