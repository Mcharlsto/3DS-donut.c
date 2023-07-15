/*
Nintendo 3DS donut.c port
Original code by a1k0n [https://www.a1k0n.net/2006/09/15/obfuscated-c-donut.html]
Based on the Wii port by Andrew Piroli [https://github.com/andrewpiroli/wii-donut.c]
3dsx donut icon used under CC BY-NC 4.0 [https://pngimg.com/image/32439]
Public domain
*/

#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define BUFFER_WIDTH 40
#define BUFFER_HEIGHT 27
#define BUFFER_SIZE (BUFFER_WIDTH*BUFFER_HEIGHT)

#define IMAGE_SCALE 15
#define TWO_PI 6.28

// donut point traversal increment (inverse of density)
#define DELTA_J 0.07
#define DELTA_I 0.02

// Fwd Decls
static void blit_buffer(char const* const);
static char intensity_char(float);

static volatile bool debug = false;

// rotation increment around each axis
static volatile double DELTA_A = 0.08;
static volatile double DELTA_B = 0.04;

int main(int argc, char **argv)
{
	// Initialize services
	gfxInitDefault();

	//In this example we need one PrintConsole for each screen
	PrintConsole topScreen, bottomScreen;

	osSetSpeedupEnable(true); // boost clock if on n3ds

	//Initialize console for both screen using the two different PrintConsole we have defined
	consoleInit(GFX_TOP, &topScreen);
	consoleInit(GFX_BOTTOM, &bottomScreen);

	//Now write something else on the bottom screen
	consoleSelect(&bottomScreen);
	printf("Nintendo 3DS donut.c\nPress START to exit\n");
	consoleSelect(&topScreen);

	float A = 0;		// rotation around one axis (radians)
	float B = 0;		// rotation around other axis (radians)
	float i;
	float j;
	float z_buffer[BUFFER_SIZE];
	char pixel_buffer[BUFFER_SIZE];

	// clear the screen
	printf("\x1b[2J");

	// Main loop
	while (aptMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput(); 

        gfxFlushBuffers();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown();

		if (kDown & KEY_START) break; // break in order to return to hbmenu

		// reset the pixel and z buffers
		memset(pixel_buffer, 32, BUFFER_SIZE);
		memset(z_buffer, 0, sizeof(float) * BUFFER_SIZE);

		float sin_A = sin(A);
		float cos_A = cos(A);
		float cos_B = cos(B);
		float sin_B = sin(B);

		// for every point on the surface of the donut
		for (j = 0; j < TWO_PI; j += DELTA_J) {
			float cos_j = cos(j);
			float sin_j = sin(j);
			for (i = 0; i < TWO_PI; i += DELTA_I) {
				float sin_i = sin(i);
				float cos_i = cos(i);
				float h = cos_j + 2;
				float D =
					1 / (sin_i * h * sin_A + sin_j * cos_A + 5);
				float t = sin_i * h * cos_A - sin_j * sin_A;

				// project onto buffer surface point (x,y)
				int x =
					(BUFFER_WIDTH / 2) +
					2 * IMAGE_SCALE * D * (cos_i * h * cos_B -
								t * sin_B);
				int y =
					(BUFFER_HEIGHT / 2) +
					IMAGE_SCALE * D * (cos_i * h * sin_B +
								t * cos_B);

				// only consider points that occur on the defined surface
				if (0 < x && x < BUFFER_WIDTH && 0 < y
					&& y < BUFFER_HEIGHT) {

					// calculate position on linear buffer
					int buffer_pos = (BUFFER_WIDTH * y) + x;

					// if this point is closer to the viewer than previous point at this position, overwrite it
					if (D > z_buffer[buffer_pos]) {
						z_buffer[buffer_pos] = D;

						// calculate intensity of this point
						float intensity =
							(cos_B *
								(sin_j * sin_A -
								sin_i * cos_j * cos_A)
								- sin_i * cos_j * sin_A -
								sin_j * cos_A -
								cos_i * cos_j * sin_B);

						// write the intensity to the buffer
						pixel_buffer[buffer_pos] =
							intensity_char(intensity);
					}
				}
			}
		}

        gfxSwapBuffers();

		gspWaitForVBlank();

		blit_buffer(pixel_buffer);
		// We could use a lock around these since they could be updated by the other thread in-between
		// but it's a spinning donut, so who cares if the rotation is a few radians off what the user technically wanted
		A += DELTA_A;
		B += DELTA_B;
	}

	// Exit services
	gfxExit();

	return 0;
}

static void blit_buffer(char const* const pixel_buffer) {
	int buffer_pos;
	printf("\x1b[2J");
	printf("      ");
	if (debug){
		printf("DELTA_A: %f DELTA_B: %f", DELTA_A, DELTA_B);
	}
	
	// for every char in the buffer
	for (buffer_pos = 0; buffer_pos < 1 + BUFFER_SIZE; buffer_pos++) {
		// newline if necessary
		if (buffer_pos % BUFFER_WIDTH == 0) {
			putchar('\n');
			for(int i=0; i < 6; i++) {
				putchar(' ');
			}
		} else {
			putchar(pixel_buffer[buffer_pos]);
		}
	}
}

static char intensity_char(float const intensity) {
	char gradient[] = ".,-~:;=!*#$@";
	int gradient_position = 12 * (intensity / 1.5);
	if (0 < gradient_position && gradient_position < 12) {
		return gradient[gradient_position];
	}
	return gradient[0];
}