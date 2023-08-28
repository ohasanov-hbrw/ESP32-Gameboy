#include <Ui.h>
#include <Ppu.h>
#include <GameBoyEmulator.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture;
SDL_Surface *screen;

SDL_Window *sdlDebugWindow;
SDL_Renderer *sdlDebugRenderer;
SDL_Texture *sdlDebugTexture;
SDL_Surface *debugScreen;

TTF_Font* CFont;

SDL_Color TextColor = { 255, 0, 0, 255}; // Red SDL color.
SDL_Surface* TextSurface; // The surface necessary to create the font texture.
SDL_Texture* TextTexture; // The font texture prepared for render.
SDL_Rect TextRect; // Text rectangle area with the position for the texture text.

SDL_Color White = {255, 255, 255};

static int scale = 2;

void initUi(){
    SDL_Init(SDL_INIT_VIDEO);
    printf("SDL INIT\n");
    TTF_Init();
    printf("TTF INIT\n");

    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &sdlWindow, &sdlRenderer);

    screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
                                            0x00FF0000,
                                            0x0000FF00,
                                            0x000000FF,
                                            0xFF000000);
    sdlTexture = SDL_CreateTexture(sdlRenderer,
                                                SDL_PIXELFORMAT_ARGB8888,
                                                SDL_TEXTUREACCESS_STREAMING,
                                                SCREEN_WIDTH, SCREEN_HEIGHT);


    SDL_CreateWindowAndRenderer(16 * 8 * scale, 32 * 8 * scale, 0, 
        &sdlDebugWindow, &sdlDebugRenderer);

    debugScreen = SDL_CreateRGBSurface(0, (16 * 8 * scale) + (16 * scale), 
                                            (32 * 8 * scale) + (64 * scale), 32,
                                            0x00FF0000,
                                            0x0000FF00,
                                            0x000000FF,
                                            0xFF000000);

    sdlDebugTexture = SDL_CreateTexture(sdlDebugRenderer,
                                            SDL_PIXELFORMAT_ARGB8888,
                                            SDL_TEXTUREACCESS_STREAMING,
                                            (16 * 8 * scale) + (16 * scale), 
                                            (32 * 8 * scale) + (64 * scale));

    int x, y;
    SDL_GetWindowPosition(sdlWindow, &x, &y);
    SDL_SetWindowPosition(sdlDebugWindow, x + SCREEN_WIDTH + 10, y);


    CFont = TTF_OpenFont("font.ttf", 16);
}

void delay(u32 ms){
    SDL_Delay(ms);
}



u32 getTicks(){
    return SDL_GetTicks();
}


static unsigned long tileColors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000}; 

void displayTile(SDL_Surface *surface, u16 startLocation, u16 tileNum, int x, int y){
    SDL_Rect rc;
    for(int tileY=0; tileY<16; tileY += 2){
        u8 b1 = readBus(startLocation + (tileNum * 16) + tileY);
        u8 b2 = readBus(startLocation + (tileNum * 16) + tileY + 1);
        for(int bit = 7; bit >= 0; bit--){
            u8 hi = !!(b1 & (1 << bit)) << 1;
            u8 lo = !!(b2 & (1 << bit));
            u8 color = hi | lo;
            rc.x = x + ((7 - bit) * scale);
            rc.y = y + (tileY / 2 * scale);
            rc.w = scale;
            rc.h = scale;
            SDL_FillRect(surface, &rc, tileColors[color]);
        }
    }
}

void updateDebugWindow(){
    int xDraw = 0;
    int yDraw = 0;
    int tileNum = 0;

    SDL_Rect rc;
    rc.x = 0;
    rc.y = 0;
    rc.w = debugScreen->w;
    rc.h = debugScreen->h;
    SDL_FillRect(debugScreen, &rc, 0xFF101010);

    u16 addr = 0x8000;
    for(int y = 0; y < 24; y++){
        for(int x = 0; x < 16; x++){
            displayTile(debugScreen, addr, tileNum, xDraw + (x * scale), yDraw + (y * scale));
            xDraw += (8 * scale);
            tileNum++;
        }
        yDraw += (8 * scale);
        xDraw = 0;
    }

	SDL_UpdateTexture(sdlDebugTexture, NULL, debugScreen->pixels, debugScreen->pitch);
	SDL_RenderClear(sdlDebugRenderer);
	SDL_RenderCopy(sdlDebugRenderer, sdlDebugTexture, NULL, NULL);
	SDL_RenderPresent(sdlDebugRenderer);
}

void updateUi(){
    SDL_Rect rc;
    rc.x = rc.y = 0;
    rc.w = rc.h = 2048;

    u32 *video_buffer = getPpuContext()->vBuffer;

    for (int line_num = 0; line_num < YRES; line_num++) {
        for (int x = 0; x < XRES; x++) {
            rc.x = x * scale;
            rc.y = line_num * scale;
            rc.w = scale;
            rc.h = scale;

            SDL_FillRect(screen, &rc, video_buffer[x + (line_num * XRES)]);
        }
    }

    SDL_UpdateTexture(sdlTexture, NULL, screen->pixels, screen->pitch);
    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
    SDL_RenderPresent(sdlRenderer);
    updateDebugWindow();
}

void CreateText(const char* Message) {
    TextSurface = TTF_RenderText_Solid(CFont, Message, TextColor);
    TextTexture = SDL_CreateTextureFromSurface(sdlRenderer, TextSurface);
    TextRect.x = TextSurface->w * 0.5; // Center horizontaly
    TextRect.y = TextSurface->h * 0.5; // Center verticaly
    TextRect.w = TextSurface->w;
    TextRect.h = TextSurface->h;
    // After you create the texture you can release the surface memory allocation because we actually render the texture not the surface.
    SDL_FreeSurface(TextSurface);
}


void RenderText() {
    SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255); // Make window bg black.
    SDL_RenderClear(sdlRenderer); // Paint screen black.
    SDL_RenderCopy(sdlRenderer, TextTexture, NULL, &TextRect); // Add text to render queue.
    SDL_RenderPresent(sdlRenderer); // Render everything that's on the queue.
}

void handleEventsUi(){
    SDL_Event e;
    SDL_UpdateWindowSurface(sdlWindow);
    while (SDL_PollEvent(&e) > 0)
    {
        //TODO SDL_UpdateWindowSurface(sdlWindow);
        //TODO SDL_UpdateWindowSurface(sdlTraceWindow);
        //TODO SDL_UpdateWindowSurface(sdlDebugWindow);

        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) {
            GetEmulatorContext()->killEmu = true;
        }
    }
}