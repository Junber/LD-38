// stage 1
{
    Enemy_type* e = new Enemy_type;

    e->arms=0;
    e->blood_per_arm=0;
    e->strenght_per_arm=3;
    e->blood=5;
    e->tex="Armless Degenerate";
    e->number = 3;
    e->min_radius = 1;
    e->max_radius = 10;

    enemy_types.push_back(e);
}
{
    Enemy_type* e = new Enemy_type;

    e->arms=1;
    e->blood_per_arm=5;
    e->strenght_per_arm=5;
    e->blood=10;
    e->tex="One-Armed Bandit";
    e->number = 1;
    e->min_radius = 1;
    e->max_radius = 10;

    enemy_types.push_back(e);
}
// stage 2
{
    Enemy_type* e = new Enemy_type;

    e->arms=0;
    e->blood_per_arm=0;
    e->strenght_per_arm=3;
    e->blood=5;
    e->tex="Armless Degenerate";
    e->number = 5;
    e->min_radius = 11;
    e->max_radius = 20;

    enemy_types.push_back(e);
}
{
    Enemy_type* e = new Enemy_type;

    e->arms=1;
    e->blood_per_arm=5;
    e->strenght_per_arm=5;
    e->blood=10;
    e->tex="One-Armed Bandit";
    e->number = 3;
    e->min_radius = 11;
    e->max_radius = 20;

    enemy_types.push_back(e);
}
{
    Enemy_type* e = new Enemy_type;

    e->arms=2;
    e->blood_per_arm=3;
    e->strenght_per_arm=3;
    e->blood=3;
    e->tex="Weak Guy";
    e->number = 3;
    e->min_radius = 11;
    e->max_radius = 20;

    enemy_types.push_back(e);
}

//stage 3
{
    Enemy_type* e = new Enemy_type;

    e->arms=2;
    e->blood_per_arm=3;
    e->strenght_per_arm=7;
    e->blood=15;
    e->tex="Cripple Crew Soldier";
    e->number = 5;
    e->min_radius = 21;
    e->max_radius = 30;

    enemy_types.push_back(e);
}
{
    Enemy_type* e = new Enemy_type;

    e->arms=2;
    e->blood_per_arm=3;
    e->strenght_per_arm=3;
    e->blood=3;
    e->tex="Weak Guy";
    e->number = 3;
    e->min_radius = 21;
    e->max_radius = 30;

    enemy_types.push_back(e);
}
