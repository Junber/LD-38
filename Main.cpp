#include <SDL.h>
#include <SDL_image.h>
//#include <SDL2_gfxPrimitives.h>
#include <deque>
#include <map>
#include <string>
#include <algorithm>
#include <time.h>
#include <random>

void *__gxx_personality_v0;

const int window[2] = {500,500}, map_size[2] = {50,50};
const int tile_size = 40;
const int durability_drop = 2, weapon_durability_drop = 3;
const float weapon_multiplier = 0.8;

int camera_pos[2] = {0,0};
int selected_arm = 1;

bool breakk = false;
SDL_Window* renderwindow;
SDL_Renderer* renderer;

int last_time;
float wait;
void limit_fps()
{
    wait = (100.0/6)-(SDL_GetTicks() - last_time);
    if (wait>0) SDL_Delay(wait);
    last_time = SDL_GetTicks();
}

std::default_random_engine generator;
int random(int x, int y)
{
    std::uniform_int_distribution<int> distribution(x,y);
    return distribution(generator);
}

void random_init()
{
    generator.seed(time(nullptr));
    random(0,1);
}

template<class t> void remove_it(std::deque<t>* base, t thing)
{
    base->erase( std::remove( std::begin(*base), std::end(*base), thing ), std::end(*base) );
}

std::map<std::string,SDL_Texture*> loaded_textures;
SDL_Texture* load_image(std::string s)
{
    if (!loaded_textures.count(s)) loaded_textures[s] = IMG_LoadTexture(renderer,("Data\\Graphics\\"+s+".png").c_str());

    return loaded_textures[s];
}

class Arm
{
public:
    int hp, strenght;
    bool worn;
    Arm* carrying;

    Arm()
    {
        hp = 10;
        strenght=1;
    }
};

class Object;
std::deque<Object*> objects, walls;
class Object
{
public:
    int pos[2], size[2];
    SDL_Texture* tex;

    Object(int x, int y, std::string s)
    {
        pos[0] = x;
        pos[1] = y;
        tex = load_image(s);
        SDL_QueryTexture(tex, nullptr, nullptr, &size[0], &size[1]);

        objects.push_back(this);
    }

    virtual ~Object()
    {
        remove_it(&objects, this);
    }

    virtual void update()
    {

    }

    virtual void render()
    {
        SDL_Rect r={int((pos[0]-camera_pos[0]+0.5)*tile_size-size[0]/2), (pos[1]-camera_pos[1]+1)*tile_size-size[1], size[0], size[1]};

        SDL_RenderCopy(renderer, tex, nullptr, &r);
    }
};

class Character: public Object
{
public:
    int hp;
    std::deque<Arm*> worn;

    Character(int x, int y, std::string s): Object(x,y,s)
    {
        hp = 10;
    }

    void attack(Arm* arm)
    {
        hp -= arm->strenght + (arm->carrying? arm->carrying->strenght*weapon_multiplier:0);
        arm->hp -= durability_drop;
        if (arm->carrying) arm->carrying->hp -= weapon_durability_drop;
    }

    virtual void ai()
    {
        //...
    }
};
Character* player;
std::deque<Character*> enemies;

void generate_level(int tiles_to_generate)
{
    while (!walls.empty())
    {
        delete walls[0];
        walls.pop_front();
    }

    bool free_tiles[map_size[0]][map_size[1]];

    int walker_pos[2] = {map_size[0]/2,map_size[1]/2};
    int tiles=0;

    while (tiles < tiles_to_generate)
    {
        if (!free_tiles[walker_pos[0]][walker_pos[1]])
        {
            free_tiles[walker_pos[0]][walker_pos[1]] = true;
            tiles++;
        }
        int r = random(0,3);
        walker_pos[0] += (r==0)?-1:((r==1)?1:0);
        walker_pos[1] += (r==2)?-1:((r==3)?1:0);

        if (walker_pos[0] < 0) walker_pos[0] = 0;
        else if (walker_pos[0] >= map_size[0]) walker_pos[0] = map_size[0];
        if (walker_pos[1] < 0) walker_pos[1] = 0;
        else if (walker_pos[1] >= map_size[1]) walker_pos[1] = map_size[1];
    }

    for (int i=0; i<map_size[0]; i++)
    {
        for (int u=0; u<map_size[1]; u++)
        {
            if (!free_tiles[i][u])
            {
                //l u r d
                walls.push_back(new Object(i,u,"Wall"));
                //new Object(i,u,"Wall"+std::to_string(walls[i-1][u])+std::to_string(walls[i][u-1])+std::to_string(walls[i+1][u])+std::to_string(walls[i][u+1]));
            }
        }
    }
}

int main(int argc, char* args[])
{
    IMG_Init(IMG_INIT_PNG);

    renderwindow = SDL_CreateWindow("LD 38", 50, 50, window[0], window[1], SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(renderwindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    generate_level(100);
    player = new Character(map_size[0]/2,map_size[1]/2,"Player");

    //SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    SDL_Event e;
	while (!breakk)
    {
        while(SDL_PollEvent(&e))
        {
			if (e.type == SDL_QUIT) breakk = true;

			else if (e.type == SDL_KEYDOWN)
			{
			    if (e.key.keysym.sym == SDLK_ESCAPE) breakk = true;
			    else if (e.key.keysym.sym == SDLK_1) selected_arm=1;
			    else if (e.key.keysym.sym == SDLK_2) selected_arm=2;
			    else if (e.key.keysym.sym == SDLK_3) selected_arm=3;
			    else if (e.key.keysym.sym == SDLK_4) selected_arm=4;
			    else if (e.key.keysym.sym == SDLK_5) selected_arm=5;
			    else if (e.key.keysym.sym == SDLK_6) selected_arm=6;
			    else
                {
                    int di[2] = {0,0};

                    if (e.key.keysym.sym == SDLK_UP || e.key.keysym.sym == SDLK_w) di[1]--;
                    else if (e.key.keysym.sym == SDLK_DOWN || e.key.keysym.sym == SDLK_s) di[1]++;
                    else if (e.key.keysym.sym == SDLK_LEFT || e.key.keysym.sym == SDLK_a) di[0]--;
                    else if (e.key.keysym.sym == SDLK_RIGHT || e.key.keysym.sym == SDLK_d) di[0]++;
                    else continue;

                    player->pos[0] += di[0];
                    player->pos[1] += di[1];

                    bool possible_move = true;

                    for (Object* e: walls)
                    {
                        if (e->pos[0] == player->pos[0] && e->pos[1] == player->pos[1])
                        {
                            possible_move = false;
                            break;
                        }
                    }

                    if (!possible_move)
                    {
                        player->pos[0] -= di[0];
                        player->pos[1] -= di[1];
                        continue;
                    }

                    for (Character* e: enemies)
                    {
                        if (e->pos[0] == player->pos[0] && e->pos[1] == player->pos[1])
                        {
                            player->pos[0] -= di[0];
                            player->pos[1] -= di[1];
                            e->attack(player->worn[selected_arm-1]);
                            break;
                        }
                    }

                    for (Character* e: enemies)
                    {
                        e->ai();
                    }

                    camera_pos[0] = player->pos[0]-window[0]/(tile_size*2);
                    camera_pos[1] = player->pos[1]-window[1]/(tile_size*2);
                }
			}
        }

        SDL_SetRenderDrawColor(renderer,255,255,255,255);
        SDL_RenderClear(renderer);

        for (Object* o: objects)
        {
            o->update();
            o->render();
        }

        SDL_RenderPresent(renderer);
        limit_fps();
    }

    return 0;
}
