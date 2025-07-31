#include "templehero.h"
#include <stdint.h>
#include <math.h>

void GameUpdateAndRender(game_offscreen_buffer* buffer, int xOffset, int yOffset)
{
	RenderWeirdGradient(buffer, xOffset, yOffset);
}


void RenderWeirdGradient(game_offscreen_buffer* buffer, int xOffset, int yOffset)
{
	uint8_t* row = (uint8_t*)buffer->memory;

	for (int y = 0; y < buffer->height; ++y)
	{
		uint32_t* pixel = (uint32_t*)row;

		for (int x = 0; x < buffer->width; ++x)
		{
			/*
							  BB GG RR XX
			 Pixel in memory: 00 00 00 00
			*/

			uint8_t blue = (x + xOffset);
			uint8_t green = (y + yOffset);

			*pixel++ = ((green << 8) | blue);

		}

		row += buffer->pitch;
	}
}
