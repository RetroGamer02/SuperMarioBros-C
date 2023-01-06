#include <cstdio>
#include <iostream>

#include <SDL/SDL.h>

#include "Emulation/Controller.hpp"
#include "SMB/SMBEngine.hpp"
#include "Util/Video.hpp"

#include "Configuration.hpp"
#include "Constants.hpp"

#include "3ds.h"
#include <dirent.h>

uint8_t* romImage;
static SDL_Surface* texture;
static SDL_Surface* scanlineTexture;
static SMBEngine* smbEngine = nullptr;
static uint32_t renderBuffer[RENDER_WIDTH * RENDER_HEIGHT];

Thread drawThread;
bool running = true;
Uint8* pixel;

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

    texture = SDL_SetVideoMode(256, 240, 24, SDL_HWSURFACE);
    if ( texture == NULL ) {
        std::cout << "Couldn't set 256x240x24 video mode: %s\n" << SDL_GetError();
    }

    SDL_ShowCursor(SDL_DISABLE);

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
        desiredSpec.freq = Configuration::getAudioFrequency();
        desiredSpec.format = AUDIO_S8;
        desiredSpec.channels = 1;
        desiredSpec.samples = 2048;
        desiredSpec.callback = audioCallback;
        desiredSpec.userdata = NULL;

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

    SDL_Quit();
}

void drawThreadFunc(void(*))
{
    int i = 0;
    for (int y = 0; y < RENDER_HEIGHT; y++)
    {
        for (int x = 0; x < RENDER_WIDTH; x++)
        {
            pixel = (Uint8*)texture->pixels + y * texture->pitch + x * 3;
            *(Uint32*)pixel = renderBuffer[i];
            i++;
        }
    }
}

static void mainLoop()
{
    static SMBEngine engine(romImage); //Static to fit into stack.
    smbEngine = &engine;
    engine.reset();

    
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

    int now = 0;
    int delay = 0;

    while (running)
    {
        drawThread = threadCreate(drawThreadFunc, 0x0, 1024, 0x18, 1, true);
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
            
        }
        if (kUp & KEY_X)
        {
            
        }
        if (kDown & KEY_Y)
        {
           
        }
        if (kUp & KEY_Y)
        {
            
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

        engine.update();
        engine.render(renderBuffer);
        
        //SDL_Flip(texture);
        
        /**
         * Ensure that the framerate stays as close to the desired FPS as possible. If the frame was rendered faster, then delay. 
         * If the frame was slower, reset time so that the game doesn't try to "catch up", going super-speed.
         */
        now = SDL_GetTicks();
        //int delay = progStartTime + int(double(frame) * double(MS_PER_SEC) / double(Configuration::getFrameRate())) - now;
        delay = progStartTime + ((frame) * (MS_PER_SEC) / (Configuration::getFrameRate())) - now;
        if(delay > 0) 
        {
            SDL_Delay(delay);
        }
        else 
        {
            frame = 0;
            progStartTime = now;
        }
        SDL_Flip(texture);
        frame++;
    }
}

int main(int argc, char** argv)
{
    APT_SetAppCpuTimeLimit(70); // enables syscore usage

    gfxInitDefault();
    
	consoleInit(GFX_BOTTOM, NULL);

    DIR* dir = opendir("3ds/SMB");
    if (dir) {
        closedir(dir);
    } else if (ENOENT == errno) {
        mkdir("3ds/SMB", 0700);
    } else {
        printf("SMB directory unknown error.\n");
    }

    if (!initialize())
    {
        std::cout << "Failed to initialize. Please check previous error messages for more information. The program will now exit.\n";
        return -1;
    }

    mainLoop();

    shutdown();

    return 0;
}
