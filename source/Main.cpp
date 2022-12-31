#include <cstdio>
#include <iostream>

#include <SDL/SDL.h>

#include "Emulation/Controller.hpp"
#include "SMB/SMBEngine.hpp"
#include "Util/Video.hpp"

#include "Configuration.hpp"
#include "Constants.hpp"

#include "3ds.h"

uint8_t* romImage;
//static SDL_Window* window;
//static SDL_Renderer* renderer;
static SDL_Surface* texture;
static SDL_Surface* scanlineTexture;
static SMBEngine* smbEngine = nullptr;
static uint32_t renderBuffer[RENDER_WIDTH * RENDER_HEIGHT];

/**
 * Load the Super Mario Bros. ROM image.
 */
static bool loadRomImage()
{
    FILE* file = fopen(Configuration::getRomFileName().c_str(), "r");
    if (file == NULL)
    {
        std::cout << "Failed to open the file \"" << Configuration::getRomFileName() << "\". Exiting.\n";
        return false;
    }

    // Find the size of the file
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0L, SEEK_SET);

    // Read the entire file into a buffer
    romImage = new uint8_t[fileSize];
    fread(romImage, sizeof(uint8_t), fileSize, file);
    fclose(file);

    return true;
}

/**
 * SDL Audio callback function.
 */
static void audioCallback(void* userdata, uint8_t* buffer, int len)
{
    if (smbEngine != nullptr)
    {
        smbEngine->audioCallback(buffer, len);
    }
}

/**
 * Initialize libraries for use.
 */
static bool initialize()
{
    // Load the configuration
    //
    Configuration::initialize(CONFIG_FILE_NAME);

    // Load the SMB ROM image
    if (!loadRomImage())
    {
        return false;
    }

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        std::cout << "SDL_Init() failed during initialize(): " << SDL_GetError() << std::endl;
        return false;
    }

    // Create the window
    /*window = SDL_CreateWindow(APP_TITLE,
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              RENDER_WIDTH * Configuration::getRenderScale(),
                              RENDER_HEIGHT * Configuration::getRenderScale(),
                              0);
    if (window == nullptr)
    {
        std::cout << "SDL_CreateWindow() failed during initialize(): " << SDL_GetError() << std::endl;
        return false;
    }*/

    // Setup the renderer and texture buffer
    /*renderer = SDL_CreateRenderer(window, -1, (Configuration::getVsyncEnabled() ? SDL_RENDERER_PRESENTVSYNC : 0) | SDL_RENDERER_ACCELERATED);
    if (screen == nullptr)
    {
        std::cout << "SDL_CreateRenderer() failed during initialize(): " << SDL_GetError() << std::endl;
        return false;
    }*/

    texture = SDL_SetVideoMode(256, 240, 24, SDL_HWSURFACE);
    if ( texture == NULL ) {
        std::cout << "Couldn't set 256x240x24 video mode: %s\n" << SDL_GetError();
        //exit(1);
    }

    /*if (SDL_RenderSetLogicalSize(renderer, RENDER_WIDTH, RENDER_HEIGHT) < 0)
    {
        std::cout << "SDL_RenderSetLogicalSize() failed during initialize(): " << SDL_GetError() << std::endl;
        return false;
    }*/

    /*texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, RENDER_WIDTH, RENDER_HEIGHT);
    if (texture == nullptr)
    {
        std::cout << "SDL_CreateTexture() failed during initialize(): " << SDL_GetError() << std::endl;
        return false;
    }*/

    /*if (Configuration::getScanlinesEnabled())
    {
        scanlineTexture = generateScanlineTexture(renderer);
    }*/

    // Set up custom palette, if configured
    //
    if (!Configuration::getPaletteFileName().empty())
    {
        const uint32_t* palette = loadPalette(Configuration::getPaletteFileName());
        if (palette)
        {
            paletteRGB = palette;
        }
    }

    if (Configuration::getAudioEnabled())
    {
        // Initialize audio
        SDL_AudioSpec desiredSpec;
        desiredSpec.freq = 22050; //Configuration::getAudioFrequency();
        desiredSpec.format = AUDIO_S8;
        desiredSpec.channels = 1;
        desiredSpec.samples = 2048;
        desiredSpec.callback = audioCallback;
        desiredSpec.userdata = NULL;

        SDL_AudioSpec obtainedSpec;
        SDL_OpenAudio(&desiredSpec, NULL);

        // Start playing audio
        SDL_PauseAudio(0);
    }

    return true;
}

/**
 * Shutdown libraries for exit.
 */
static void shutdown()
{
    SDL_CloseAudio();

    SDL_FreeSurface(scanlineTexture);
    SDL_FreeSurface(texture);
    //SDL_DestroyRenderer(renderer);
    //SDL_DestroyWindow(window);

    SDL_Quit();
}

void putpixel(SDL_Surface* surface, int x, int y, Uint32 pixel)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16*)p = pixel;
        break;

    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        }
        else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32*)p = pixel;
        break;
    }
}

static void mainLoop()
{
    static SMBEngine engine(romImage); //Static to fit into stack.
    smbEngine = &engine;
    engine.reset();

    bool running = true;
    int progStartTime = SDL_GetTicks();
    int frame = 0;

    //Matrix containing the name of each key. Useful for printing when a key is pressed
	char keysNames[32][32] = {
		"KEY_A", "KEY_B", "KEY_SELECT", "KEY_START",
		"KEY_DRIGHT", "KEY_DLEFT", "KEY_DUP", "KEY_DDOWN",
		"KEY_R", "KEY_L", "KEY_X", "KEY_Y",
		"", "", "KEY_ZL", "KEY_ZR",
		"", "", "", "",
		"KEY_TOUCH", "", "", "",
		"KEY_CSTICK_RIGHT", "KEY_CSTICK_LEFT", "KEY_CSTICK_UP", "KEY_CSTICK_DOWN",
		"KEY_CPAD_RIGHT", "KEY_CPAD_LEFT", "KEY_CPAD_UP", "KEY_CPAD_DOWN"
	};

    u32 kDownOld = 0, kHeldOld = 0, kUpOld = 0; //In these variables there will be information about keys detected in the previous frame
    
		

    Controller& controller1 = engine.getController1();

    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))

        //Scan all the inputs. This should be done once for each frame
		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown();
		//hidKeysHeld returns information about which buttons have are held down in this frame
		u32 kHeld = hidKeysHeld();
		//hidKeysUp returns information about which buttons have been just released
		u32 kUp = hidKeysUp();

        {
            if (kDown & KEY_START)
        {
            controller1.setButtonState(BUTTON_START, true);
        }
        if (kUp & KEY_START)
        {
            controller1.setButtonState(BUTTON_START, false);
        }
        if (kDown & KEY_SELECT)
        {
            controller1.setButtonState(BUTTON_SELECT, true);
        }
        if (kUp & KEY_SELECT)
        {
            controller1.setButtonState(BUTTON_SELECT, false);
        }
        if (kDown & KEY_A)
        {
            controller1.setButtonState(BUTTON_A, true);
        }
        if (kUp & KEY_A)
        {
            controller1.setButtonState(BUTTON_A, false);
        }
        if (kDown & KEY_B)
        {
            controller1.setButtonState(BUTTON_B, true);
        }
        if (kUp & KEY_B)
        {
            controller1.setButtonState(BUTTON_B, false);
        }
        /*if (kDown & KEY_X)
        {
            XButton = 1;
        }
        if (kUp & KEY_X)
        {
            XButton = 0;
        }
        if (kDown & KEY_Y)
        {
            YButton = 1;
        }
        if (kUp & KEY_Y)
        {
            YButton = 0;
        }*/
        if (kDown & KEY_DLEFT)
        {
            controller1.setButtonState(BUTTON_LEFT, true);
        }
        if (kUp & KEY_DLEFT)
        {
            controller1.setButtonState(BUTTON_LEFT, false);
        }
        if (kDown & KEY_DRIGHT)
        {
            controller1.setButtonState(BUTTON_RIGHT, true);
        }
        if (kUp & KEY_DRIGHT)
        {
            controller1.setButtonState(BUTTON_RIGHT, false);
        }
        if (kDown & KEY_DUP)
        {
            controller1.setButtonState(BUTTON_UP, true);
        }
        if (kUp & KEY_DUP)
        {
            controller1.setButtonState(BUTTON_UP, false);
        }
        if (kDown & KEY_DDOWN)
        {
           controller1.setButtonState(BUTTON_DOWN, true);
        }
        if (kUp & KEY_DDOWN)
        {
            controller1.setButtonState(BUTTON_DOWN, false);
        }
        /*if (kDown & KEY_L)
        {
            LeftShoulder = 1;
        }
        if (kUp & KEY_L)
        {
            LeftShoulder = 0;
        }
        if (kDown & KEY_R)
        {
            RightShoulder = 1;
        }
        if (kUp & KEY_R)
        {
            RightShoulder = 0;
        }*/

        //circlePosition pos;

		//Read the CirclePad position
		/*hidCircleRead(&pos);

        if (pos.dx >= 15 || pos.dx <= -15)
        {
            StickX = pos.dx / 30;
        } else {
            StickX = 0;
        }
        if (pos.dy >= 15 || pos.dy <= -15)
        {
            StickY = pos.dy / 30 *-1;
        } else {
            StickY = 0;
        }*/

		//Set keys old values for the next frame
		kDownOld = kDown;
		kHeldOld = kHeld;
		kUpOld = kUp;

            switch (event.type)
            {
            case SDL_QUIT:
                running = false;
                break;
            default:
                break;
            }
        }

        

        

        //Fixme
/*
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        Controller& controller1 = engine.getController1();
        controller1.setButtonState(BUTTON_A, keys[SDL_SCANCODE_X]);
        controller1.setButtonState(BUTTON_B, keys[SDL_SCANCODE_Z]);
        controller1.setButtonState(BUTTON_SELECT, keys[SDL_SCANCODE_BACKSPACE]);
        controller1.setButtonState(BUTTON_START, keys[SDL_SCANCODE_RETURN]);
        controller1.setButtonState(BUTTON_UP, keys[SDL_SCANCODE_UP]);
        controller1.setButtonState(BUTTON_DOWN, keys[SDL_SCANCODE_DOWN]);
        controller1.setButtonState(BUTTON_LEFT, keys[SDL_SCANCODE_LEFT]);
        controller1.setButtonState(BUTTON_RIGHT, keys[SDL_SCANCODE_RIGHT]);
*/
        /*if (keys[SDL_SCANCODE_R])
        {
            // Reset
            engine.reset();
        }
        if (keys[SDL_SCANCODE_ESCAPE])
        {
            // quit
            running = false;
            break;
        }*/

        engine.update();
        engine.render(renderBuffer);

        SDL_LockSurface(texture);

        int i = 0;
        for (int y = 0; y < RENDER_HEIGHT; y++)
        {
            for (int x = 0; x < RENDER_WIDTH; x++)
            {
                    putpixel(texture, x, y, renderBuffer[i]);
                    i++;
            }
        }

        SDL_UnlockSurface(texture);

        //SDL_UpdateTexture(texture, NULL, renderBuffer, sizeof(uint32_t) * RENDER_WIDTH);
        SDL_Flip(texture);

        //SDL_RenderClear(renderer);

        // Render the screen
        //SDL_RenderSetLogicalSize(renderer, RENDER_WIDTH, RENDER_HEIGHT);
        //SDL_RenderCopy(renderer, texture, NULL, NULL);

        // Render scanlines
        //
        /*if (Configuration::getScanlinesEnabled())
        {
            SDL_RenderSetLogicalSize(renderer, RENDER_WIDTH * 3, RENDER_HEIGHT * 3);
            SDL_RenderCopy(renderer, scanlineTexture, NULL, NULL);
        }

        SDL_RenderPresent(renderer);*/

        /**
         * Ensure that the framerate stays as close to the desired FPS as possible. If the frame was rendered faster, then delay. 
         * If the frame was slower, reset time so that the game doesn't try to "catch up", going super-speed.
         */
        int now = SDL_GetTicks();
        int delay = progStartTime + int(double(frame) * double(MS_PER_SEC) / double(Configuration::getFrameRate())) - now;
        if(delay > 0) 
        {
            SDL_Delay(delay);
        }
        else 
        {
            frame = 0;
            progStartTime = now;
        }
        frame++;
    }
}

int main(int argc, char** argv)
{
    gfxInitDefault();
    
	consoleInit(GFX_BOTTOM, NULL);

    if (!initialize())
    {
        std::cout << "Failed to initialize. Please check previous error messages for more information. The program will now exit.\n";
        return -1;
    }

    mainLoop();

    shutdown();

    return 0;
}
