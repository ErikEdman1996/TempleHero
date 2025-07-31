/*
	TODO: THIS IS NOT A FINAL PLATFORM LAYER, THINGS THAT NEEDS TO BE IMPLEMENTED

	- Saved game location
	- Getting a handle to our own executable file
	- Asset loading path
	- Threading (launch a thread)
	- Raw Input (keyboard, mouse, gamepad)
	- Sleep/timeBeginPeriod
	- ClipCursor() (for multimonitor support)
	- Fullscreen support
	- WM_SETCURSOR (for mouse cursor)
	- QueryCancelAutoplay (to disable autoplay)
	- WM_ACTIVATEAPP (for when we are not active)
	- Blit speed improvements (BitBlt)
	- Hardware acceleration (Direct3D, OpenGL)
	- GetKeyboardLayout (for French, Swedish, international WASD support)

	- Just a partial list!!!
*/

#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <math.h>
#include <dsound.h>
#include "templehero.h"

#define internal static
#define local_persist static
#define global_variable static

const global_variable double PI = 3.14159265358979323846;

struct win32_offscreen_buffer
{
	void* memory;
	BITMAPINFO info;
	int width;
	int height;
	int pitch; // Bytes per row
	int bytesPerPixel; // 4 for 32-bit color (RGBA)
};

struct win32_window_dimension
{
	int width;
	int height;
};

//NOTE: XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}

//Note: XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}

//Note: DirectSoundCreate
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

void Win32LoadXinput(void);

global_variable bool RUNNING;
global_variable win32_offscreen_buffer GLOBAL_BUFFER;

internal void BeforeClosingApp();

internal win32_window_dimension Win32GetWindowDimension(HWND window);

internal void Win32ResizeDIBSection(win32_offscreen_buffer& buffer, int width, int height);

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer buffer, HDC deviceContext, int windowWidth, int windowHeight, int x, int y, int width, int height);

internal void Win32InitDSound(HWND *window, int32_t samplesPerSecond, int32_t bufferSize);

LRESULT Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

//Instead of regular Main(), we use wWinMain for Windows GUI applications
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prevInstance, PWSTR commandLine, int showCode)
{
	LARGE_INTEGER perfCounterFrequencyResult;
	QueryPerformanceFrequency(&perfCounterFrequencyResult);
	int64_t perfCounterFrequency = perfCounterFrequencyResult.QuadPart;

	Win32LoadXinput();

	WNDCLASSA windowClass = {};

	Win32ResizeDIBSection(GLOBAL_BUFFER, 1280, 720);

	game_offscreen_buffer buffer = {};
	buffer.memory = GLOBAL_BUFFER.memory;
	buffer.width = GLOBAL_BUFFER.width;
	buffer.height = GLOBAL_BUFFER.height;
	buffer.pitch = GLOBAL_BUFFER.pitch;
	buffer.bytesPerPixel = GLOBAL_BUFFER.bytesPerPixel;

	windowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	windowClass.lpfnWndProc = Win32MainWindowCallback;
	windowClass.hInstance = instance;
	windowClass.lpszClassName = "TempleHeroWindowClass";

	if (RegisterClassA(&windowClass))
	{
		HWND windowHandle = CreateWindowExA
					(
						0, // No extended styles
						windowClass.lpszClassName, // Class name
						"Temple Hero", // Window title
						WS_OVERLAPPEDWINDOW | WS_VISIBLE, // Window style
						CW_USEDEFAULT, CW_USEDEFAULT, // Position (default)
						CW_USEDEFAULT, CW_USEDEFAULT, // Size (default)
						NULL, // No parent window
						NULL, // No menu
						instance, // Instance handle
						NULL // No additional parameters
					);
		if (windowHandle)
		{

			Win32InitDSound(&windowHandle, 48000, 48000*sizeof(int16_t)*2);

			RUNNING = true;

			int xOffset = 0;
			int yOffset = 0;

			LARGE_INTEGER lastCounter;
			QueryPerformanceCounter(&lastCounter);

			uint64_t lastCycleCount = __rdtsc();

			// Main message loop
			while(RUNNING)
			{
				MSG message;

				while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
				{
					if (message.message == WM_QUIT)
					{
						RUNNING = false;
					}

					TranslateMessage(&message);
					DispatchMessage(&message);
				}

				//TODO: Should we pull this more frequently?
				for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; ++controllerIndex)
				{
					XINPUT_STATE controllerState;
					if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
					{
						// Controller is connected
						// Process controllerState here if needed
						XINPUT_GAMEPAD *gamepad = &controllerState.Gamepad;
						bool up = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool down = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool left = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool right = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool start = (gamepad->wButtons & XINPUT_GAMEPAD_START);
						bool back = (gamepad->wButtons & XINPUT_GAMEPAD_BACK);
						bool leftShoulder = (gamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool rightShoulder = (gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);

						bool aButton = (gamepad->wButtons & XINPUT_GAMEPAD_A);

						bool bButton = (gamepad->wButtons & XINPUT_GAMEPAD_B);
						static bool wasBDown = false;
						bool justPressedB = bButton && !wasBDown;

						bool xButton = (gamepad->wButtons & XINPUT_GAMEPAD_X);
						bool yButton = (gamepad->wButtons & XINPUT_GAMEPAD_Y);

						int16_t stickX = gamepad->sThumbLX;
						int16_t stickY = gamepad->sThumbLY;

						int16_t stickDeadzone = 4000; // tweak as needed

						if (abs(stickX) > stickDeadzone)
						{
						}

						if (aButton)
						{
						
						}
						if (justPressedB)
						{

						}
						if (yButton)
						{
						}

						wasBDown = bButton;
					}
					else
					{
						// Controller is not connected
						// Handle disconnection if needed
					}
				}

				++xOffset;

				GameUpdateAndRender(&buffer, xOffset, yOffset);

				HDC deviceContext = GetDC(windowHandle);
				win32_window_dimension dimension = Win32GetWindowDimension(windowHandle);
				Win32DisplayBufferInWindow(GLOBAL_BUFFER, deviceContext, dimension.width, dimension.height, 0, 0, dimension.width, dimension.height);
				ReleaseDC(windowHandle, deviceContext);

				uint64_t endCycleCount = __rdtsc();

				LARGE_INTEGER endCounter;
				QueryPerformanceCounter(&endCounter);

				uint64_t cyclesElapsed = endCycleCount - lastCycleCount;
				int64_t counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
				int32_t msPerFrame = (int32_t)((1000*counterElapsed) / perfCounterFrequency);
				int32_t FPS = (int32_t)(perfCounterFrequency / counterElapsed);
				uint32_t MegaCyclesPerFrame = (uint32_t)(cyclesElapsed / (1000 * 1000));

				lastCounter = endCounter;
				lastCycleCount = endCycleCount;

				//TODO: REMOVE DEBUGGING OUTPUT BEFORE RELEASE
				wchar_t frameBuffer[256];
				wsprintf(frameBuffer, L"Milliseconds/frame: %dms  / %dFPS / %dMCPF\n", msPerFrame, FPS, MegaCyclesPerFrame);
				OutputDebugStringW(frameBuffer);
			}
		}
		else
		{
			//TODO: Handle window creation failure
		}
	}
	else
	{
		//TODO: Handle registration failure
	}

	BeforeClosingApp();

	return 0;
}

internal void BeforeClosingApp()
{
	OutputDebugStringA("Application is closing.\n");
}

internal win32_window_dimension Win32GetWindowDimension(HWND window)
{
	win32_window_dimension result;
	RECT clientRect;
	GetClientRect(window, &clientRect);
	result.width = clientRect.right - clientRect.left;
	result.height = clientRect.bottom - clientRect.top;
	return result;
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer& buffer, int width, int height)
{
	if (buffer.memory)
	{
		VirtualFree(buffer.memory, 0, MEM_RELEASE);
		buffer.memory = NULL;
	}

	buffer.width = width;
	buffer.height = height;
	buffer.bytesPerPixel = 4; // 32 bits per pixel (4 bytes)

	buffer.info.bmiHeader.biSize = sizeof(buffer.info.bmiHeader);
	buffer.info.bmiHeader.biWidth = buffer.width;
	buffer.info.bmiHeader.biHeight = -buffer.height;
	buffer.info.bmiHeader.biPlanes = 1;
	buffer.info.bmiHeader.biBitCount = 32; // 32 bits per pixel
	buffer.info.bmiHeader.biCompression = BI_RGB; // No compression

	int bitMapMemorySize = (buffer.width * buffer.height) * buffer.bytesPerPixel;
	buffer.memory = VirtualAlloc(NULL, bitMapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	buffer.pitch = buffer.width * buffer.bytesPerPixel;
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer buffer, HDC deviceContext, int windowWidth, int windowHeight, int x, int y, int width, int height)
{
	//TODO: Aspect ratio correction
	StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight, 0, 0, buffer.width, buffer.height, buffer.memory, &buffer.info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	switch (message)
	{
	case WM_SIZE:
	{
	} break;
	case WM_DESTROY:
	{
		RUNNING = false;
		OutputDebugStringA("WM_DESTROY received\n");
	} break;
	case WM_ACTIVATEAPP:
	{
		OutputDebugStringA("WM_ACTIVATEAPP received\n");
	} break;
	case WM_CLOSE:
	{
		RUNNING = false;
		OutputDebugStringA("WM_CLOSE received\n");
	} break;

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		uint32_t VKCode = wParam;
		bool wasDown = ((lParam & (1 << 30)) != 0);
		bool isDown = ((lParam & (1 << 31)) == 0);

		if (wasDown != isDown)
		{
			if (VKCode == 'W')
			{
			}
			else if (VKCode == 'A')
			{

			}
			else if (VKCode == 'S')
			{

			}
			else if (VKCode == 'D')
			{

			}
			else if (VKCode == 'Q')
			{

			}
			else if (VKCode == 'E')
			{

			}
			else if (VKCode == VK_UP)
			{

			}
			else if (VKCode == VK_DOWN)
			{

			}
			else if (VKCode == VK_LEFT)
			{

			}
			else if (VKCode == VK_RIGHT)
			{

			}
			else if (VKCode == VK_SPACE)
			{

			}
			else if (VKCode == VK_ESCAPE)
			{
				OutputDebugStringA("ESCAPE: ");
				if (isDown)
				{
					OutputDebugStringA("IsDown ");
				}
				if (wasDown)
				{
					OutputDebugStringA("WasDown ");
				}
				OutputDebugStringA("\n");
			}
		}

		bool altKeyWasDown = ((lParam & (1 << 29)) != 0);
		if (altKeyWasDown && (VKCode == VK_F4))
		{
			RUNNING = false;
		}

	}break;
	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC deviceContext = BeginPaint(window, &paint);
		int x = paint.rcPaint.left;
		int y = paint.rcPaint.top;
		int height = paint.rcPaint.bottom - paint.rcPaint.top;
		int width = paint.rcPaint.right - paint.rcPaint.left;

		win32_window_dimension dimension = Win32GetWindowDimension(window);
		Win32DisplayBufferInWindow(GLOBAL_BUFFER, deviceContext, dimension.width, dimension.height, x, y, width, height);
		EndPaint(window, &paint);
	}break;
	default:
	{
		result = DefWindowProcA(window, message, wParam, lParam);
	}break;
	}

	return result;
}

void Win32LoadXinput(void)
{
	// Try all known versions in order
	HMODULE XInputLibrary = LoadLibraryW(L"xinput1_4.dll");
	if (!XInputLibrary) XInputLibrary = LoadLibraryW(L"xinput1_3.dll");
	if (!XInputLibrary) XInputLibrary = LoadLibraryW(L"xinput9_1_0.dll");

	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
	}

	if (!XInputGetState) { XInputGetState = XInputGetStateStub; }
	if (!XInputSetState) { XInputSetState = XInputSetStateStub; }
}

internal void Win32InitDSound(HWND *window, int32_t samplesPerSecond, int32_t bufferSize)
{
	// Load the library
	HMODULE dsoundLibrary = LoadLibraryW(L"dsound.dll");

	if (!dsoundLibrary)
	{
		OutputDebugStringA("Failed to load dsound.dll\n");
		return;
	}

	direct_sound_create* directSoundCreate = (direct_sound_create*)GetProcAddress(dsoundLibrary, "DirectSoundCreate8");

	if (!directSoundCreate)
	{
		OutputDebugStringA("Failed to get DirectSoundCreate function address.\n");
		return;
	}

	LPDIRECTSOUND directSound;

	HRESULT hr = directSoundCreate(0, &directSound, 0);

	if (FAILED(hr))
	{
		OutputDebugStringA("Failed to create DirectSound object\n");
		return;
	}

	// Set cooperative level to allow format changes
	if (FAILED(directSound->SetCooperativeLevel(*window, DSSCL_PRIORITY)))
	{
		OutputDebugStringA("Failed to set cooperative level\n");
		return;
	}

	//Set Wave Format
	WAVEFORMATEX waveFormat = {};
	waveFormat.wFormatTag = WAVE_FORMAT_PCM; // PCM format
	waveFormat.nChannels = 2; // Stereo
	waveFormat.nSamplesPerSec = samplesPerSecond; // Sample rate
	waveFormat.wBitsPerSample = 16;
	waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0; // No extra data

	// Describe the buffer
	DSBUFFERDESC bufferDesc = {};
	bufferDesc.dwSize = sizeof(DSBUFFERDESC);
	bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
	bufferDesc.dwBufferBytes = 0;
	bufferDesc.lpwfxFormat = NULL;

	//*Create* a primary sound buffer
	LPDIRECTSOUNDBUFFER primaryBuffer;

	hr = directSound->CreateSoundBuffer(&bufferDesc, &primaryBuffer, NULL);

	if (FAILED(hr))
	{
		OutputDebugStringA("Failed to create primary sound buffer.\n");
		return;
	}

	hr = primaryBuffer->SetFormat(&waveFormat);

	if(FAILED(hr))
	{
		OutputDebugStringA("Failed to set format on primary sound buffer.\n");
		return;
	}

	// Create the secondary buffer
	DSBUFFERDESC secondaryBufferDesc = {};
	secondaryBufferDesc.dwSize = sizeof(DSBUFFERDESC);
	secondaryBufferDesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2; // Better position accuracy
	secondaryBufferDesc.dwBufferBytes = bufferSize;
	secondaryBufferDesc.lpwfxFormat = &waveFormat;

	LPDIRECTSOUNDBUFFER secondaryBuffer;

	hr = directSound->CreateSoundBuffer(&secondaryBufferDesc, &secondaryBuffer, NULL);

	if (FAILED(hr))
	{
		OutputDebugStringA("Failed to create secondary sound buffer.\n");
		return;
	}

	// Optionally zero out the buffer to avoid noise at startup
	VOID* region1;
	DWORD region1Size;
	VOID* region2;
	DWORD region2Size;

	if (SUCCEEDED(secondaryBuffer->Lock(0, bufferSize, &region1, &region1Size, &region2, &region2Size, 0)))
	{
		ZeroMemory(region1, region1Size);
		ZeroMemory(region2, region2Size);
		secondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
	}
}
