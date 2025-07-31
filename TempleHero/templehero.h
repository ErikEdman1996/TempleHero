#pragma once
#if !defined(TEMPLEHERO_H)
#include <stdint.h>

struct game_offscreen_buffer
{
	void* memory;
	int width;
	int height;
	int pitch;
	int bytesPerPixel;
};


//Services that the game provides to the platform layer
//NEEDS 4 THINGS: Input, Sound, Bitmaps, and Timing
void GameUpdateAndRender(game_offscreen_buffer* buffer, int xOffset, int yOffset);
void RenderWeirdGradient(game_offscreen_buffer *buffer, int xOffset, int yOffset);

#define TEMPLEHERO_H
#endif