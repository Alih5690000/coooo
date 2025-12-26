#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <utility>
#include <emscripten.h>
#include <iostream>
#include <ctime>
#include <sstream>
#include <map>
#include <any>
#include <memory>
//#define emscripten_cancel_main_loop() currloop=close
//#define emscripten_set_main_loop(a,b,c) currloop=a

//MY HANDS SHALL RELISH ENDING YOU HERE AND KNOW!!!!
//1 minute later
//is that my blood?..

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
SDL_Rect player{100,100,50,50};
int plr_speed=300;
int plr_dsh_speed=10000;
SDL_Window* window;
SDL_Renderer* renderer;

TTF_Font* arial;
SDL_Texture* GameOver_txt;
Mix_Music* l1_mus1;

float dt;
float start;
float end;

bool keydownDASH=false;
SDL_Scancode dashBut=SDL_SCANCODE_E;

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
    float left;
    SDL_Rect rect;
    virtual void update(){};
    virtual ~Enemy()=default;
};

class Sharik : public Enemy{
    public:
    std::vector<std::pair<int,int>> road;
    int s,e;
    int speed;
    int curr=0;
    int warning_since=0;
    Sharik(std::vector<std::pair<int,int>> r,int s,int e,int spee) : road(r), s(s), e(e), speed(spee){
        if (r[0].first==-1){
            exMode=true;
            left=r[0].second;
            rect={r[1].first,r[1].second,s,s};
        }else
            rect={r[0].first,r[0].second,s,s};
        warning_since=start;
    }
    void update() override{
        if (start-warning_since>WAIT_TIME)
            isDamaging=true;
        if (!isDamaging){
            SDL_SetRenderDrawColor(renderer,255,0,0,155);
            SDL_RenderFillRect(renderer,&rect);
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
        SDL_RenderFillRect(renderer,&rect);
    }
};

class Laser : public Enemy{
    public:
    int speed;
    int curr=0;
    int warning_since=0;
    std::vector<int> cords;
    Laser(std::vector<int> cords,int s,int w):cords(cords),speed(s){
        if (cords[0]==-1){
            exMode=true;
            left=cords[1];
            rect={cords[2],0,w,2000};
        }else
            rect={cords[0],0,w,2000};
        warning_since=start;
    }
    void update() override{
        if (start-warning_since>WAIT_TIME)
            isDamaging=true;
        if (!isDamaging){
            SDL_SetRenderDrawColor(renderer,255,0,0,155);
            SDL_RenderFillRect(renderer,&rect);
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
        SDL_RenderFillRect(renderer,&rect);
    }
};

//balls
class Ball : public Enemy{
    public:
    int speed;
    int curr=0;
    int warning_since=0;
    std::vector<std::pair<int,int>> road;
    Ball(std::vector<std::pair<int,int>> r,int s) : road(r),speed(s){
        if (r[0].first==-1){
            exMode=true;
            left=r[0].second;
            rect={r[1].first,r[1].second,100,100};
        }else
            rect={r[0].first,r[0].second,100,100};
        warning_since=start;
    }
    void update() override{
        if (start-warning_since>WAIT_TIME)
            isDamaging=true;
        if (!isDamaging){
            SDL_SetRenderDrawColor(renderer,255,0,0,155);
            SDL_RenderFillRect(renderer,&rect);
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
        SDL_RenderFillRect(renderer,&rect);
    }
};

class Spike : public Enemy{
    public:
    float livetime;
    int warning_since=0;
    Spike(SDL_Rect r){
        livetime=0.f;
        rect=r;
        warning_since=start;
    }
    void update() override{
        if (start-warning_since>WAIT_TIME)
            isDamaging=true;
        if (!isDamaging){
            SDL_SetRenderDrawColor(renderer,255,0,0,155);
            SDL_RenderFillRect(renderer,&rect);
            return;
        }
        if (livetime>=3.f)
            active=false;
        livetime+=dt;
        SDL_SetRenderDrawColor(renderer,255,0,0,255);
        SDL_RenderFillRect(renderer,&rect);
    }
};

class Custom : public Enemy{
    public:
    std::map<std::string,std::any> vars;
    std::vector<int> path;
    int speed;
    int pause;
    void (*updatefunc)(Custom*);
    Custom(SDL_Rect r,void (*u)(Custom*),void (*c)(Custom*)){
        updatefunc=u;
        rect=r;
        c(this);
    }
    void update() override{
        updatefunc(this);
    }
};

void Parse(std::string list,std::vector<Enemy*>* ens){
    std::vector<std::string> tokens;
    std::stringstream stream(list);
    std::string temp;
    while (stream>>temp){
        tokens.push_back(temp);
    }
    for (int i=0;i<tokens.size();i++){
        auto token=tokens[i];
        if (token=="SHARIK"){
            std::vector<std::pair<int,int>> path;
            int start,end,speed;
            i++;
            if (tokens[i]!="START")
                throw std::runtime_error("Invalid syntax (no START)");
            i++;
            while(true){
                if (tokens[i]=="END")
                    break;
                if (i>=tokens.size())
                    throw std::runtime_error("START unclosed");
                path.push_back(std::make_pair(std::stoi(tokens[i]),std::stoi(tokens[i+1])));
                i+=2;
            }
            i++;
            if (tokens.size()<=i+2)
                throw std::runtime_error("too few arguments");
            start=std::stoi(tokens[i]);
            end=std::stoi(tokens[i+1]);
            speed=std::stoi(tokens[i+2]);
            Enemy* en=new Sharik(path,start,end,speed);
            ens->push_back(en);
            i+=3;
        }
        else if (token=="BALL"){
            std::vector<std::pair<int,int>> path;
            int speed;
            i++;
            if (tokens[i]!="START")
                throw std::runtime_error("Invalid syntax (no START)");
            i++;
            while(true){
                if (tokens[i]=="END")
                    break;
                if (i>=tokens.size())
                    throw std::runtime_error("START unclosed");
                path.push_back(std::make_pair(std::stoi(tokens[i]),std::stoi(tokens[i+1])));
                i+=2;
            }
            i++;
            if (tokens.size()<=i)
                throw std::runtime_error("too few arguments");
            speed=std::stoi(tokens[i]);
            Enemy* en=new Ball(path,speed);
            int size=std::stoi(tokens[i+1]);
            en->rect.w=size;
            en->rect.h=size;
            ens->push_back(en);
            i++;
        }
        else if (token=="LASER"){
            std::vector<int> path;
            int speed,width;
            i++;
            if (tokens[i]!="START")
                throw std::runtime_error("Invalid syntax (no START)");
            i++;
            while(true){
                if (tokens[i]=="END")
                    break;
                if (i>=tokens.size())
                    throw std::runtime_error("START unclosed");
                path.push_back(std::stoi(tokens[i]));
                i++;
            }
            i++;
            if (tokens.size()<=i+1)
                throw std::runtime_error("too few arguments");
            speed=std::stoi(tokens[i]);
            width=std::stoi(tokens[i+1]);
            ens->push_back(new Laser(path,speed,width));
            i++;
        }
        else if (token=="SPIKE"){
            int x,y,w,h;
            i++;
            x=std::stoi(tokens[i]);
            i++;
            y=std::stoi(tokens[i]);
            i++;
            w=std::stoi(tokens[i]);
            i++;
            h=std::stoi(tokens[i]);
            ens->push_back(new Spike({x,y,w,h}));
        }
    }
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
}

std::vector<std::string> coms={
    "BALL START -1 2 400 400 END 300 50",
    "500",

    "BALL START 900 200 0 200 END 200 50",
    "500",

    "BALL START 900 600 0 600 END 400 50",
    "250",

    "BALL START 900 100 0 100 END 200 50",
    "500",

    "BALL START 900 300 0 300 END 150 50",
    "500",

    //

    "BALL START 900 400 0 400 END 150 50",
    "500",

    "BALL START 900 100 0 100 END 150 50",
    "500",
    
    "BALL START 900 600 0 600 END 150 50",
    "250",

    "BALL START 900 400 0 400 END 150 50",
    "500",

    "BALL START 900 900 0 900 END 150 50",
    "250",

    "BALL START 900 100 0 100 END 150 50",
    "250",

    "BALL START 900 600 0 600 END 150 50",
    "250",

    "BALL START 900 200 0 200 END 150 50",
    "500",

};

std::vector<std::pair<std::vector<std::string>,Mix_Music*>> levels={{coms,l1_mus1}};
int current_level=0;

int curr_interval=0;
int last_time=0;
int at=0;


void HandleList(){
    if (start-last_time>curr_interval){
        if (at<coms.size()){
            if (at%2==1 && at!=0){
                curr_interval=std::stoi(coms[at]);
                last_time=start;
            }
            else{
                Parse(levels[current_level].first[at],&enemies1);
            }
            at++;
        }
    }
}

float settings_volume=64.f;
SDL_Rect settings_volume_up{200,200,100,100};
SDL_Rect settings_volume_down{200,400,100,100};
std::vector<SDL_Rect> settings_buttons={settings_volume_up,settings_volume_down};
void settings(){
    int x,y;
    Uint8 mstate=SDL_GetMouseState(&x,&y);
    HandleDelta();
    SDL_Event e;
    while (SDL_PollEvent(&e)){
        if (e.type==SDL_QUIT)
            emscripten_cancel_main_loop();
        if (e.type==SDL_KEYDOWN)
            if (e.key.keysym.sym==SDLK_ESCAPE){
                Mix_VolumeMusic((int)settings_volume);
                Mix_ResumeMusic();
                switch_loop(lastloop);
            }
    }
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderClear(renderer);

    if (settings_volume<0) settings_volume=0;
    if (settings_volume>128) settings_volume=128;

    SDL_Rect tchrect{x,y,1,1};
    if (SDL_HasIntersection(&settings_volume_down,&tchrect) && (mstate & SDL_BUTTON_LMASK)){
        settings_volume-=dt*30;
        Mix_VolumeMusic((int)settings_volume);
    }
    if (SDL_HasIntersection(&settings_volume_up,&tchrect) && (mstate & SDL_BUTTON_LMASK)){
        settings_volume+=dt*30;
        Mix_VolumeMusic((int)settings_volume);
    }

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
        SDL_RenderFillRect(renderer,&i);

    SDL_Rect r1{200,100,200,100};
    
    SDL_RenderPresent(renderer);
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

    if (keys[SDL_SCANCODE_W]){
        if (keys[dashBut] && !(keydownDASH))
            player.y-=plr_dsh_speed*dt;
        else
            player.y-=plr_speed*dt;
    }
    if (keys[SDL_SCANCODE_S]){
        if (keys[dashBut] && !(keydownDASH))
            player.y+=plr_dsh_speed*dt;
        else
            player.y+=plr_speed*dt;
    }
    if (keys[SDL_SCANCODE_A]){
        if (keys[dashBut] && !(keydownDASH))
            player.x-=plr_dsh_speed*dt;
        else
            player.x-=plr_speed*dt;
    }
    if (keys[SDL_SCANCODE_D]){
        if (keys[dashBut] && !(keydownDASH))
            player.x+=plr_dsh_speed*dt;
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

    SDL_Event e;
    while (SDL_PollEvent(&e)){
        if (e.type==SDL_QUIT)
            emscripten_cancel_main_loop();
        if (e.type==SDL_KEYDOWN)
            if (e.key.keysym.sym==SDLK_ESCAPE){
                Mix_PauseMusic();
                switch_loop(settings);
            }
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
        else{
            delete i;
            continue;
        }
        i->update();
        if (SDL_HasIntersection(&i->rect,&player) && dmg_cd==0 && i->isDamaging){
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
}

void switch_level(int no){
    Mix_HaltMusic();
    current_level=no;
    Mix_PlayMusic(levels[current_level].second,0);
    coms=levels[current_level].first;
    at=0;
    for (auto i:enemies1)
        delete i;
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
    
    Mix_PlayMusic(l1_mus1,0);
    emscripten_set_main_loop(loop,0,1);
    return 0;
}