// bool
typedef enum {
    false,  // 0
    true,  // 1
} bool;

typedef int fixed_t;

enum {
	type_player_spawn = 1,
	type_player,
	type_key = 8,
	type_platform = 11,
	type_platform_2 = 12,
	type_spring = 18,
	type_chest = 20,
	type_balloon = 22,
	type_fall_floor = 23,
	type_fruit = 26,
	type_fly_fruit = 28,
	type_fake_wall = 64,
	type_message = 86,
	type_big_chest = 96,
	type_big_chest2 = 97,
	type_orb = 103,
	type_flag = 118,
	type_smoke,
	type_lifeup = 130
};

typedef struct
{
	short x, y;
} Coordinate;

typedef struct
{
	fixed_t x, y;
} Coordinate_fixed;

typedef struct
{
	bool x, y;
} Flip;

typedef struct
{
	short x, y, w, h;
} Quad;

typedef struct
{
	bool active;
	short type;
	bool collideable;
	bool solids;
	short spr;
	Flip flip;
	short x;
	short y;
	Quad hitbox;
	Coordinate_fixed spd;
	Coordinate_fixed rem;
} OBJ;

typedef struct
{
	OBJ obj;
	bool p_jump;
	bool p_dash;
	short grace;
	short jbuffer;
	short djump;
	short dash_time;
	short dash_effect_time;
	Coordinate_fixed dash_target;
	Coordinate_fixed dash_accel;
	short spr_off;
	bool was_on_ground;
} PLAYER;

typedef struct
{
	bool active;
	short spr;
	Coordinate target;
	short x,y;
	Coordinate spd;
	short state;
	short delay;
	bool solids;
	Flip flip;
} PLAYER_SPAWN;

typedef struct
{
	fixed_t x, y;
	short size;
} HAIR;

typedef struct
{
	OBJ obj;
	short hide_in;
	short hide_for;
	short delay;
} SPRING;

typedef struct
{
	OBJ obj;
	unsigned short offset;
	fixed_t start;
	short timer;
} BALLOON;

typedef struct
{
	OBJ obj;
	short state;
	bool solid;
	short delay;
} FALL_FLOOR;

typedef struct
{
	OBJ obj;
	unsigned short off;
	short start;
} FRUIT;

typedef struct
{
	OBJ obj;
	short start;
	bool fly;
	unsigned short step;
	short sfx_delay;
} FLY_FRUIT;

typedef struct
{
	OBJ obj;
	short duration;
	short flash;
} LIFEUP;

typedef struct
{
	OBJ obj;
	short start;
	short timer;
} CHEST;

typedef struct
{
	OBJ obj;
	fixed_t last;
	fixed_t offset;
} PLATFORM;

typedef struct
{
	OBJ obj;
	short index;
} MESSAGE;

typedef struct
{
	bool active;
	short x, y;
	short h;
	short spd;
} BIG_CHEST_PARTICLE;

typedef struct
{
	OBJ obj;
	short state;
	short timer;
	BIG_CHEST_PARTICLE particles[50];
} BIG_CHEST;

typedef struct
{
	OBJ obj;
} ORB;

typedef struct
{
	short x, y;
	short s;
	fixed_t spd;
	unsigned short off;
	short c;
} PARTICLE;

typedef struct
{
	short x, y;
	short s, c;
	short spd;
} CLOUD;

typedef struct
{
	OBJ obj;
	short score;
	bool show;
} FLAG;

typedef struct
{
	bool active;
	short delay;
} ROOM_TITLE;

typedef struct
{
	bool active;
	fixed_t x, y;
	short t;
	Coordinate_fixed spd;
} DEAD_PARTICLE;

typedef struct
{
	OBJ obj;
	short timer;
} SMOKE;

void celeste_update();
void celeste_draw();

void player_spawn_init(PLAYER_SPAWN* this, short x, short y);

void create_hair(short x, short y);
void set_hair_color(short djump);
void draw_hair(short x, short y, short flip);

void init_object(OBJ* obj, short type, short x, short y);
void spring_init(SPRING* this, short x, short y);
void balloon_init(BALLOON* this, short x, short y);
void fall_floor_init(FALL_FLOOR* this, short x, short y);
void smoke_init(short x, short y);
void lifeup_init(LIFEUP* this, short x, short y);
void fruit_init(FRUIT* this, short x, short y);
void fly_fruit_init(FLY_FRUIT* this, short x, short y);
void fake_wall_init(OBJ* this, short x, short y);
void key_init(OBJ* this, short x, short y);
void chest_init(CHEST* this, short x, short y);
void platform_init(PLATFORM* this, short x, short y, short dir);
void message_init(MESSAGE* this, short x, short y);
void big_chest_init(BIG_CHEST* this, short x, short y);
void orb_init(ORB* this, short x, short y);
void flag_init(FLAG* this, short x, short y);
void room_title_init(ROOM_TITLE* this);
void clouds_init();

void break_fall_floor(FALL_FLOOR* this);

void fill_palette(short c);
void reset_palette();
void fill_background_color(short c);
void camera(int x, int y);
void new_bg(CLOUD* cloud, bool set);
void spr_reset();
void spr_not_flip(short n, short x, short y);
void spr(short n, short x, short y, bool flip_x, bool flip_y);
void spr_double(short n, short x, short y);
void spr_with_palette(short n, short x, short y, short c, bool flip_x, bool flip_y);
void print_with_spr(char const* str, short x, short y, short c);
void draw_time(short x, short y);

const unsigned char FLAGS_DATA[256] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x02, 0x02, 0x00, 0x00, 0x00, 
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x02, 0x02, 0x02, 0x02, 0x02, 
	0x03, 0x03, 0x13, 0x13, 0x13, 0x13, 0x02, 0x02, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x02, 
	0x03, 0x03, 0x13, 0x13, 0x13, 0x13, 0x02, 0x02, 0x04, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
	0x00, 0x00, 0x13, 0x13, 0x13, 0x13, 0x00, 0x04, 0x04, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
	0x00, 0x00, 0x13, 0x13, 0x13, 0x13, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
