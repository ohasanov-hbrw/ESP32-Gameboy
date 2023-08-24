#include <Ui.h>
#include <GameBoyEmulator.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture;
SDL_Surface *screen;

TTF_Font* CFont;

SDL_Color TextColor = { 255, 0, 0, 255}; // Red SDL color.
SDL_Surface* TextSurface; // The surface necessary to create the font texture.
SDL_Texture* TextTexture; // The font texture prepared for render.
SDL_Rect TextRect; // Text rectangle area with the position for the texture text.

SDL_Color White = {255, 255, 255};

void initUi(){
    SDL_Init(SDL_INIT_VIDEO);
    printf("SDL INIT\n");
    TTF_Init();
    printf("TTF INIT\n");

    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &sdlWindow, &sdlRenderer);

    CFont = TTF_OpenFont("font.ttf", 16);
}

void delay(u32 ms){
    SDL_Delay(ms);
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
    RenderText();
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