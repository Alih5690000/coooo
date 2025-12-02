#include <SDL2/SDL.h>
#include "SDL_ttf/SDL_ttf.h"
#include <vector>
#include <utility>
#include <emscripten.h>
#include <iostream>
#include <ctime>

void (*lastloop)();

bool dead=false;
SDL_Rect ima_fckin_killer{400,400,50,50};
float dmg_cd=0.f;
int lives=5;
SDL_Rect player{100,100,50,50};
int plr_speed=300;
int plr_dsh_speed=10000;
SDL_Window* window;
SDL_Renderer* renderer;

TTF_Font* arial;
SDL_Texture* GameOver_txt;

float dt;
float start;
float end;

bool keydownE=false;

bool move(SDL_Rect* rect, int targetX, int targetY, float speed, float delta) {
    float dx = targetX - rect->x;
    float dy = targetY - rect->y;
    float dist = SDL_sqrtf(dx * dx + dy * dy);

    if (dist < speed * delta) { 
        rect->x = targetX;
        rect->y = targetY;
        return false;
    }

    dx /= dist;  // Normalize direction
    dy /= dist;

    // Move by a constant speed in both directions
    rect->x += dx * speed * delta;
    rect->y += dy * speed * delta;

    return true;
}


class Enemy{
    public:
    bool active=true;
    SDL_Rect rect;
    virtual void update(){};
};

class Laser : public Enemy{
    public:
    int speed;
    int curr=0;
    std::vector<int> cords;
    Laser(std::vector<int> cords,int s,int w):cords(cords),speed(s){rect={cords[0],0,w,2000};}
    void update() override{
        if (!move(&rect,cords[curr],0,speed,dt))
            curr++;
        if (curr>=cords.size())
            active=false;
        SDL_SetRenderDrawColor(renderer,255,0,0,255);
        SDL_RenderFillRect(renderer,&rect);
    }
};

class Ball : public Enemy{
    public:
    int speed;
    int curr=0;
    std::vector<std::pair<int,int>> road;
    Ball(std::vector<std::pair<int,int>> r,int s) : road(r),speed(s){rect={0,0,50,50};}
    void update() override{
        if (!move(&rect,std::get<0>(road[curr]),std::get<1>(road[curr]),speed,dt))
            curr++;
        if (curr>=road.size())
            active=false;
        SDL_SetRenderDrawColor(renderer,255,0,0,255);
        SDL_RenderFillRect(renderer,&rect);
    }
};

Ball l1_ball({{0,0},{300,300},{600,300}},300);
Ball l1_ball2({{0,0},{300,0},{300,300}},300);
Ball l1_ball3({{300,300},{300,400},{500,700}},300);
Ball l1_ball4({{900,200},{800,400},{700,700}},300);
Ball l1_ball5({{900,500},{800,700},{700,700}},300);
Laser l1_laser1({400,100},100,50);
Laser l1_laser2({600,900},100,50);

std::vector<Enemy*> enemies1={&l1_ball,&l1_ball2,&l1_ball3,&l1_ball4,&l1_laser1,&l1_laser2};

void loop1();

void GameOver(){
    SDL_Event e;
    while (SDL_PollEvent(&e)){
        if (e.type==SDL_QUIT)
            emscripten_cancel_main_loop();
        if (e.type==SDL_KEYDOWN)
            if (e.key.keysym.sym==SDLK_r){
                lives=5;
                player.x=400;
                player.y=400;
                dead=false;
                emscripten_cancel_main_loop();
                emscripten_set_main_loop(loop1,0,1);
            }
    }
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderClear(renderer);
    {
        SDL_Rect rect{200,200,500,200};
        SDL_RenderCopy(renderer,GameOver_txt,nullptr,&rect);
    }
    SDL_RenderPresent(renderer);
}

void loop1(){
    if (lives<=0){
        dead=true;
    }
    if (dead){
        emscripten_cancel_main_loop();
        emscripten_set_main_loop(GameOver,0,1);
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
    std::vector<Enemy*> real;
    for (auto i:enemies1){
        if (i->active)
            real.push_back(i);
        else
            continue;
        i->update();
        if (SDL_HasIntersection(&i->rect,&player) && dmg_cd==0){
            lives--;
            dmg_cd=2;
        }
    }
    enemies1=real;

    if (enemies1.size()<7){
        int r=rand()%2;
        std::vector<std::pair<int,int>> ra;
        ra.resize(4);
        for (auto& i:ra){
            int x=(rand()%1000)+1;
            int y=(rand()%800)+1;
            i=std::make_pair(x,y);
        }
        if (r==0)
            enemies1.push_back(new Ball(ra,200));
        else if(r==1){
            std::cout<<"LASER"<<std::endl;
            std::vector<int> rr;
            rr.resize(4);
            for (int i=0;i<4;i++){
                rr[i]=std::get<0>(ra[i]);
            }
            enemies1.push_back(new Laser(rr,200,50));
        }
    }

    SDL_RenderPresent(renderer);
}

int main(){

    srand(time(NULL));

    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();

    window=SDL_CreateWindow("Game",0,0,1000,800,SDL_WINDOW_SHOWN);
    renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);

    arial=TTF_OpenFont("assets/arialmt.ttf",100);
    if (!arial){
        std::cout<<"FCK U BOZO CUDNT OPEN ARIAL"<<TTF_GetError()<<std::endl;
        return 1;
    }

    {
        SDL_Surface* surf=TTF_RenderText_Solid(arial,"Game Over",SDL_Color{255,255,255,255});
        if (!surf){
            std::cout<<"COULDNT TRANSFORM SURF"<<std::endl;
            return 1;
        }
        GameOver_txt=SDL_CreateTextureFromSurface(renderer,surf);
        SDL_FreeSurface(surf);
    }
    
    emscripten_set_main_loop(loop1,0,1);
    return 0;
}