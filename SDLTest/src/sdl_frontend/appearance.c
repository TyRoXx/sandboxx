#include "appearance.h"
#include "image_manager.h"
#include "base/algorithm.h"
#include <assert.h>


Bool AnimationSide_init(AnimationSide *a, size_t frame_count)
{
	a->frames = malloc(frame_count * sizeof(AnimationFrame));
	a->frame_count = frame_count;
	return (a->frames != NULL);
}

void AnimationSide_free(AnimationSide *a)
{
	free(a->frames);
}


void Animation_free(Animation *a)
{
	size_t i;
	for (i = 0; i < DIR_COUNT; ++i)
	{
		AnimationSide_free(&a->sides[i]);
	}
}


void AppearanceLayout_free(AppearanceLayout *a)
{
	size_t i;

	for (i = 0; i < (size_t)Anim_COUNT; ++i)
	{
		Animation_free(a->animations + i);
	}
}


void Appearance_init(Appearance *appearance,
					 SDL_Surface *image,
					 AppearanceLayout const *layout)
{
	appearance->image = image;
	appearance->layout = layout;
}

void Appearance_free(Appearance *appearance)
{
	(void)appearance;
}


unsigned const tile_size = 32;

static Bool init_animation(
		Animation *anim,
		AnimationType anim_id,
		Bool (*init_side)(AnimationSide *, AnimationType, Direction))
{
	Bool result = True;
	size_t i;

	for (i = 0; i < DIR_COUNT; ++i)
	{
		AnimationSide * const side = &anim->sides[i];
		if (!init_side(side, anim_id, (Direction)i))
		{
			result = False;
			break;
		}
	}

	if (!result)
	{
		size_t c;
		for (c = i, i = 0; i < c; ++i)
		{
			AnimationSide_free(&anim->sides[i]);
		}
	}

	return result;
}

static Bool init_layout(AppearanceLayout *layout,
						Bool (*init_side)(AnimationSide *, AnimationType, Direction))
{
	Bool result = True;
	size_t i, c;
	Vector2i offset;
	offset.x = 0;
	offset.y = 0;

	for (i = 0, c = (size_t)Anim_COUNT; i < c; ++i)
	{
		Animation * const anim = layout->animations + i;

		if (!init_animation(anim, (AnimationType)i, init_side))
		{
			result = False;
			break;
		}

		offset.y += (ptrdiff_t)(DIR_COUNT * tile_size);
	}

	if (!result)
	{
		for (c = i, i = 0; i < c; ++i)
		{
			Animation_free(layout->animations + i);
		}
		return False;
	}

	return True;
}

static Bool init_dynamic_side_1(AnimationSide *side,
								AnimationType anim_id,
								Direction side_id)
{
	size_t const frame_count = 3;
	size_t j;
	SDL_Rect section;

	if (!AnimationSide_init(side, frame_count))
	{
		return False;
	}

	section.x = 0;
	section.y = (Sint16)(((unsigned)anim_id * DIR_COUNT + (unsigned)side_id) * tile_size);
	section.w = (Uint16)tile_size;
	section.h = (Uint16)tile_size;

	for (j = 0; j < frame_count; ++j)
	{
		AnimationFrame * const frame = side->frames + j;
		frame->duration = 200;
		frame->section = section;

		section.x = (Sint16)((unsigned)section.x + tile_size);
	}

	return True;
}

static Bool init_static_side(AnimationSide *side,
							 AnimationType anim_id,
							 Direction side_id)
{
	SDL_Rect section;

	(void)anim_id;
	(void)side_id;

	if (!AnimationSide_init(side, 1))
	{
		return False;
	}

	section.x = (Sint16)0;
	section.y = (Sint16)0;
	section.w = (Uint16)tile_size;
	section.h = (Uint16)tile_size;

	side->frames->duration = 0;
	side->frames->section = section;
	return True;
}

static Bool init_dynamic_layout_1(AppearanceLayout *layout)
{
	return init_layout(layout, &init_dynamic_side_1);
}

static Bool init_static_layout(AppearanceLayout *layout)
{
	return init_layout(layout, &init_static_side);
}

static Bool add_appearance(AppearanceManager *a,
						   ImageManager *images,
						   char const *image_name,
						   AppearanceLayout const *layout)
{
	SDL_Surface *image;
	Appearance appearance;

	image = ImageManager_get(images, image_name);
	if (!image)
	{
		return False;
	}

	Appearance_init(&appearance, image, layout);
	if (Vector_push_back(&a->appearances, &appearance, sizeof(appearance)))
	{
		return True;
	}
	Appearance_free(&appearance);
	return False;
}

static Bool load_appearances_file(AppearanceManager *a,
								  FILE *file,
								  ImageManager *images)
{
	unsigned expected_index = 0;
	unsigned index;
	char type[32];

	while (fscanf(file, " %u %31s", &index, type) == 2)
	{
		if (index != expected_index)
		{
			fprintf(stderr, "Expected appearance index %u, found %u\n",
					expected_index, index);
			return False;
		}

		if (!strcmp("STATIC", type))
		{
			char image_name[1024];
			if (fscanf(file, " %1023s", image_name) != 1)
			{
				fprintf(stderr, "Static appearance image name expected\n");
				return False;
			}

			if (!add_appearance(a, images, image_name, &a->static_layout))
			{
				return False;
			}
		}
		else if (!strcmp("DYNAMIC_1", type))
		{
			char image_name[1024];
			if (fscanf(file, " %1023s", image_name) != 1)
			{
				fprintf(stderr, "Dynamic appearance 1 image name expected\n");
				return False;
			}

			/*TODO*/
			if (!add_appearance(a, images, image_name, &a->dynamic_layout_1))
			{
				return False;
			}
		}
		else
		{
			fprintf(stderr, "Unknown appearance type\n");
			return False;
		}

		++expected_index;
	}

	return True;
}

static void free_appearance(void *appearance, void *user)
{
	(void)user;
	Appearance_free(appearance);
}

static void free_appearances(Vector *appearances)
{
	for_each(Vector_begin(appearances),
			 Vector_end(appearances),
			 sizeof(Appearance),
			 free_appearance,
			 NULL);
	Vector_free(appearances);
}

Bool AppearanceManager_init(AppearanceManager *a,
							FILE *file,
							ImageManager *images)
{
	if (init_static_layout(&a->static_layout))
	{
		if (init_dynamic_layout_1(&a->dynamic_layout_1))
		{
			Vector_init(&a->appearances);

			if (load_appearances_file(a, file, images))
			{
				return True;
			}

			free_appearances(&a->appearances);
			AppearanceLayout_free(&a->dynamic_layout_1);
		}

		AppearanceLayout_free(&a->static_layout);
	}

	AppearanceManager_free(a);
	return False;
}

void AppearanceManager_free(AppearanceManager *a)
{
	free_appearances(&a->appearances);
	AppearanceLayout_free(&a->dynamic_layout_1);
	AppearanceLayout_free(&a->static_layout);
}

Appearance const *AppearanceManager_get(AppearanceManager const *a,
										AppearanceId id)
{
	size_t const offset = id * sizeof(Appearance);
	if (offset >= Vector_size(&a->appearances))
	{
		return NULL;
	}
	return (Appearance *)(Vector_data(&a->appearances) + offset);
}
