#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <utility>
#include <emscripten/html5.h>
#include <iostream>
#include <ctime>
#include <functional>
#include <sstream>
#include <map>
#include <any>
#include <cmath>
#include <memory>
#define LINE_OFFSET 150
//#define emscripten_cancel_main_loop() currloop=close
//#define emscripten_set_main_loop(a,b,c) currloop=a

//MY HANDS SHALL RELISH ENDING YOU HERE AND KNOW!!!!
//1 minute later
//is that my blood?..

//works

int WAIT_TIME=1000;

void (*lastloop)();
void (*currloop)();

void loop(){
    currloop();
}

void close(){
    SDL_Quit();
}

void switch_loop(void (*to)()){
    lastloop=currloop;
    currloop=to;
}

bool dead=false;
SDL_Rect ima_fckin_killer{400,400,50,50};
float dmg_cd=0.f;
int lives=5;
SDL_FRect player{100,100,50,50};
int plr_speed=300;
int plr_dsh_speed=150;
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* textureW;

TTF_Font* arial;
SDL_Texture* GameOver_txt;
Mix_Music* l1_mus1;

float dt;
float start;
float end;

bool keydownDASH=false;
SDL_Scancode dashBut=SDL_SCANCODE_E;

bool move(SDL_FRect* rect, int targetX, int targetY, float speed, float delta) {
    float dx = targetX - rect->x;
    float dy = targetY - rect->y;
    float dist = SDL_sqrtf(dx * dx + dy * dy);

    if (dist <= speed * delta) { 
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


static float approach(float cur, float target, float maxStep)
{
    float d = target - cur;
    if (fabsf(d) <= maxStep) return target;
    return cur + (d > 0.f ? maxStep : -maxStep);
}

// speed = pixels per second (REAL)
void plush(SDL_FRect* rect,
           float tw,
           float th,
           float speed,
           float delta)
{
    if (!rect) return;

    float cx = rect->x + rect->w * 0.5f;
    float cy = rect->y + rect->h * 0.5f;

    float step = speed * delta;

    rect->w = approach(rect->w, tw, step);
    rect->h = approach(rect->h, th, step);

    rect->x = cx - rect->w * 0.5f;
    rect->y = cy - rect->h * 0.5f;

}

void DrawThickFRect(SDL_FRect rect, float thickness) {
    SDL_FRect top{rect.x, rect.y, rect.w, thickness};
    SDL_RenderFillRectF(renderer, &top);

    SDL_FRect bottom{rect.x, rect.y + rect.h - thickness, rect.w, thickness};
    SDL_RenderFillRectF(renderer, &bottom);

    SDL_FRect left{rect.x, rect.y, thickness, rect.h};
    SDL_RenderFillRectF(renderer, &left);

    SDL_FRect right{rect.x + rect.w - thickness, rect.y, thickness, rect.h};
    SDL_RenderFillRectF(renderer, &right);
}


template <typename T>
std::string to_str(T a){
    std::stringstream s;
    s<<a;
    return s.str();
}

class Enemy{
    public:
    bool active=true;
    bool isDamaging=false;
    bool exMode=false;
    unsigned char ndmgAlpha=155;
    float left;
    SDL_FRect rect;
    float ndmg_left=WAIT_TIME/1000.f;
    bool started=false;
    virtual void update(){};
    virtual ~Enemy()=default;
    void Handle(){
      if (ndmg_left>0){ 
          ndmg_left-=dt;
          if (ndmg_left<=0) isDamaging=true;
        }  
    } 
};

class Trail : public Enemy{
    public:
    SDL_FRect rect;
    unsigned char alpha=255;
    Trail(SDL_FRect r) : rect(r){}
    void update() override{
        alpha-=dt*255;
        if (alpha<=0){
            active=false;
            return;
        }
        SDL_SetRenderDrawColor(renderer,0,0,255,alpha);
        SDL_RenderFillRectF(renderer,&rect);
    }
};

class Sharik : public Enemy{
    public:
    std::vector<std::pair<int,int>> road;
    int s,e;
    int speed;
    int curr=0;
    Sharik(std::vector<std::pair<int,int>> r,int s,int e,int spee) : road(r), s(s), e(e), speed(spee){
        if (r[0].first==-1){
            exMode=true;
            left=r[0].second/1000.f;
            rect={(float)r[1].first,(float)r[1].second,(float)s,(float)s};
        }else
            rect={(float)r[0].first,(float)r[0].second,(float)s,(float)s};
    }
    void update() override{
        Handle();
        if (!isDamaging){
            SDL_SetRenderDrawColor(renderer,255,0,0,ndmgAlpha);
            SDL_RenderFillRectF(renderer,&rect);
            return;
        }
        if (exMode){
            left-=dt;
            if (left<=0)
                active=false;
        }
        else{
            rect.x-=speed*dt;
            rect.y-=speed*dt;
            rect.w+=speed*dt;
            rect.h+=speed*dt;
            if (curr>=road.size())
                curr=0;
            if (rect.w>e)
                active=false;
            if (!move(&rect,road[curr].first,road[curr].second,speed,dt))
                curr++;
        }
        SDL_SetRenderDrawColor(renderer,255,0,0,255);
        SDL_RenderFillRectF(renderer,&rect);
    }
};

class Laser : public Enemy{
    public:
    int speed;
    int curr=0;
    std::vector<int> cords;
    Laser(std::vector<int> cords,int s,int w):cords(cords),speed(s){
        if (cords[0]==-1){
            exMode=true;
            left=cords[1]/1000.f;
            rect={(float)cords[2],0,(float)w,2000};
        }else
            rect={(float)cords[0],0,(float)w,2000};
    }
    void update() override{
        Handle();
        if (!isDamaging){
            SDL_SetRenderDrawColor(renderer,255,0,0,ndmgAlpha);
            SDL_RenderFillRectF(renderer,&rect);
            return;
        }
        if (exMode){
            left-=dt;
            if (left<=0)
                active=false;
        }
        else{
            if (!move(&rect,cords[curr],0,speed,dt))
                curr++;
            if (curr>=cords.size())
                active=false;
        }
        SDL_SetRenderDrawColor(renderer,255,0,0,255);
        SDL_RenderFillRectF(renderer,&rect);
    }
};

//balls
class Ball : public Enemy{
    public:
    int speed;
    int curr=0;
    std::vector<std::pair<int,int>> road;
    Ball(std::vector<std::pair<int,int>> r,int s) : road(r),speed(s){
        if (r[0].first==-1){
            exMode=true;
            left=r[0].second/1000.f;
            rect={(float)r[1].first,(float)r[1].second,100,100};
        }else
            rect={(float)r[0].first,(float)r[0].second,100,100};
    }
    void update() override{
        Handle();
        if (!isDamaging){
            SDL_SetRenderDrawColor(renderer,255,0,0,ndmgAlpha);
            SDL_RenderFillRectF(renderer,&rect);
            return;
        }
        if (exMode){
            left-=dt;
            if (left<=0)
                active=false;
        }
        else{
            if (!move(&rect,std::get<0>(road[curr]),std::get<1>(road[curr]),speed,dt))
                curr++;
            if (curr>=road.size())
                active=false;
        }
        SDL_SetRenderDrawColor(renderer,255,0,0,255);
        SDL_RenderFillRectF(renderer,&rect);
    }
};

class Spike : public Enemy{
    public:
    float livetime;
    Spike(SDL_FRect r){
        livetime=0.f;
        rect=r;
    }
    void update() override{
        Handle();
        if (!isDamaging){
            SDL_SetRenderDrawColor(renderer,255,0,0,ndmgAlpha);
            SDL_RenderFillRectF(renderer,&rect);
            return;
        }
        if (livetime>=3.f)
            active=false;
        livetime+=dt;
        SDL_SetRenderDrawColor(renderer,255,0,0,255);
        SDL_RenderFillRectF(renderer,&rect);
    }
};

class Custom : public Enemy{
    public:
    std::map<std::string,std::any> vars;
    std::vector<int> path;
    int speed;
    int pause;
    void (*updatefunc)(Custom*);
    Custom(SDL_FRect r,void (*u)(Custom*),void (*c)(Custom*)){
        updatefunc=u;
        rect=r;
        c(this);
    }
    void update() override{
        Handle() ;
        updatefunc(this);
    }
};

inline void UpdateRenderer(){
    SDL_SetRenderTarget(renderer,NULL);
    SDL_RenderCopy(renderer,textureW,NULL,NULL);
    SDL_SetRenderTarget(renderer,textureW);
}

std::vector<Enemy*> enemies1;

void HandleDelta(){
    start=SDL_GetTicks();
    dt=(start-end)/1000.f;
    end=start;
}

void loop1();

void GameOver(){
    SDL_Event e;
    HandleDelta();
    while (SDL_PollEvent(&e)){
        if (e.type==SDL_QUIT)
            emscripten_cancel_main_loop();
        if (e.type==SDL_KEYDOWN)
            if (e.key.keysym.sym==SDLK_r){
                lives=5;
                player.x=400;
                player.y=400;
                dead=false;
                switch_loop(loop1);
            }
    }
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderClear(renderer);
    {
        SDL_Rect rect{200,200,500,200};
        SDL_RenderCopy(renderer,GameOver_txt,nullptr,&rect);
    }
    SDL_RenderPresent(renderer);
    UpdateRenderer();
}

std::vector<int> pauses;

std::vector<std::function<Enemy*()>>* objs;

Mix_Music* mus;

std::vector<int> pauses1={
    1000,
    2000,
    700
};

std::vector<std::function<Enemy*() >> objs1={
    [] () {return new Enemy;} ,
    [](){
        Ball* u=new Ball({{-1,2000},{200,200}},0);
        u->ndmgAlpha=0;
        return (Enemy*)u;
    },
    [] () {
        return (Enemy*)(new Ball({{-1,2000},{100,100}},0));
    } 
};

std::vector<std::tuple<std::vector<int>,std::vector<std::function<Enemy*()>>,Mix_Music**>> levels={{pauses1,objs1,&l1_mus1}};
int current_level=0;

int last_time=Mix_GetMusicPosition(mus);
int curr_interval=0;
int at=0;

int HandleList(){
    if (Mix_GetMusicPosition(mus)-last_time<curr_interval) return 0;
    if (at>=pauses.size())
        return -1;
    if (lasts<=0){
        enemies1.push_back((*objs)[at]());
        curr_interval=pauses[at]/1000.f;
        last_time=Mix_GetMusicPosition(mus);
        at++;
    }
    return 0;
}

void Empty(){}

struct Button{
    SDL_FRect rect;
    std::function<void()> onPressed=Empty;
    std::function<void()> Draw=Empty;
    Button(std::function<void()> f,std::function<void()> d,SDL_FRect r) : onPressed(f),Draw(d),rect(r){}
};

float settings_volume=64.f;
SDL_FRect settings_volume_up{200,200,100,100};
SDL_FRect settings_volume_down{200,400,100,100};
bool settings_fullscreen=false;
SDL_FRect settings_fullscreen_rect{500,100,100,100};
bool settings_isMouseDown=false;
bool settings_isMousePressedFirstFrame=true;
int settings_buttNo=0;
std::vector<Button> settings_buttons={
    Button{[](){settings_volume-=dt*30;Mix_VolumeMusic((int)settings_volume);},[](){SDL_SetRenderDrawColor(renderer,100,100,100,255);SDL_RenderFillRectF(renderer,&settings_volume_down);},settings_volume_down},
    Button{[](){settings_volume+=dt*30;Mix_VolumeMusic((int)settings_volume);},[](){SDL_SetRenderDrawColor(renderer,100,100,100,255);SDL_RenderFillRectF(renderer,&settings_volume_up);},settings_volume_up},
    Button{[](){
        settings_fullscreen=!settings_fullscreen;
        if  (settings_fullscreen){
            emscripten_request_fullscreen("#canvas",0);
        }
        else{ 
            emscripten_exit_fullscreen();
        }
    },
    [](){
        if (settings_fullscreen) SDL_SetRenderDrawColor(renderer,0,255,0,255);
        else SDL_SetRenderDrawColor(renderer,255,0,0,255);
        SDL_RenderFillRectF(renderer,&settings_fullscreen_rect);
    },settings_fullscreen_rect}
};

void settings(){
    HandleDelta();
    SDL_Event e;
    const Uint8* keys=SDL_GetKeyboardState(NULL);
    while (SDL_PollEvent(&e)){
        if (e.type==SDL_QUIT)
            emscripten_cancel_main_loop();
        if (e.type==SDL_KEYDOWN)
            if (e.key.keysym.sym==SDLK_F1){
                Mix_VolumeMusic((int)settings_volume);
                Mix_ResumeMusic();
                switch_loop(lastloop);
            }
        if (e.type==SDL_KEYDOWN)
            if (e.key.keysym.sym==SDLK_UP){
                settings_buttNo+=1;
                if (settings_buttNo>=settings_buttons.size()) settings_buttNo=settings_buttons.size()-1;
            }
        if (e.type==SDL_KEYDOWN)
            if (e.key.keysym.sym==SDLK_DOWN){
                settings_buttNo-=1;
                if (settings_buttNo<0) settings_buttNo=0;
            }
        if (e.type==SDL_KEYDOWN)
            if (e.key.keysym.sym==SDLK_KP_ENTER)
                settings_buttons[settings_buttNo].onPressed();
    }
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderClear(renderer);

    if (settings_volume<0) settings_volume=0;
    if (settings_volume>128) settings_volume=128;

    SDL_Rect sound_rect1{200, 100, 128 * 2, 50};

    int filled_width = settings_volume * 2;

    filled_width = SDL_clamp(filled_width, 0, sound_rect1.w);

    SDL_Rect sound_rect2{
        sound_rect1.x,
        sound_rect1.y,
        filled_width,
        sound_rect1.h
    };

    SDL_SetRenderDrawColor(renderer,255,0,0,255);
    SDL_RenderFillRect(renderer,&sound_rect1);
    SDL_SetRenderDrawColor(renderer,0,255,0,255);
    SDL_RenderFillRect(renderer,&sound_rect2);
    SDL_SetRenderDrawColor(renderer,100,100,100,255);

    for (auto& i:settings_buttons)
        i.Draw();
    
    SDL_SetRenderDrawColor(renderer,255,255,255,255);
    DrawThickFRect(settings_buttons[settings_buttNo].rect,10);

    SDL_RenderPresent(renderer);
    UpdateRenderer();
}

void loop1(){
    if (lives<=0){
        dead=true;
    }
    if (dead){
        switch_loop(GameOver);
    }

    HandleDelta();

    HandleList();

    if (dmg_cd>0) dmg_cd-=dt;
    if (dmg_cd<0) dmg_cd=0;
    const Uint8* keys=SDL_GetKeyboardState(NULL);

    const float dsh_w=75;
    const float dsh_h=25;

    if (keys[SDL_SCANCODE_W]){
        if (keys[dashBut] && !(keydownDASH)){
            player.y-=plr_dsh_speed;
            enemies1.push_back(new Trail(player));
            plush(&player,dsh_h,dsh_w,1000,dt);
        }
        else
            player.y-=plr_speed*dt;
    }
    if (keys[SDL_SCANCODE_S]){
        if (keys[dashBut] && !(keydownDASH)){
            player.y+=plr_dsh_speed;
            enemies1.push_back(new Trail(player));
            plush(&player,dsh_h,dsh_w,1000,dt);
        }
        else
            player.y+=plr_speed*dt;
    }
    if (keys[SDL_SCANCODE_A]){
        if (keys[dashBut] && !(keydownDASH)){
            player.x-=plr_dsh_speed;
            enemies1.push_back(new Trail(player));
            plush(&player,dsh_w,dsh_h,1000,dt);
        }
        else
            player.x-=plr_speed*dt;
    }
    if (keys[SDL_SCANCODE_D]){
        if (keys[dashBut] && !(keydownDASH)){
            player.x+=plr_dsh_speed;
            enemies1.push_back(new Trail(player));
            plush(&player,dsh_w,dsh_h,1000,dt);
        }
        else
            player.x+=plr_speed*dt;
    }

    if (keys[dashBut])
        keydownDASH=true;
    else
        keydownDASH=false;

    
    if (player.x>1000-player.w)
        player.x=1000-player.w;
    if (player.x<0)
        player.x=0;

    if (player.y>800-player.h)
        player.y=800-player.h;
    if (player.y<0)
        player.y=0;

    plush(&player,50,50,75,dt);

    SDL_Event e;
    while (SDL_PollEvent(&e)){
        if (e.type==SDL_QUIT)
            emscripten_cancel_main_loop();
        if (e.type==SDL_KEYDOWN)
            if (e.key.keysym.sym==SDLK_F1){
                Mix_PauseMusic();
                switch_loop(settings);
            }
    }

    SDL_SetRenderDrawColor(renderer,0,0,50,255);
    SDL_RenderClear(renderer);

    if (dmg_cd==0)
    SDL_SetRenderDrawColor(renderer,0,0,255,255);
    else
    SDL_SetRenderDrawColor(renderer,255,0,255,255);
    SDL_RenderFillRectF(renderer,&player);
    std::vector<Enemy*> real;
    for (auto i:enemies1){
        if (i->active)
            real.push_back(i);
        else{
            delete i;
            continue;
        }
        i->update();
        if (SDL_HasIntersectionF(&i->rect,&player) && dmg_cd==0 && i->isDamaging){
            lives--;
            dmg_cd=2;
        }
    }
    enemies1=real;
    
    /*
    if (enemies1.size()<7){
        int r=rand()%3;
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
            std::vector<int> rr;
            rr.resize(4);
            for (int i=0;i<4;i++){
                rr[i]=std::get<0>(ra[i]);
            }
            enemies1.push_back(new Laser(rr,200,50));
        }
        else if (r==2){
            enemies1.push_back(new Sharik(ra,50,150,200));
        }
    }
    */

    SDL_RenderPresent(renderer);
    UpdateRenderer();
}

void switch_level(int no){
    for (auto i:enemies1) delete i;
    Mix_HaltMusic();
    current_level=no;
    if (Mix_PlayMusic(*(std::get<2>(levels[current_level])),-1) == -1) {
        std::cout << "Mix_PlayMusic error: " << Mix_GetError() << std::endl;
    }
    pauses=std::get<0>(levels[current_level]);
    objs=&(std::get<1>(levels[current_level]));
    mus=*std::get<2>(levels[current_level]);
    at=0;
}

int main(){

    currloop=loop1;

    srand(time(NULL));

    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();
    Mix_Init(MIX_INIT_MP3 | MIX_INIT_OGG);

    if (Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,2048)==-1){
        std::cout<<"ERROR OF AUDIO:"<<Mix_GetError()<<std::endl;
    }

    window=SDL_CreateWindow("Game",0,0,1000,800,SDL_WINDOW_SHOWN);
    renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);

    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);

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

    l1_mus1=Mix_LoadMUS("assets/corrupted.mp3");
    if (!l1_mus1){
        std::cout<<"COULDNT LOAD MUSIC CAUSE:"<<Mix_GetError()<<std::endl;
        return 1;
    }
    textureW=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,1000,800);
    SDL_SetRenderTarget(renderer,textureW);

    switch_level(0);
    emscripten_set_main_loop(loop,0,1);
    return 0;
}
