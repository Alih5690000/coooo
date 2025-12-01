#include <SDL2/SDL.h>
#include <emscripten.h>

bool dead=false;
SDL_Rect ima_fckin_killer{400,400,50,50};
float dmg_cd=0.f;
int lives=5;
SDL_Rect player{100,100,50,50};
int plr_speed=300;
int plr_dsh_speed=10000;
SDL_Window* window;
SDL_Renderer* renderer;

float dt;
float start;
float end;

bool keydownE=false;

void loop(){
    if (lives<=0){
        dead=true;
    }
    if (dead){
        SDL_Event e;
        while (SDL_PollEvent(&e)){
        if (e.type==SDL_QUIT)
            emscripten_cancel_main_loop();
        }
        SDL_SetRenderDrawColor(renderer,255,0,0,255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        return;
    }
    start=SDL_GetTicks();
    dt=(start-end)/1000.f;
    end=start;
    if (dmg_cd>0) dmg_cd-=dt;
    if (dmg_cd<0) dmg_cd=0;
    const Uint8* keys=SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_W]){
        if (keys[SDL_SCANCODE_E] && !(keydownE))
            player.y-=plr_dsh_speed*dt;
        else
            player.y-=plr_speed*dt;
    }
    if (keys[SDL_SCANCODE_S]){
        if (keys[SDL_SCANCODE_E] && !(keydownE))
            player.y+=plr_dsh_speed*dt;
        else
            player.y+=plr_speed*dt;
    }
    if (keys[SDL_SCANCODE_A]){
        if (keys[SDL_SCANCODE_E] && !(keydownE))
            player.x-=plr_dsh_speed*dt;
        else
            player.x-=plr_speed*dt;
    }
    if (keys[SDL_SCANCODE_D]){
        if (keys[SDL_SCANCODE_E] && !(keydownE))
            player.x+=plr_dsh_speed*dt;
        else
            player.x+=plr_speed*dt;
    }
    if (SDL_HasIntersection(&ima_fckin_killer,&player) && dmg_cd==0){
        lives--;
        dmg_cd=2;
    }

    SDL_Event e;
    while (SDL_PollEvent(&e)){
        if (e.type==SDL_QUIT)
            emscripten_cancel_main_loop();
        if (e.type==SDL_KEYDOWN && e.key.keysym.sym==SDLK_e)
            keydownE=true;
        if (e.type==SDL_KEYUP && e.key.keysym.sym==SDLK_e)
            keydownE=false;
    }

    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderClear(renderer);
    if (dmg_cd==0)
    SDL_SetRenderDrawColor(renderer,0,0,255,255);
    else
    SDL_SetRenderDrawColor(renderer,255,0,255,255);
    SDL_RenderFillRect(renderer,&player);

    SDL_SetRenderDrawColor(renderer,255,0,0,255);
    SDL_RenderFillRect(renderer,&ima_fckin_killer);

    SDL_RenderPresent(renderer);
}

int main(){
    SDL_Init(SDL_INIT_EVERYTHING);

    window=SDL_CreateWindow("Game",0,0,1000,800,SDL_WINDOW_SHOWN);
    renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);
    
    emscripten_set_main_loop(loop,0,1);
    return 0;
}