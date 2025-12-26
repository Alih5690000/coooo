#include <SDL2/SDL.h>

SDL_Window* window;
SDL_Renderer* renderer;

void loop(){
    
}

int main(){
    SDL_Init(SDL_INIT_EVERYTHING);
    window=SDL_CreateWindow("LOL",0,0,1000,800,SDL_WINDOW_SHOWN);
    renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);
    return 0;
}