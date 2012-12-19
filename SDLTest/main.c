#include "SDL.h"
#include "SDL_main.h"
#include "tile_grid.h"
#include <math.h>


static void draw_tiles(SDL_Surface *screen,
				size_t x,
				size_t y,
				TileGrid const *tiles,
				SDL_Surface * const *tile_images,
				TileIndex tile_image_count)
{
	size_t ty;
	for (ty = 0; ty < tiles->height; ++ty)
	{
		size_t tx;
		for (tx = 0; tx < tiles->width; ++tx)
		{
			TileIndex const tile_index = TileGrid_get(tiles, tx, ty);
			SDL_Rect dest;
			SDL_Surface *image;

			if (tile_index >= tile_image_count)
			{
				continue;
			}

			image = tile_images[tile_index];

			dest.x = x + (tx * image->w);
			dest.y = y + (ty * image->h);

			SDL_BlitSurface(image, 0, screen, &dest);
		}
	}
}

enum
{
	Width = 640, Height = 480,
	MapWidth = 12, MapHeight = 8
};

static const TileIndex BuiltInMap[MapWidth * MapHeight] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 2, 2, 2, 2, 5, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 4, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 6, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		 fprintf(stderr, "SDL error: %s\n", SDL_GetError());
		 return 1;
	}

	{
		SDL_Surface * const screen = SDL_SetVideoMode(Width, Height, 32, SDL_SWSURFACE);
		SDL_Surface *tile_images[4] = {};
		SDL_Surface **tile;
		TileGrid grid;
		SDL_Event event;
		int is_running = 1;

		if (!screen)
		{
			fprintf(stderr, "Cannot create screen: %s\n", SDL_GetError());
			return 1;
		}

		tile_images[0] = SDL_LoadBMP("sprites/grass_32.bmp");
		tile_images[1] = SDL_LoadBMP("sprites/dirt_32.bmp");
		tile_images[2] = SDL_LoadBMP("sprites/n_grass_s_dirt_32.bmp");
		tile_images[3] = SDL_LoadBMP("sprites/n_dirt_s_grass_32.bmp");
		if (!tile_images[0] ||
				!tile_images[1] ||
				!tile_images[2] ||
				!tile_images[3]
				)
		{
			return 1;
		}

		TileGrid_init(&grid, MapWidth, MapHeight);
		memcpy(grid.tiles, BuiltInMap, sizeof(BuiltInMap));

		while (is_running)
		{
			while (SDL_PollEvent(&event))
			{
				if (event.type == SDL_QUIT)
				{
					is_running = 0;
					break;
				}
			}

			if (screen->format->BytesPerPixel == 4)
			{
				unsigned x, y;
				unsigned const time_ms = SDL_GetTicks();

				for (y = 0; y < Height; ++y)
				{
					for (x = 0; x < Width; ++x)
					{
						uint32_t const pixel = SDL_MapRGB(screen->format,
													(Uint8)((1 + sin((float)time_ms / 2800.0f)) * 128),
													(Uint8)((1 + cos((float)time_ms / 1700.0f)) * 128),
													(Uint8)((float)y / (float)Height * 256.0f));

						memcpy(((char *)screen->pixels) + ((Width * y) + x) * 4,
							   &pixel,
							   4);
					}
				}
			}

			draw_tiles(screen, 64, 64, &grid, tile_images,
					   sizeof(tile_images) / sizeof(tile_images[0]));

			SDL_UpdateRect(screen, 0, 0, Width, Height);

			SDL_Delay(10);
		}

		TileGrid_free(&grid);

		for (tile = tile_images;
			 tile != (tile_images + sizeof(tile_images) / sizeof(tile_images[0]));
			 ++tile)
		{
			SDL_FreeSurface(*tile);
		}
	}

	SDL_Quit();
	return 0;
}
