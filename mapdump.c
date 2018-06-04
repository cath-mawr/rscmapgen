/* mapdump.c. author: stormy. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "models.h"

#define MAPSTARTX	(48)
#define MAPSTARTY	(39)
#define MAPSIZE		(17)
#define CHUNKSIZE	(48)
#define CHUNKAREA	(CHUNKSIZE * CHUNKSIZE)
#define TILESIZE	(3)
#define XSIZE		((MAPSIZE * CHUNKSIZE) * TILESIZE)
#define YSIZE		((MAPSIZE * CHUNKSIZE) * TILESIZE)
#define IMGSIZE		(XSIZE * YSIZE)

struct chunk {
	uint8_t elevation[4][CHUNKAREA];
	uint8_t colour[4][CHUNKAREA];
	uint8_t v_boundaries[4][CHUNKAREA];
	uint8_t h_boundaries[4][CHUNKAREA];
	uint16_t d_boundaries[4][CHUNKAREA];
	uint8_t roofing[4][CHUNKAREA];
	uint8_t overlays[4][CHUNKAREA];
	uint8_t rotation[4][CHUNKAREA];
};

struct world {
	struct chunk chunks[MAPSIZE][MAPSIZE];
};

static uint32_t colour_tab[256];

int
load_chunk(struct chunk *c, unsigned x, unsigned y, unsigned z)
{
	FILE *f;
	char name[128];

	snprintf(name, sizeof(name),
		"/home/sks/sakaki/rs/game/data2/Map/%u-%u-%u.dat",
		x, y, z);
	f = fopen(name, "rb");
	if (f == NULL) {
		if (errno == ENOENT) {
			memset(c->overlays[z], 2, sizeof(c->overlays[z]));
			return 0;
		}
		return 1;
	}
	puts(name);
	fread(c->colour[z], 1, CHUNKAREA, f);
	fread(c->elevation[z], 1, CHUNKAREA, f);
	fread(c->h_boundaries[z], 1, CHUNKAREA, f);
	fread(c->v_boundaries[z], 1, CHUNKAREA, f);
	fread(c->d_boundaries[z], 2, CHUNKAREA, f);
	fread(c->roofing[z], 1, CHUNKAREA, f);
	fread(c->overlays[z], 1, CHUNKAREA, f);
	fread(c->rotation[z], 1, CHUNKAREA, f);
	fclose(f);
	return 0;
}

uint32_t
get_tile_colour(const struct chunk *c, unsigned x, unsigned y)
{
	const unsigned pos = x + y * CHUNKSIZE;

	switch (c->overlays[0][pos]) {
	case 0: return colour_tab[c->colour[0][pos]];
	case 1: return 0x40403f;
	case 2: return 0x1f3f7d;
	case 3: return 0x603001;
	case 4: return 0x603001;
	case 5: return 0x40403f;
	case 6: return 0x6b030f;
	case 7: return 0x0e2c38;
	case 8: return 0x000000;
	case 9: return 0x606260;
	case 10: return 0x000000;
	case 11: return 0x9f5000;
	case 12: return 0x613000;
	case 13: return 0x1f3f7d;
	case 14: return 0x40403f;
	case 15: return 0x241609;
	case 16: return 0x241609;
	case 17: return 0x797b7a;
	case 23: return 0x483000;
	case 21: return 0x000000;
	case 24: return 0x3f2200;
	default:
		printf("Missing overlay definition: %u\n", c->overlays[0][pos]);
		return 0x0;
	}
}

int
write_chunk(uint32_t *pixels, unsigned chunkx, unsigned chunky, const struct chunk *c)
{
	unsigned x, y;
	unsigned ox, oy;
	unsigned dx, dy;
	unsigned pos;
	unsigned i;
	uint32_t colour;

	for (x = 0; x < CHUNKSIZE; ++x) {
		for (y = 0; y < CHUNKSIZE; ++y) {
			colour = get_tile_colour(c, x, y);
			/*ox = ((MAPSIZE - chunkx) * CHUNKSIZE) + (CHUNKSIZE - y);
			oy = (chunky * CHUNKSIZE) + x;*/

			/*pixels[ox + oy * XSIZE] = colour;*/

			ox = (((MAPSIZE - chunkx) * CHUNKSIZE) + (CHUNKSIZE - y)) * TILESIZE;
			oy = ((chunky * CHUNKSIZE) + x) * TILESIZE;
			pos = x + y * CHUNKSIZE;

			for (dx = ox; dx < (ox + TILESIZE); ++dx) {
				for (dy = oy; dy < (oy + TILESIZE); ++dy) {
					pixels[dx + dy * XSIZE] = colour;
				}
			}

			if (c->h_boundaries[0][pos] != 0) {
				/* vertical */
				if (x > 0 && c->overlays[(x - 1) + y * CHUNKSIZE] == 0) {
					for (i = 0; i < TILESIZE; ++i) {
						pixels[(ox - (TILESIZE - 1)) + (oy + i) * XSIZE] = 0x636363;
					}
				} else if (x < CHUNKSIZE) {
					for (i = 0; i < TILESIZE; ++i) {
						pixels[(ox + (TILESIZE - 1)) + (oy + i) * XSIZE] = 0x636363;
					}
				}
			} else if (c->v_boundaries[0][pos] != 0) {
				/* horizontal */
				for (i = 0; i < TILESIZE; ++i) {
					pixels[(ox + i) + oy * XSIZE] = 0x636363;
				}
			} else if (c->d_boundaries[0][pos] != 0) {
				if (c->d_boundaries[0][pos] < 12000) {
					/*pixels[(ox + 0) + (oy + 1) * XSIZE] = 0x636363;
					pixels[(ox + 1) + (oy + 0) * XSIZE] = 0x636363;*/
					for (i = (TILESIZE - 1); i != UINT_MAX; --i) {
						pixels[(ox + (TILESIZE - 1) - i) + (oy + i) * XSIZE] = 0x636363 + i;
					}
				} else if (c->d_boundaries[0][pos] < 24000) {
					for (i = (TILESIZE - 1); i != UINT_MAX; --i) {
						pixels[(ox + (TILESIZE - 1) - i) + (oy + i) * XSIZE] = 0x636363 + i;
					}
				}
			}
		}
	}
	return 0;
}

int
create_image(const struct world *w, uint32_t *pixels)
{
	unsigned x, y;
	unsigned i, num;
	struct model_spawn m;
	uint32_t colour;

	for (x = 0; x < MAPSIZE; ++x) {
		for (y = 0; y < MAPSIZE; ++y) {
			if (write_chunk(pixels, x, y, &w->chunks[x][y]) != 0) {
				return 1;
			}
		}
	}
	num = sizeof(model_spawns) / sizeof(struct model_spawn);
	for (i = 0; i < num; ++i) {
		m = model_spawns[i];
		if (m.y > 944) continue;
		switch (m.id) {
		case 0:
		case 1:
			colour = 0x00a500;
			break;
#if 0
		case 100: /* mining */
		case 101: /* mining */
		case 102: /* mining */
		case 103: /* mining */
		case 104: /* mining */
		case 105: /* mining */
		case 106: /* mining */
		case 107: /* mining */
		case 108: /* mining */
		case 109: /* mining */
		case 110: /* mining */
		case 111: /* mining */
		case 112: /* mining */
		case 113: /* mining */
		case 195: /* mining */
		case 196: /* mining */
		case 210: /* mining */
		case 588: /* mining */
		case 45: /* railing */
		case 3: /* table */
		case 7: /* chair */
		case 11: /* range */
		case 72: /* wheat */
		case 118: /* furnace */
		case 70: /* dead */
		case 88: /* dead */
		case 209: /* dead */
			colour = 0xb56300;
			break;
#endif
		default:
			colour = 0xb56300;
			break;
		}
		x = XSIZE - ((m.x - 48) * TILESIZE);
		y = (YSIZE - (YSIZE - (m.y - 96))) * TILESIZE;
		if (x == 0 || y == 0 || x >= XSIZE || y >= YSIZE) {
			continue;
		}
		pixels[(x - 1) + y * XSIZE] = colour;
		pixels[(x + 1) + y * XSIZE] = colour;
		pixels[x + (y - 1) * XSIZE] = colour;
		pixels[x + (y + 1) * XSIZE] = colour;
		pixels[x + y * XSIZE] = colour;
		/*pixels[x + y * XSIZE] = 0x00FFFF;*/
	}
	return 0;
}

int write_tga(const char *outname, const uint32_t *pixels)
{
	unsigned i;
	FILE *f;

	if ((f = fopen(outname, "w")) == NULL) {
		return 1;
	}
	fputc(0, f); /* id len */
	fputc(0, f); /* maptype */
	fputc(2, f); /* uncompressed trucolour */
	fputc(0, f); /* map entry index */
	fputc(0, f); /* map entry index */
	fputc(0, f); /* map len */
	fputc(0, f); /* map len */
	fputc(0, f); /* map bpp */
	fputc(0, f); /* x-origin */
	fputc(0, f); /* x-origin */
	fputc(0, f); /* y-origin */
	fputc(0, f); /* y-origin */
	fputc((uint8_t)(XSIZE >> 0), f); /* width */
	fputc((uint8_t)(XSIZE >> 8), f); /* width */
	fputc((uint8_t)(YSIZE >> 0), f); /* height */
	fputc((uint8_t)(YSIZE >> 8), f); /* height */
	fputc(24, f); /* bpp */
	fputc(0x20, f); /* desc */
	for (i = 0; i < IMGSIZE; ++i) {
		fputc((uint8_t)(pixels[i] >> 0), f);
		fputc((uint8_t)(pixels[i] >> 8), f);
		fputc((uint8_t)(pixels[i] >> 16), f);
	}
	fclose(f);
	return 0;
}

int
main(int argc, char **argv)
{
	struct world *w;
	unsigned x, y;
	unsigned i;
	uint32_t *pixels;

	for (i = 0; i < 64; ++i) {
		colour_tab[i] = ((255 - i * 4) << 16) |
			((255 - (int)(i * 1.75)) << 8) |
			((255 - i * 4) & 0xFF) << 0;
	}
	for (i = 0; i < 64; ++i) {
		colour_tab[i + 64] = ((i * 3) << 16) |
			(144 << 8) |
			(0 << 0);
	}
	for (i = 0; i < 64; ++i) {
		colour_tab[i + 128] = ((192 - (int)(i * 1.5)) << 16) |
			((144 - (unsigned)(i * 1.5)) << 8) |
			(0 << 0);
	}
	for (i = 0; i < 64; ++i) {
		colour_tab[i + 192] = ((96 - (int)(i * 1.5)) << 16) |
			((48 + (unsigned)(i * 1.5)) << 8) |
			(0 << 0);
	}
	for (i = 0; i < 256; ++i) {
		colour_tab[i] = colour_tab[i] >> 1 & 0x7f7f7f;
	}

	if ((w = calloc(sizeof(struct world), 1)) == NULL) {
		return 1;
	}
	for (x = 0; x < MAPSIZE; ++x) {
		for (y = 0; y < MAPSIZE; ++y) {
			if (load_chunk(&w->chunks[x][y],
				MAPSTARTX + x, MAPSTARTY + y, 0) != 0) {
					perror("load_chunk");
			}
		}
	}
	if ((pixels = calloc(IMGSIZE, sizeof(uint32_t))) == NULL) {
		perror("calloc");
		return 1;
	}
	if (create_image(w, pixels) != 0) {
		perror("create_image");
		free(pixels);
		return 1;
	}
	if (write_tga("./test.tga", pixels) != 0) {
		perror("write_tga");
		free(pixels);
		return 1;
	}
	free(pixels);
	return 0;
}
