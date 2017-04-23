#include <SDL.h>
#include <SDL_image.h>
//#include <SDL2_gfxPrimitives.h>
#include <iostream>
#include <deque>
#include <map>
#include <string>
#include <algorithm>
#include <time.h>
#include <random>

void *__gxx_personality_v0;

const int window[2] = {500,500}, map_size[2] = {64,64};
const int tile_size = 40;
const int durability_drop = 1, weapon_durability_drop = 2, throw_durability_drop = 2, kick_damage=2, bomb_explode_time=4, bomb_explosion_radius=5;
const float weapon_multiplier = 0.8;

int camera_pos[2] = {0,0};
int selected_arm = 0;

bool breakk = false;
SDL_Window* renderwindow;
SDL_Renderer* renderer;

void render_everything();
class Character;
Character* player;
std::deque<Character*> enemies;
class Object;
std::deque<Object*> objects, walls, to_delete;
class Item;
std::deque<Item*> items;

enum
{
    normal,equip,throwing
}select_mode;

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

int sign(int i)
{
    return i<0?-1:(i>0?1:0);
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

void death_screen()
{
    breakk = true;
    SDL_SetRenderDrawColor(renderer,255,0,0,255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

class Arm
{
public:
    int hp, max_hp, strenght;
    bool over_filled;
    std::string name;
    Arm *carrying, *carried_by_arm;
    Character* carried_by_char;

    Arm(int blood, int attack, std::string namee)
    {
        max_hp = hp = blood;
        strenght=attack;
        name = namee;

        over_filled = false;

        carrying = nullptr;
        carried_by_arm = nullptr;
        carried_by_char = nullptr;
    }

    void kill();
};

class Object
{
public:
    int pos[2], size[2];
    bool background;
    SDL_Texture* tex;

    Object(int x, int y, std::string s, bool bg=false)
    {
        pos[0] = x;
        pos[1] = y;
        background = bg;
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

    virtual void kill()
    {
        to_delete.push_back(this);
    }
};

bool wall_or_void(int x, int y)
{
    if (x < 0 || x >= map_size[0] || y < 0 || y >= map_size[1]) return true;

    for (Object* o: walls)
    {
        if (x == o->pos[0] && y == o->pos[1])
        {
            return true;
        }
    }

    return false;
}

class Item: public Object
{
public:
    Arm* arm;
    int lifetime;

    Item(int x, int y, Arm* a) : Object(x,y,a->name,true)
    {
        arm = a;
        items.push_back(this);

        arm->carried_by_arm = nullptr;
        arm->carried_by_char = nullptr;
    }

    ~Item()
    {
        remove_it(&items,this);
    }

    void ai();
};

std::pair<int,int> place_item(int pos_x, int pos_y, Item* this_item=nullptr)
{
    int dist = 0;
    while (dist <= map_size[0])
    {
        for (int i=pos_x-dist;i<=pos_x+dist;i++)
        {
            for (int u=pos_y-dist;u<=pos_y+dist;u++)
            {
                if (!wall_or_void(i,u))
                {
                    bool found_item = false;
                    for (Item* itt: items)
                    {
                        if (itt->pos[0] == i && itt->pos[1] == u && itt != this_item)
                        {
                            found_item = true;
                            break;
                        }
                    }
                    if (!found_item) return std::pair<int,int>(i,u);
                }
            }
        }
        dist++;
    }

    return std::pair<int,int>(pos_x,pos_y); //Hopefully this won't happen
}

class Character: public Object
{
public:
    int hp, explosion_radius;
    bool enemy;
    Arm* worn[6];

    Character(int x, int y, std::string s, int blood, int explosion_rad, bool player=false): Object(x,y,s)
    {
        hp = blood;
        enemy = !player;
        explosion_radius = explosion_rad;

        for (int i=0;i<6;i++) worn[i] = nullptr;

        if (enemy) enemies.push_back(this);
    }

    ~Character()
    {
        if (enemy) remove_it(&enemies, this);
        else death_screen();
    }

    bool add_arm(Arm* arm)
    {
        for (int i=0;i<6;i++)
        {
            if (!worn[i])
            {
                worn[i] = arm;
                arm->carried_by_char=this;
                return true;
            }
        }
        for (int i=0;i<6;i++)
        {
            if (!worn[i]->carrying)
            {
                worn[i]->carrying = arm;
                arm->carried_by_arm=worn[i];
                return true;
            }
        }
        return false;
    }

    void kill();

    void attack(Arm* arm)
    {
        if (arm)
        {
            hp -= arm->strenght + (arm->carrying? arm->carrying->strenght*weapon_multiplier:0);
            if (hp <= 0) kill();

            if (arm->carrying)
            {
                arm->carrying->hp -= weapon_durability_drop;
                if (arm->carrying->hp <= 0) arm->carrying->kill();
            }

            arm->hp -= durability_drop;
            if (arm->hp <= 0) arm->kill();
        }
        else
        {
            hp -= kick_damage;
            if (hp <= 0) kill();
        }
    }

    virtual void ai()
    {
        int last_pos[2] = {pos[0],pos[1]};

        if (abs(player->pos[0]-pos[0]) >= abs(player->pos[1]-pos[1])) pos[0] += sign(player->pos[0]-pos[0]);
        else pos[1] += sign(player->pos[1]-pos[1]);

        if (wall_or_void(pos[0],pos[1]))
        {
            pos[0] = last_pos[0];
            pos[1] = last_pos[1];
        }

        for (Object* o: enemies)
        {
            if (pos[0] == o->pos[0] && pos[1] == o->pos[1] && o!=this)
            {
                pos[0] = last_pos[0];
                pos[1] = last_pos[1];
            }
        }

        if (pos[0] == player->pos[0] && pos[1] == player->pos[1])
        {
            pos[0] = last_pos[0];
            pos[1] = last_pos[1];

            player->attack(worn[0]);
        }
    }
};

void explosion(int x, int y, bool bomb, int range=bomb_explosion_radius)
{
    const int radius_square = range*range/4;

    Arm* explosion_arm = new Arm(9999999,1,"");

    for (int i=-range/2;i<=range;i++)
    {
        for (int u=-range/2;u<=range;u++)
        {
            if (i*i+u*u<=radius_square)
            {
                if (bomb)
                {
                    int pos[2] = {x+i, y+u};

                    for (Character* e: enemies)
                    {
                        if (e->pos[0] == pos[0] && e->pos[1] == pos[1]) e->attack(explosion_arm);
                    }

                    if (player->pos[0] == pos[0] && player->pos[1] == pos[1]) player->attack(explosion_arm);
                }
            }
        }
    }

    render_everything();

    for (int frames=0;frames<=5;frames++)
    {
        for (int i=-range/2;i<=range;i++)
        {
            for (int u=-range/2;u<=range;u++)
            {
                if (i*i+u*u<=radius_square)
                {
                    SDL_SetRenderDrawColor(renderer,255,random(100,255),0,255);
                    SDL_Rect r={int((x+i-camera_pos[0])*tile_size), (y+u-camera_pos[1])*tile_size, tile_size, tile_size};
                    SDL_RenderFillRect(renderer,&r);
                }
            }
        }
        SDL_RenderPresent(renderer);
        limit_fps();
    }
}

void Arm::kill()
{
    if (carried_by_arm) carried_by_arm->carrying=nullptr;
    if (carried_by_char)
    {
        for (int i=0;i<6;i++)
        {
            if (carried_by_char->worn[i] == this)
            {
                carried_by_char->worn[i] = nullptr;
                if (carrying) carried_by_char->worn[i] = carrying;
            }
        }
    }

    delete this;
}

void Character::kill()
{
    for (int i=0;i<5;i++)
    {
        if (worn[i])
        {
            worn[i]->carried_by_char=nullptr;
            auto pos_i = place_item(pos[0],pos[1]);
            new Item(pos_i.first,pos_i.second,worn[i]);

            if (worn[i]->carrying)
            {
                worn[i]->carrying->carried_by_arm=nullptr;
                auto pos_i = place_item(pos[0],pos[1]);
                new Item(pos_i.first,pos_i.second,worn[i]->carrying);
            }

            worn[i] = nullptr;
        }
    }

    explosion(pos[0], pos[1], false, explosion_radius);

    to_delete.push_back(this);
}

void Item::ai()
{
    lifetime++;
    if (lifetime >= bomb_explode_time && arm->over_filled)
    {
        explosion(pos[0],pos[1],true);
        kill();
    }
}

struct Enemy_type
{
    int arms, blood, blood_per_arm, strenght_per_arm, explosion_radius;
    int number, min_radius, max_radius;
    std::string tex, arm_name;
};
std::deque<Enemy_type*> enemy_types;

void generate_level(int tiles_to_generate)
{
    while (!walls.empty())
    {
        delete walls[0];
        walls.pop_front();
    }

    bool free_tiles[map_size[0]][map_size[1]];

    for (int i=0; i<map_size[0]; i++)
    {
        for (int u=0; u<map_size[1]; u++)
        {
            free_tiles[i][u] = false;
        }
    }

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

        /*if (walker_pos[0] < 0) walker_pos[0] = 0;
        else if (walker_pos[0] >= map_size[0]) walker_pos[0] = map_size[0];
        if (walker_pos[1] < 0) walker_pos[1] = 0;
        else if (walker_pos[1] >= map_size[1]) walker_pos[1] = map_size[1];*/
        if (walker_pos[0] < 1 || walker_pos[0] >= map_size[0]-1 || walker_pos[1] < 1 || walker_pos[1] >= map_size[1]-1)
        {
            do
            {
                walker_pos[0] = random(0,map_size[0]-1);
                walker_pos[1] = random(0,map_size[1]-1);
            } while (!free_tiles[walker_pos[0]][walker_pos[1]]);
        }
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

    for (Enemy_type* e: enemy_types)
    {
        for (int u=1;u<=e->number;u++)
        {
            int pos[2], counter=0;
            do
            {
                pos[0] = map_size[0]/2+(random(0,1)?1:-1)*random(e->min_radius,e->max_radius);
                pos[1] = map_size[1]/2+(random(0,1)?1:-1)*random(e->min_radius,e->max_radius);
                counter++;
                if (counter>=3000) return generate_level(tiles_to_generate);
            } while (!free_tiles[pos[0]][pos[1]]);

            Character* c = new Character(pos[0],pos[1],e->tex,e->blood,e->explosion_radius);
            free_tiles[pos[0]][pos[1]] = false;

            if (e->arms)
            {
                for (int i=1;i<=e->arms;i++)
                {
                    c->add_arm(new Arm(e->blood_per_arm,e->strenght_per_arm,e->arm_name));
                }
            }
        }
    }
}

void set_camera()
{
    camera_pos[0] = player->pos[0]-window[0]/(tile_size*2);
    camera_pos[1] = player->pos[1]-window[1]/(tile_size*2);
}

bool sort_crit(Object* a, Object* b)
{
    return a->background || (!b->background && a->pos[1] < b->pos[1]);
}

void render_everything()
{
    SDL_SetRenderDrawColor(renderer,255,255,255,255);
    SDL_RenderClear(renderer);

    std::sort(objects.begin(), objects.end(), sort_crit);
    for (Object* o: objects) o->render();

    SDL_Rect r = {window[0]-20,window[1]-10,10,100*player->hp/50};
    r.y -= r.h;
    SDL_SetRenderDrawColor(renderer,255,0,0,255);
    SDL_RenderFillRect(renderer,&r);
    r.y += r.h;
    r.h = 100;
    r.y -= r.h;
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderDrawRect(renderer,&r);

    SDL_RenderPresent(renderer);
}

void execute_turn()
{
    for (Character* e: enemies) e->ai();
    for (Item* i: items) i->ai();
    set_camera();
    select_mode = normal;
}

int main(int argc, char* args[])
{
    random_init();
    IMG_Init(IMG_INIT_PNG);

    renderwindow = SDL_CreateWindow("LD 38", 50, 50, window[0], window[1], SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(renderwindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    #include "Enemy_types.cpp"

    generate_level(1609);
    player = new Character(map_size[0]/2,map_size[1]/2,"Player",50,5,true);
    player->add_arm(new Arm(5,5,"Normal Arm"));

    set_camera();

    //SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    SDL_Event e;
	while (!breakk)
    {
        while(SDL_PollEvent(&e))
        {
			if (e.type == SDL_QUIT) breakk = true;

			else if (e.type == SDL_KEYDOWN)
			{
			    int select = 0;
			    if (e.key.keysym.sym == SDLK_ESCAPE) breakk = true;
			    else if (e.key.keysym.sym == SDLK_1) select=0;
			    else if (e.key.keysym.sym == SDLK_2) select=1;
			    else if (e.key.keysym.sym == SDLK_3) select=2;
			    else if (e.key.keysym.sym == SDLK_4) select=3;
			    else if (e.key.keysym.sym == SDLK_5) select=4;
			    else if (e.key.keysym.sym == SDLK_6) select=5;
			    else if (e.key.keysym.sym == SDLK_f) select_mode = (select_mode==equip?normal:equip); //equip
			    else if (e.key.keysym.sym == SDLK_r) select_mode = (select_mode==throwing?normal:throwing); //throwing
			    else if (e.key.keysym.sym == SDLK_e) //prepare
                {
                    if (player->worn[selected_arm] && !player->worn[selected_arm]->over_filled)
                    {
                        Arm* a;
                        if (player->worn[selected_arm]->carrying) a = player->worn[selected_arm]->carrying;
                        else a = player->worn[selected_arm];

                        player->hp -= a->max_hp-a->hp+1;
                        if (player->hp <= 0)
                        {
                            death_screen();
                            break;
                        }
                        a->over_filled = true;
                    }
                }
			    else if (e.key.keysym.sym == SDLK_x) //drop
                {
                    if (player->worn[selected_arm])
                    {
                        auto pos_i = place_item(player->pos[0],player->pos[1]);
                        if (player->worn[selected_arm]->carrying)
                        {
                            Item(pos_i.first, pos_i.second, player->worn[selected_arm]->carrying);
                            player->worn[selected_arm]->carrying = nullptr;
                        }
                        else
                        {
                            Item(pos_i.first, pos_i.second, player->worn[selected_arm]);
                            player->worn[selected_arm] = nullptr;
                        }
                    }
                }
			    else if (e.key.keysym.sym == SDLK_q) //eat
                {
                    if (player->worn[selected_arm] && !player->worn[selected_arm]->over_filled)
                    {
                        if (player->worn[selected_arm]->carrying)
                        {
                            player->hp += player->worn[selected_arm]->carrying->hp;
                            player->worn[selected_arm]->carrying->kill();
                        }
                        else
                        {
                            player->hp += player->worn[selected_arm]->hp;
                            player->worn[selected_arm]->kill();
                        }

                        execute_turn();
                    }
                }
                else if (e.key.keysym.sym == SDLK_c) //switch
                {
                    if (player->worn[selected_arm] && player->worn[selected_arm]->carrying)
                    {
                        Arm* temp = player->worn[selected_arm];
                        player->worn[selected_arm] = player->worn[selected_arm]->carrying;
                        player->worn[selected_arm]->carrying = temp;
                        player->worn[selected_arm]->carrying->carrying = nullptr;

                        player->worn[selected_arm]->carried_by_arm = nullptr;
                        player->worn[selected_arm]->carried_by_char = player;

                        player->worn[selected_arm]->carrying->carried_by_arm = player->worn[selected_arm];
                        player->worn[selected_arm]->carrying->carried_by_char = nullptr;
                    }
                }
			    else //movement & throw & shit
                {
                    int di[2] = {0,0};

                    if (e.key.keysym.sym == SDLK_UP || e.key.keysym.sym == SDLK_w) di[1]--;
                    else if (e.key.keysym.sym == SDLK_DOWN || e.key.keysym.sym == SDLK_s) di[1]++;
                    else if (e.key.keysym.sym == SDLK_LEFT || e.key.keysym.sym == SDLK_a) di[0]--;
                    else if (e.key.keysym.sym == SDLK_RIGHT || e.key.keysym.sym == SDLK_d) di[0]++;
                    else continue;

                    if (select_mode == throwing)
                    {
                        if (player->worn[selected_arm] && player->worn[selected_arm]->carrying)
                        {
                            Item* it = new Item(player->pos[0], player->pos[1], player->worn[selected_arm]->carrying);
                            player->worn[selected_arm]->carrying->carried_by_arm = nullptr;
                            player->worn[selected_arm]->carrying = nullptr;

                            for (int i=0;i<=4;i++)
                            {
                                it->pos[0] += di[0];
                                it->pos[1] += di[1];

                                if (wall_or_void(it->pos[0],it->pos[1]))
                                {
                                    it->pos[0] -= di[0];
                                    it->pos[1] -= di[1];
                                    break;
                                }

                                bool found_enemy = false;
                                for (Character* e: enemies)
                                {
                                    if (e->pos[0] == it->pos[0] && e->pos[1] == it->pos[1])
                                    {
                                        found_enemy = true;
                                        int org_strenght = it->arm->strenght;
                                        it->arm->strenght = (it->arm->strenght+player->worn[selected_arm]->strenght)/2;
                                        e->attack(it->arm);
                                        it->arm->strenght = org_strenght;

                                        if (it->arm->over_filled)
                                        {
                                            explosion(it->pos[0],it->pos[1],true);
                                            it->arm->hp = 0;
                                        }
                                        else
                                        {
                                            it->arm->hp -= throw_durability_drop-weapon_durability_drop;
                                        }

                                        if (it->arm->hp <= 0)
                                        {
                                            it->arm->kill();
                                            it->kill();
                                        }

                                        break;
                                    }
                                }

                                if (found_enemy) break;

                                render_everything();
                            }

                            auto pos_i = place_item(it->pos[0], it->pos[1], it);
                            it->pos[0] = pos_i.second;
                            it->pos[1] = pos_i.first;

                            player->worn[selected_arm]->hp--;
                            if (player->worn[selected_arm]->hp<=0) player->worn[selected_arm]->kill();
                        }
                        else
                        {
                            select_mode = normal;
                            continue;
                        }
                    }
                    else
                    {
                        player->pos[0] += di[0];
                        player->pos[1] += di[1];


                        if (wall_or_void(player->pos[0],player->pos[1]))
                        {
                            player->pos[0] -= di[0];
                            player->pos[1] -= di[1];
                            continue;
                        }

                        bool attacked = false;

                        for (Character* e: enemies)
                        {
                            if (e->pos[0] == player->pos[0] && e->pos[1] == player->pos[1])
                            {
                                attacked = true;
                                player->pos[0] -= di[0];
                                player->pos[1] -= di[1];

                                if (!player->worn[selected_arm] || !player->worn[selected_arm]->over_filled)
                                {
                                    e->attack(player->worn[selected_arm]);
                                }
                                break;
                            }
                        }

                        if (attacked && player->worn[selected_arm] && player->worn[selected_arm]->over_filled) continue;

                        for (Item* it: items)
                        {
                            if (it->pos[0] == player->pos[0] && it->pos[1] == player->pos[1])
                            {
                                if (player->add_arm(it->arm))
                                {
                                    it->arm = nullptr;
                                    it->kill();
                                }
                            }
                        }

                        while(!to_delete.empty())
                        {
                            delete to_delete[0];
                            to_delete.pop_front();
                        }
                    }

                    execute_turn();
                }

                if (select)
                {
                    if (select_mode == normal) selected_arm = select;
                    else if (select_mode == equip && select != selected_arm)
                    {
                        //The swapping itself
                        if (player->worn[select])
                        {
                            Arm* temp = player->worn[select]->carrying;

                            if (player->worn[selected_arm]->carrying)
                            {
                                player->worn[select]->carrying = player->worn[selected_arm]->carrying;
                                player->worn[selected_arm]->carrying = temp;
                            }
                            else
                            {
                                player->worn[select]->carrying = player->worn[selected_arm];
                                player->worn[selected_arm] = temp;
                            }
                        }
                        else
                        {
                            Arm* temp = player->worn[select];

                            if (player->worn[selected_arm]->carrying)
                            {
                                player->worn[select] = player->worn[selected_arm]->carrying;
                                player->worn[selected_arm]->carrying = temp;
                            }
                            else
                            {
                                player->worn[select] = player->worn[selected_arm];
                                player->worn[selected_arm] = temp;
                            }
                        }

                        //correcting backwards references
                        if (player->worn[select])
                        {
                            player->worn[select]->carrying->carried_by_arm = nullptr;
                            player->worn[select]->carrying->carried_by_char = player;

                            if (player->worn[select]->carrying)
                            {
                                player->worn[select]->carrying->carried_by_arm = player->worn[select];
                                player->worn[select]->carrying->carried_by_char = nullptr;
                                player->worn[select]->carrying->carrying = nullptr;
                            }
                        }
                        if (player->worn[selected_arm])
                        {
                            player->worn[selected_arm]->carrying->carried_by_arm = nullptr;
                            player->worn[selected_arm]->carrying->carried_by_char = player;

                            if (player->worn[selected_arm]->carrying)
                            {
                                player->worn[selected_arm]->carrying->carried_by_arm = player->worn[selected_arm];
                                player->worn[selected_arm]->carrying->carried_by_char = nullptr;
                                player->worn[selected_arm]->carrying->carrying = nullptr;
                            }
                        }
                    }
                }
			}
        }

        if (breakk) break;
        for (Object* o: objects) o->update();

        render_everything();

        while(!to_delete.empty())
        {
            delete to_delete[0];
            to_delete.pop_front();
        }

        limit_fps();
    }

    return 0;
}
