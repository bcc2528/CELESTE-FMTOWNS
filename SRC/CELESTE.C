// Celeste for FM TOWNS

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <io.h>
#include <conio.h>
#include <fmcfrb.h>
#include <egb.h>
#include <spr.h>
#include <snd.h>
#include <mos.h>
#include <msvdrv.h>

#include "fixed.h"
#include "celeste.h"
#include "sine.h"

char work[1536];
char snd_work[16384];
char msvwork[MSVWorkSize];

short sprite_number = 0;

//-- object lists --
//------------------

CHEST chest = { 0 };
OBJ fake_wall = { 0 };
FLY_FRUIT fly_fruit = { 0 };
FRUIT fruit = { 0 };
OBJ key = { 0 };

PLAYER player = { 0 };
PLAYER_SPAWN player_spawn = { 0 };
short player_color;
LIFEUP lifeup = { 0 };
MESSAGE message = { 0 };
BIG_CHEST big_chest = { 0 };
ORB orb = { 0 };
FLAG flag = { 0 };
ROOM_TITLE room_title = { 0 };

#define MAX_BALLOONS 6
BALLOON balloons[MAX_BALLOONS] = { 0 };

#define MAX_DEAD_PARTICLES 8
DEAD_PARTICLE dead_particles[MAX_DEAD_PARTICLES] = { 0 };

#define MAX_FALL_FLOORS 12
FALL_FLOOR fall_floors[MAX_FALL_FLOORS] = { 0 };

#define MAX_HAIR 5
HAIR hair[MAX_HAIR] = { 0 };

#define MAX_PARTICLES 24
PARTICLE particles[MAX_PARTICLES] = { 0 };

#define MAX_PLATFORMS 10
PLATFORM platforms[MAX_PLATFORMS] = { 0 };

#define MAX_SMOKE 10
SMOKE smoke[MAX_SMOKE] = { 0 };

#define MAX_SPRINGS 5
SPRING springs[MAX_SPRINGS] = { 0 };

#define MAX_CLOUDS 16
CLOUD clouds[MAX_CLOUDS] = { 0 };

#define MAX_FRUIT 32
bool got_fruit[MAX_FRUIT] = { false };


// -- globals --
// -------------
bool set_spr_room;

Coordinate room = { 0, 0 };
short room_tiles[16][16];
short freeze = 0;
short shake = 0;
bool cheated = false;
bool can_shake = true;
bool will_restart = false;
short delay_restart = 0;
bool has_dashed = false;
bool has_key = false;
bool pause_player = false;
bool flash_bg = false;
int music_timer = 0;

bool game_pause = false;
bool game_pause_was = false;
bool can_pause = false;
short frames = 0;
short seconds = 0;
short minutes = 0;
short deaths = 0;
short max_djump = 1;
bool start_game = false;
bool cloud_scroll = false;
short start_game_flash = 0;
short MAP_DATA[8192];
short base_palette[16];

#define k_left KEY_LEFT
#define k_right KEY_RIGHT
#define k_up KEY_UP
#define k_down KEY_DOWN
#define k_jump KEY_A
#define k_dash KEY_B
#define k_pause KEY_RUN

bool up_now;
bool down_now;
bool left_now;
bool right_now; 
bool jump_now;
bool dash_now;



// -- helper functions --
// ----------------------

#define rnd(a) (rand() % (a))
#define maybe() (rnd(2) < 1)
//#define max(a,b) ((a) > (b) ? (a) : (b))
//#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) (_max((a),(b)))
#define min(a,b) (_min((a),(b)))
#define clamp(val,a,b) (max((a), min((b), (val))))
#define appr(val,target,amount) (((val) > (target))? max((val) - (amount), (target)): min((val) + (amount), (target)))
#define sign(v) (((v) > 0) - ((v) < 0))
#define flr(a) (((a) & 0xffff0000))
#define fget(n,f) ((FLAGS_DATA[(n)] >> (f)) & 1)
#define tile_at(x,y) (room_tiles[(x)][(y)])

#define sin(a) (SIN[(((a)*360) >> 16) % 360])
#define cos(a) (COS[(((a)*360) >> 16) % 360])

#define solid_at(x,y,w,h) (tile_flag_at((x),(y),(w),(h),0))
#define ice_at(x,y,w,h) (tile_flag_at((x),(y),(w),(h),4))

bool tile_flag_at(short x,short y,short w,short h,short flag)
{
	short i, j, i_comp, j_comp;

	i_comp = min(15, (x+w-1) / 8);
	j_comp = min(15, (y+h-1) / 8);

	for (i = max(0, x / 8); i <= i_comp; i++)
	{
		for (j = max(0, y / 8); j <= j_comp; j++)
		{
			if (fget(tile_at(i,j),flag))
			{
				return true;
			}
		}
	}

	return false;
}

bool spikes_at(short x, short y, short w, short h, fixed_t xspd, fixed_t yspd)
{
	short i, j, i_comp, j_comp;

	i_comp = min(15, (x+w-1) / 8);
	j_comp = min(15, (y+h-1) / 8);

	for (i = max(0, x / 8); i <= i_comp; i++)
	{
		for (j = max(0, y / 8); j <= j_comp; j++)
		{
			short tile = tile_at(i,j);

			if (tile == 17 || tile == 27 || tile == 43 || tile == 59)
			{
				if (tile == 17 && ((y+h-1)%8>=6 || y+h == j*8+8) && yspd >= 0) 
				{
					return true;
				}
				else if (tile == 27 && y%8 <= 2 && yspd <= 0)
				{
					return true;
				}
				else if (tile == 43 && x%8 <= 2 && xspd <= 0)
				{
					return true;
				}
				else if (tile == 59 && ((x+w-1)%8 >= 6 || x+w == i*8+8) && xspd >= 0) 
				{
					return true;
				}
			}
		}
	}

	return false;
}


// -- sfx & music functions --
// ---------------------------
char *music_data[6];

void music_init()
{
	FILE *fp;
	int  mlot;
	long len;

	fp = fopen("no_music.msv", "rb");
	mlot = MSV_getLot();
	len = _filelength(_fileno(fp));
	music_data[0] = (char *)MSV_mallocMemory(mlot,len);
	fread(music_data[0], len, 1,fp);
	fclose(fp);

	fp = fopen("music00.msv", "rb");
	mlot = MSV_getLot();
	len = _filelength(_fileno(fp));
	music_data[1] = (char *)MSV_mallocMemory(mlot,len);
	fread(music_data[1], len, 1,fp);
	fclose(fp);

	fp = fopen("music10.msv", "rb");
	mlot = MSV_getLot();
	len = _filelength(_fileno(fp));
	music_data[2] = (char *)MSV_mallocMemory(mlot, len);
	fread(music_data[2], len, 1, fp);
	fclose(fp);

	fp = fopen("music20.msv", "rb");
	mlot = MSV_getLot();
	len = _filelength(_fileno(fp));
	music_data[3] = (char *)MSV_mallocMemory(mlot, len);
	fread(music_data[3], len, 1, fp);
	fclose(fp);

	fp = fopen("music30.msv", "rb");
	mlot = MSV_getLot();
	len = _filelength(_fileno(fp));
	music_data[4]=(char *)MSV_mallocMemory(mlot, len);
	fread(music_data[4], len, 1, fp);
	fclose(fp);

	fp = fopen("music40.msv", "rb");
	mlot = MSV_getLot();
	len = _filelength(_fileno(fp));
	music_data[5] = (char *)MSV_mallocMemory(mlot, len);
	fread(music_data[5],len,1,fp);
	fclose(fp);
}

void music(short n, short feedms, short chmask)
{
	MSV_play_stop();

	switch(n)
	{
		case 0:
			MSV_set_memimage(music_data[1],msvwork);
			break;
		case 10:
			MSV_set_memimage(music_data[2],msvwork);
			break;
		case 20:
			MSV_set_memimage(music_data[3],msvwork);
			break;
		case 30:
			MSV_set_memimage(music_data[4],msvwork);
			break;
		case 40:
			MSV_set_memimage(music_data[5],msvwork);
			break;
		default:
			MSV_set_memimage(music_data[0],msvwork);
			break;
	}
	
	MSV_play_start();
}

void sfx(int n, int ch)
{
	int steptime,datalen;
	static char msv[3][256];

	MSVC_set_playcount(MSVWK->playcount);

	switch(n)
	{
		case 0:
			MSVC_line_compile("t225 l64 @4v15 o6f+ <<@5b >@4b <@5f >@4f+ <<@5b >>@4d+ <<@5g >@4b <@5f >@4f+ <@5d v13>@4d v11<@5c v9@4a+ <v7@5g+ >v5@4f+ <v3@5f*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 1:
			MSVC_line_compile("t225 @1v15l64 o3fg>d>c*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 2:
			MSVC_line_compile("t150 @1v15l64 o3c+ea+>a+*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 3:
			MSVC_line_compile("t225 @5v3l64 o2f+g+ab>>v9a+>v11f+>>@7c<bbaf+d<aev9c<afd<v7a+fd<v3bgv1fd+*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 4:
			MSVC_line_compile("t225 @1v15l32 o3d+>f+<f+>a+<b>>d<v13d+>g+<v11a>>c+<v9d+>f+<v7g>a+v5c>d<v3f*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 5:
			MSVC_line_compile("t150 @8v15l64 o2aav13av11av9g+v7gv5f+v3f*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 6:
			MSVC_line_compile("t150 @2v15l64 o5c<<d>>a<<a+>>>e<<v13g+>>b<v11e>>v9d+<<v5bv3e<f<e<d+*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 7:
			MSVC_line_compile("t225 @2v3l64 o3ef+g+a+>v5dg+>v7d>v9dd*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 8:
			MSVC_line_compile("t150 @1v15l64 o2ga+>dea+>a+>bv13bg+v11g+bv9bg+v7g+v5bv3b*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 9:
			MSVC_line_compile("t150 @2v3l64 o2fv7g@7v9d+d+v7d+v5d+v3d+d+*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 13:
			MSVC_line_compile("t225 @6v11l32 o3c>v13e<v15e>bc+>g+<a>>g<e>b<g+>>v13d<v11c+>v9d<v7c+>d<v5c+>d+<c+>d+<v3c+>d+<c+>d+<c+>d+<c+*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 14:
			MSVC_line_compile("t225 @8v9l32 o5bv13gv15d<f<aa>c+v11e<v7b*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 15:
			MSVC_line_compile("t150 @7v9l64 o2a>v11d<f+a+>c+<f>f<g>c<ea>v9f<v7g+>v3c+*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 16:
			MSVC_line_compile("t225 @4v15l8 o4gc>d+*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 23:
			MSVC_line_compile("t150 @8v15l32 o4co4ffv13fv11fv9fv7fv5fv3f*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 35:
			MSVC_line_compile("t225 @7v5l32 o6d+*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 37:
			MSVC_line_compile("t225 @4v11l8 o3cv9cv7cv5cv11d+v9d+v7d+v5d+>v11cv9c<v11gv9g>v11c<gv9a+>v13fv15a+a+v13a+v11a+v9a+v5a+*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 38:
			MSVC_line_compile("t150 @3v15l16 o4cg>c<v13cg>c<v11cg>c<v9cg>c<v7cg>c<v5cg>c<v3cg>c*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 50:
			MSVC_line_compile("t225 @6v3l16 o4ggv5ggv7ggv9ggv11ggv13ggv15grgrv13grgrv11grgrv9grv7grv5grv3g*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 51:
			MSVC_line_compile("t180 @8v7l32 o2d+fv9g>cv11g>v13d+> @4v15c>c<@6d+a+ @4c>c<@6d+a+ @4v13c>c<@6d+a+ @4v11c>c<@6d+a+ @4v9c>c<@6d+a+ @4v7c>c<@6v5d+a+  @4v3c>c*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 54:
			MSVC_line_compile("t150 @4v7l64 o4g>g<@6a+>f<@4v5g>g<@6a+>f<@4v3g>g<@6a+>f*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		case 55:
			MSVC_line_compile("t163 @4v11l16 o5frv9f>v15cv13cv11c*", msv[ch], &steptime, &datalen, 0, 3);
			break;

		default:
			MSVC_line_compile("t120 @1v1l8 *", msv[ch], &steptime, &datalen, 0, 3);
			break;
	}

	MSV_partstop(ch + 3);
	MSV_partplay(ch + 3, msv[ch]);
}



// -- room functions --
// --------------------

int level_index()
{
	return room.x % 8 + room.y * 8;
}

bool is_title()
{
	if (level_index() == 31)
	{
		return true;
	}

	return false;
}

void restart_room()
{
	will_restart = true;
	delay_restart = 15;
}

void load_room(short x, short y)
{
	short tx, ty;
	int i;
	int platforms_num, springs_num, fall_floors_num, balloon_num;

	BALLOON* balloons_temp;
	FALL_FLOOR* fall_floors_temp;
	PLATFORM* platforms_temp;
	SPRING* springs_temp;
	SMOKE* smoke_temp;

	set_spr_room = false;

	has_dashed = false;
	has_key = false;

	//remove existing objects
	chest.obj.active = false;
	fake_wall.active = false;
	fly_fruit.obj.active = false;
	fruit.obj.active = false;
	key.active = false;

	player.obj.active = false;
	player_spawn.active = false;
	lifeup.obj.active = false;
	message.obj.active = false;
	big_chest.obj.active = false;
	orb.obj.active = false;
	flag.obj.active = false;

	for(i=0;i < MAX_BALLOONS;i++)
	{
		balloons_temp = &(balloons[i]);
		balloons_temp->obj.active = false;
	}
	for(i=0;i < MAX_FALL_FLOORS;i++)
	{
		fall_floors_temp = &(fall_floors[i]);
		fall_floors_temp->obj.active = false;
	}
	for(i=0;i < MAX_PLATFORMS;i++)
	{
		platforms_temp = &(platforms[i]);
		platforms_temp->obj.active = false;
	}
	for(i=0;i < MAX_SPRINGS;i++)
	{
		springs_temp = &(springs[i]);
		springs_temp->obj.active = false;
	}
	for(i=0;i < MAX_SMOKE;i++)
	{
		smoke_temp = &(smoke[i]);
		smoke_temp->obj.active = false;
	}

	platforms_num = 0;
	springs_num = 0;
	fall_floors_num = 0;
	balloon_num = 0;

	//--current room
	room.x = x;
	room.y = y;

	i = level_index() * 16 * 16;

	for (ty = 0; ty < 16; ty++)
	{
		for (tx = 0; tx < 16; tx++)
		{
			short type = MAP_DATA[i];
			room_tiles[tx][ty] = 0;

			switch (type)
			{
				case type_player_spawn:
					player_spawn_init(&(player_spawn), tx * 8, ty * 8);
					break;

				case type_player:
					break;

				case type_platform:
					platforms_temp = &(platforms[platforms_num++]);
					platform_init(platforms_temp, tx * 8, ty * 8, -1);
					break;

				case type_platform_2:
					platforms_temp = &(platforms[platforms_num++]);
					platform_init(platforms_temp, tx * 8, ty * 8, 1);
					break;

				case type_key:
					if (!got_fruit[1 + level_index()])
					{
						key_init(&(key), tx * 8, ty * 8);
					}
					break;

				case type_spring:
					springs_temp = &(springs[springs_num++]);
					spring_init(springs_temp, tx * 8, ty * 8);
					break;

				case type_chest:
					chest_init(&(chest), tx * 8, ty * 8);
					break;

				case type_balloon:
					balloons_temp = &(balloons[balloon_num++]);
					balloon_init(balloons_temp, tx * 8, ty * 8);
					break;

				case type_fall_floor:
					fall_floors_temp = &(fall_floors[fall_floors_num++]);
					fall_floor_init(fall_floors_temp, tx * 8, ty * 8);
					room_tiles[tx][ty] = type;
					break;

				case type_fruit:
					fruit_init(&(fruit), tx * 8, ty * 8);
					break;

				case type_fly_fruit:
					fly_fruit_init(&(fly_fruit), tx * 8, ty * 8);
					break;

				case type_fake_wall:
					fake_wall_init(&(fake_wall), tx * 8, ty * 8);
					break;

				case type_message:
					message_init(&(message), tx * 8, ty * 8);
					room_tiles[tx][ty] = type;
					break;

				case type_big_chest:
					big_chest_init(&(big_chest), tx * 8, ty * 8);
					break;

				case type_big_chest2:
					break;

				case type_flag:
					flag_init(&(flag), tx * 8, ty * 8);
					break;

				case type_smoke:
					break;

				/*case type_orb:
					orb_init(&(orb), tx * 8, ty * 8);
					break;*/

				case type_lifeup:
					lifeup_init(&(lifeup), tx * 8, ty * 8);
					break;

				default:
					room_tiles[tx][ty] = type;
					break;

			}

		i++;
		}
	}

	if(!is_title())
	{
		can_pause = false;
		room_title_init(&room_title);
	}

}

void next_room()
{
	if (room.x == 2 && room.y == 1) {
		music(30, 500, 7);
	}
	else if (room.x == 3 && room.y == 1) {
		music(20, 500, 7);
	}
	else if (room.x == 4 && room.y == 2) {
		music(30, 500, 7);
	}
	else if (room.x == 5 && room.y == 3) {
		music(30, 500, 7);
	}

	if (room.x == 7)
		load_room(0, room.y + 1);
	else
		load_room(room.x + 1, room.y);
}



// -- entry point --
// -----------------

void celeste_init()
{
	int i;
	FILE *fp_map;
	FILE *fp_spr;
	FILE *fp_pal;
	short SP_PAT[16384];
	short PAL[256];

	srand(0x59f47fd3);

	//Map data load(from celeste.map)
	fp_map = fopen("CELESTE.MAP", "rb");
	fread(MAP_DATA, sizeof(short), 8192, fp_map);
	fclose(fp_map);

	SND_init(snd_work);
	MSV_init(msvwork,MSVWorkSize,EXPMODE);
	SND_elevol_mute(0x33);
	music_init();
	MSV_fmb_load("pico8.fmb", msvwork);

	EGB_init(work, 1536);

	EGB_resolution(work, 1, 5);
	EGB_resolution(work, 0, 5);

	EGB_writePage(work, 1);
	EGB_displayStart(work, 0, 128, 24);
	EGB_displayStart(work, 2, 3, 3);
	EGB_displayStart(work, 3, 128, 128);
	EGB_writePage(work, 0);
	EGB_displayStart(work, 0, 128, 24);
	EGB_displayStart(work, 2, 16, 16);
	EGB_displayStart(work, 3, 24, 24);
	EGB_color(work, 1, 0x8000);
	EGB_partClearScreen(work);
	EGB_color(work, 0, 0x7fff);
	EGB_displayPage(work, 1, 3);
	//EGB_writePage(work, 0);

	SPR_init();

	fp_spr = fopen("CELESTE.PAT", "rb");
	fread(SP_PAT, sizeof(short), 16384, fp_spr);
	fclose(fp_spr);

	for(i = 0; i < 256; i++)
	{
		SPR_define(0, 128 + i, 1, 1, (char *)SP_PAT + (i * 128));
	}

	fp_pal = fopen("CELESTE.CTB", "rb");
	fread(PAL, sizeof(short), 128, fp_pal);
	fclose(fp_pal);

	for(i = 0; i < 8; i++)
	{
		SPR_setPaletteBlock(256 + i, 1, (char *)PAL + (i * 32));
	}

	for(i = 0; i < 16; i++)
	{
		base_palette[i] = PAL[i];
	}


}

void title_screen()
{
	short i;

	cloud_scroll = false;

	clouds_init();

	fill_background_color(0);

	for (i = 0; i < 32; i++)
	{
		got_fruit[i] = false;
	}

	can_pause = false;
	frames = 0;
	deaths = 0;
	max_djump = 1;
	start_game = false;
	start_game_flash = 0;
	music(40, 0, 7);

	load_room(7, 3);
}

void begin_game()
{
	frames = 0;
	seconds = 0;
	minutes = 0;
	music_timer = 0;
	start_game = false;
	cloud_scroll = true;
	music(0, 0, 7);
	load_room(0, 0);
	spr_reset();
	reset_palette();
}

// --object functions--
// -----------------------

void init_object(OBJ* obj, short type, short x, short y)
{
	obj->active = true;
	obj->type = type;
	obj->collideable = true;
	obj->solids = true;
	obj->spr = type;
	obj->flip.x = false;
	obj->flip.y = false;
	obj->x = x;
	obj->y = y;
	obj->hitbox.x = 0;
	obj->hitbox.y = 0;
	obj->hitbox.w = 0;
	obj->hitbox.h = 0;
	obj->spd.x = 0;
	obj->spd.y = 0;
	obj->rem.x = 0;
	obj->rem.y = 0;
}

void destroy_object(OBJ* this)
{
	this->active = false;
}

bool obj_collide(OBJ* obj, OBJ* obj2, short ox, short oy)
{
	if(obj->active == false)
	{
		return false;
	}
	else if(obj2 != NULL && obj2->active)
	{
		if(!obj2->collideable ||
			obj2->x + obj2->hitbox.x > obj->x + obj->hitbox.x + obj->hitbox.w + ox ||
			obj2->x + obj2->hitbox.x + obj2->hitbox.w <= obj->x + obj->hitbox.x + ox ||
			obj2->y + obj2->hitbox.y > obj->y + obj->hitbox.y + obj->hitbox.h + oy ||
			obj2->y + obj2->hitbox.y + obj2->hitbox.h <= obj->y + obj->hitbox.y + oy)
		{ }
		else
		{
			return true;
		}
	}
	
	return false;
}

bool platform_check(short ox, short oy)
{
	PLATFORM* platform_temp;

	for(int i = 0; i < MAX_PLATFORMS;i++)
	{
		platform_temp = &(platforms[i]);

		if(!platform_temp->obj.active)
		{
			return false;
		}
		
		if(obj_collide(&(platform_temp->obj), &(player.obj), ox, 0) && obj_collide(&(platform_temp->obj), &(player.obj), ox, oy))
		{
			return true;
		}
	}

	return false;
}

bool obj_is_solid(OBJ* obj, short ox, short oy)
{
	if(oy > 0 && platform_check(ox, oy))
	{
		return true;
	}

	return solid_at(obj->x + obj->hitbox.x + ox, obj->y + obj->hitbox.y + oy, obj->hitbox.w, obj->hitbox.h)
	|| obj_collide(obj, &(fake_wall), ox, oy);
}

bool obj_is_ice(OBJ* obj, short ox, short oy)
{
	return ice_at(obj->x + obj->hitbox.x + ox, obj->y + obj->hitbox.y + oy, obj->hitbox.w, obj->hitbox.h);
}

void obj_move_x(OBJ* obj, short amount, short start)
{
	if (obj->solids)
	{
		short step = sign(amount);
		amount = abs(amount);

		for (short i = start; i <= amount; i++)
		{
			if (!obj_is_solid(obj, step, 0))
			{
				obj->x += step;
			}
			else
			{
				obj->spd.x = 0;
				obj->rem.x = 0;
				break;
			}
		}

	}
	else
	{
		obj->x += amount;
	}

}

void obj_move_y(OBJ* obj, short amount)
{
	if (obj->solids)
	{
		short step = sign(amount);
		amount = abs(amount);

		for (short i = 0; i <= amount; i++)
		{
			if (!obj_is_solid(obj, 0, step))
			{
				obj->y +=  step;
			}
			else
			{
				obj->spd.y = 0;
				obj->rem.y = 0;
				break;
			}
		}
	}
	else
	{
		obj->y +=  amount;
	}
}

void obj_move(OBJ* obj, fixed_t ox, fixed_t oy)
{
	fixed_t amount;

	// -- [x] get move amount
	obj->rem.x += ox;
	amount = flr(obj->rem.x + 32768); //32768 mean 0.5 of fixed
	obj->rem.x -= amount;
	obj_move_x(obj, amount >> 16, 0);

	// -- [y] get move amount
	obj->rem.y += oy;
	amount = flr(obj->rem.y + 32768); //32768 mean 0.5 of fixed
	obj->rem.y -= amount;
	obj_move_y(obj, amount >> 16);

}



// -- player entity --
// -------------------

void player_init(PLAYER* this, short x, short y)
{
	init_object(&(this->obj), type_player, x, y);

	this->obj.active = true;
	this->p_jump = false;
	this->p_dash = false;
	this->grace = 0;
	this->jbuffer = 0;
	this->djump = max_djump;
	this->dash_time = 0;
	this->dash_effect_time = 0;
	this->dash_target.x = 0;
	this->dash_target.y = 0;
	this->dash_accel.x = 0;
	this->dash_accel.y = 0;
	this->obj.hitbox.x = 1;
	this->obj.hitbox.y = 3;
	this->obj.hitbox.w = 6;
	this->obj.hitbox.h = 5;
	this->spr_off = 0;
	this->was_on_ground = false;
	player_color = 0;
	create_hair(this->obj.x, this->obj.y);
}

void kill_player(PLAYER* this)
{
	sfx(0, 1);
	deaths++;
	shake = 10;
	fixed_t angle = 0;

	for(int dir = 0; dir <= 7;dir++)
	{
		DEAD_PARTICLE* particle = &(dead_particles[dir]);

		particle->active =true;
		particle->x = this->obj.x + 4;
		particle->y = this->obj.y + 4;
		particle->t = 10;
		particle->spd.x = sin(angle) * 3;
		particle->spd.y = cos(angle) * 3;

		angle += 8192;
	}

	restart_room();
	this->obj.active = false;
}

void player_update(PLAYER* this)
{
	if (pause_player)
	{
		return;
	}

	short input;
	input = (short) right_now - left_now;

	// --spikes collide
	if (spikes_at(this->obj.x + this->obj.hitbox.x, this->obj.y + this->obj.hitbox.y, this->obj.hitbox.w, this->obj.hitbox.h, this->obj.spd.x, this->obj.spd.y))
	{
		kill_player(this);
	}

	// bottom death
	if (this->obj.y > 128)
	{
		kill_player(this);
	}

	bool on_ground = obj_is_solid(&(this->obj), 0, 1);
	bool on_ice = obj_is_ice(&(this->obj), 0, 1);

	// --smoke particles
	if (on_ground && !this->was_on_ground)
	{
		smoke_init(this->obj.x, this->obj.y + 4);
	}

	bool jump = jump_now && !this->p_jump;
	this->p_jump = jump_now;
	if (jump)
	{
		this->jbuffer = 4;
	}
	else if (this->jbuffer > 0)
	{
		this->jbuffer -= 1;
	}

	bool dash = dash_now && !this->p_dash;
	this->p_dash = dash_now;

	if (on_ground)
	{
		this->grace = 6;
		if (this->djump < max_djump)
		{
			sfx(54, 0);
			this->djump = max_djump;
		}
	}
	else if (this->grace > 0)
	{
		this->grace -= 1;
	}

	if (this->dash_effect_time > 0)
	{
		this->dash_effect_time -= 1;
	}

	if (this->dash_time > 0)
	{
		smoke_init(this->obj.x, this->obj.y);
		this->dash_time -= 1;
		this->obj.spd.x = appr(this->obj.spd.x, this->dash_target.x, this->dash_accel.x);
		this->obj.spd.y = appr(this->obj.spd.y, this->dash_target.y, this->dash_accel.y);
	}
	else
	{
		//-- move
		#define maxrun 65536   // 65536 mean 1 of fixed
		fixed_t accel = 39322; // 39322 mean 0.6 of fixed
		#define deccel 9830    // 9830 mean 0.15 of fixed

		if (!on_ground)
		{
			accel = 26214; // 0.4
		}
		else if (on_ice)
		{
			accel = 3277; // 0.05
			if (input == ((this->obj.flip.x) ? -1 : 1))
			{
				accel = 3277; // 0.05
			}
		}

		if (abs(this->obj.spd.x) > maxrun)
		{
			this->obj.spd.x = appr(this->obj.spd.x, sign(this->obj.spd.x) * maxrun, deccel);
		}
		else
		{
			this->obj.spd.x = appr(this->obj.spd.x, input * maxrun, accel);
		}

		//--facing
		if (this->obj.spd.x != 0)
		{
			this->obj.flip.x = (this->obj.spd.x < 0);
		}

		//-- gravity
		fixed_t maxfall = 131072; // 2
		fixed_t gravity = 13763; // 0.21

		if (abs(this->obj.spd.y) <= 9860) //9860 mean 0.15 of fixed
		{
			gravity *= 0.5;
		}

		//-- wall slide
		if (input != 0 && obj_is_solid(&(this->obj), input, 0) && !obj_is_ice(&(this->obj), input, 0))
		{
			maxfall = 26214; //0.4
			if (rnd(10) < 2)
			{
				smoke_init(this->obj.x + input * 6, this->obj.y);
			}
		}

		if (!on_ground)
		{
			this->obj.spd.y = appr(this->obj.spd.y, maxfall, gravity);
		}

		//-- jump
		if (this->jbuffer > 0)
		{
			if (this->grace > 0)
			{
				//-- normal jump
				sfx(1, 0);
				this->jbuffer = 0;
				this->grace = 0;
				this->obj.spd.y = -131072; //-2
				smoke_init(this->obj.x, this->obj.y + 4);
			}
			else
			{
				//-- wall jump
				short wall_dir = obj_is_solid(&(this->obj), 3, 0) - obj_is_solid(&(this->obj), -3, 0);
				if (wall_dir != 0)
				{
					sfx(2, 0);
					this->jbuffer = 0;
					this->obj.spd.y = -131072; //-2
					this->obj.spd.x = -wall_dir * (maxrun + 65536);
					if (!obj_is_ice(&(this->obj), wall_dir * 3, 0))
					{
						smoke_init(this->obj.x + wall_dir * 6, this->obj.y);
					}
				}
			}
		}

		//-- dash
		fixed_t d_full = 327680;
		fixed_t d_half = FixedMul (d_full , 46341); //0.7071

		if (this->djump > 0 && dash)
		{
			smoke_init(this->obj.x, this->obj.y);
			this->djump -= 1;
			this->dash_time = 4;
			has_dashed = true;
			this->dash_effect_time = 10;
			short v_input;
			v_input = (short) down_now - up_now;
			if (input != 0)
			{
				if (v_input != 0)
				{
					this->obj.spd.x = input * d_half;
					this->obj.spd.y = v_input * d_half;
				}
				else
				{
					this->obj.spd.x = input * d_full;
					this->obj.spd.y = 0;
				}
			}
			else
			{
				if (v_input != 0)
				{
					this->obj.spd.x = 0;
					this->obj.spd.y = v_input * d_full;
				}
				else
				{
					this->obj.spd.x = ((this->obj.flip.x) ? -65536 : 65536);
					this->obj.spd.y = 0;
				}
			}

			sfx(3, 0);
			freeze = 2;
			shake = 6;
			this->dash_target.x = 131072 * sign(this->obj.spd.x);
			this->dash_target.y = 131072 * sign(this->obj.spd.y);
			this->dash_accel.x = 98304; // 1.5
			this->dash_accel.y = 98304; // 1.5

			if (this->obj.spd.y < 0)
			{
				this->dash_target.y = FixedMul(this->dash_target.y, 49152); // 49152 mean 0.85
			}

			if (this->obj.spd.y != 0)
			{
				this->dash_accel.x = FixedMul(this->dash_accel.x, 46341); // 46341 mean 0.7071
			}
			if (this->obj.spd.x != 0)
			{
				this->dash_accel.y = FixedMul(this->dash_accel.y, 46341); // 46341 mean 1.4142
			}
		}
		else if (dash && this->djump <= 0)
		{
			sfx(9, 0);
			smoke_init(this->obj.x, this->obj.y);
		}
	}

	//-- animation
	if (frames % 4 == 0)
	{
		this->spr_off += 1;
	}

	if (!on_ground)
	{
		if (obj_is_solid(&(this->obj), input, 0))
		{
			this->obj.spr = 5;
		}
		else
		{
			this->obj.spr = 3;
		}
	}
	else if (down_now)
	{
		this->obj.spr = 6;
	}
	else if (up_now)
	{
		this->obj.spr = 7;
	}
	else if (this->obj.spd.x == 0 || (!left_now && !right_now))
	{
		this->obj.spr = 1;
	}
	else
	{
		this->obj.spr = 1 + this->spr_off % 4;
	}

	//-- next level
	if (this->obj.y < -4 && level_index() < 30)
	{
		next_room();
	}

	//-- was on the ground
	this->was_on_ground = on_ground;

}

void player_draw(PLAYER* this)
{
	if (this->obj.active == false)
	{
		return;
	}

	// --clamp in screen
	if (this->obj.x < -1 || this->obj.x > 121)
	{
		this->obj.x = clamp(this->obj.x, -1, 121);
		this->obj.spd.x = 0;
	}

	set_hair_color(this->djump);
	spr_with_palette(this->obj.spr, this->obj.x, this->obj.y, player_color, this->obj.flip.x, this->obj.flip.y);
	draw_hair(this->obj.x, this->obj.y, (this->obj.flip.x) ? -1 : 1);
}

void player_spawn_init(PLAYER_SPAWN* this, short x, short y)
{
	sfx(4, 0);
	this->active = true;
	this->x = x;
	this->y = y;
	this->spr = 3;
	this->target.x = this->x;
	this->target.y = this->y;
	this->y = 128;
	this->spd.y = -8;
	this->state = 0;
	this->delay = 0;
	this->solids = false;
	this->flip.x = false;
	this->flip.y = false;
	create_hair(this->x, this->y);
}

void player_spawn_update(PLAYER_SPAWN* this)
{
	//-- jumping up
	if (this->state == 0)
	{
		this->y += this->spd.y;
		if (this->y < this->target.y + 16)
		{
			this->state = 1;
			this->delay = 3;
		}
	}
	//-- falling
	else if (this->state == 1)
	{
		this->spd.y += 1;
		this->y += this->spd.y;
		if (this->spd.y > 0 && this->delay > 0)
		{
			this->spd.y = 0;
			this->delay -= 1;
		}
		if (this->spd.y > 0 && this->y > this->target.y)
		{
			this->y = this->target.y;
			this->spd.x = 0;
			this->spd.y = 0;
			this->state = 2;
			this->delay = 5;
			shake = 5;
			smoke_init(this->x, this->y + 4);
			sfx(5, 0);
		}
	}
	//-- landing
	else if (this->state == 2)
	{
		this->delay -= 1;
		this->spr = 6;
		if (this->delay == 0) {
			this->active = false;
			player_init(&player, this->x, this->y);
		}
	}
}

void player_spawn_draw(PLAYER_SPAWN* this)
{
	set_hair_color(max_djump);
	spr_with_palette(this->spr, this->x, this->y, player_color, this->flip.x, this->flip.y);
	draw_hair(this->x, this->y, 1);
}


// hair
// 
void create_hair(short x, short y)
{
	for (int i = 0; i < MAX_HAIR; i++)
	{
		hair[i].x = x << 16;
		hair[i].y = y << 16;
		hair[i].size = 122 + max(1, min(2,4-i));
	}
}

void set_hair_color(short djump)
{
	if(djump == 1)
	{
		player_color = 0;
	}
	else if(djump == 2)
	{
		if((frames / 3) % 2)
		{
			player_color = 2;
		}
		else
		{
			player_color = 3;
		}
	}
	else
	{
		player_color = 1;
	}
}

void draw_hair(short x, short y, short flip)
{
	fixed_t last_x = (x - flip * 2) << 16;
	fixed_t last_y = (y + (down_now ? 3 : 2)) << 16;
	HAIR* hair_temp;

	for(int i = 0; i < MAX_HAIR;i++)
	{
		hair_temp = &(hair[i]);
		hair_temp->x += FixedDiv((last_x - hair_temp->x), 98304);
		hair_temp->y += FixedDiv((last_y + 32768 - hair_temp->y) , 98304);
		spr_with_palette(hair_temp->size, hair_temp->x >> 16, hair_temp->y >> 16, player_color, 0, 0);
		x = hair_temp->x;
		y = hair_temp->y;
	}
}

void dead_particle_draw(DEAD_PARTICLE* this)
{
	spr_with_palette(73 + (this->t / 3), this->x, this->y, 2, 0, 0);
}

void spring_init(SPRING* this,short x, short y)
{
	init_object(&(this->obj), type_spring, x, y);
	this->obj.hitbox.w = 8;
	this->obj.hitbox.h = 8;
	this->hide_in = 0;
	this->hide_for = 0;
	this->delay = 0;
}

void spring_update(SPRING* this)
{
	if (this->hide_for > 0)
	{
		this->hide_for -= 1;
		if (this->hide_for <= 0)
		{
			this->obj.spr = 18;
			this->delay = 0;
		}
	}
	else if (this->obj.spr == 18)
	{
		if (player.obj.spd.y >= 0)
		{
			if(obj_collide(&(this->obj), &(player.obj), 0,0))
			{
				this->obj.spr = 19;
				player.obj.y = this->obj.y - 8;
				player.obj.spd.x = FixedMul(player.obj.spd.x, 13107); //0.2
				player.obj.spd.y = -196608; // -3
				player.djump = max_djump;
				this->delay = 10;
				smoke_init(this->obj.x, this->obj.y);

				//-- breakable below us
				FALL_FLOOR* fall_floor_temp;
				for (int i = 0; i < MAX_FALL_FLOORS; i++)
				{
					fall_floor_temp = &(fall_floors[i]);

					if(!fall_floor_temp->obj.active)
					{
						break;
					}
					else if (fall_floor_temp->obj.x == this->obj.x && fall_floor_temp->obj.y == this->obj.y + 8)
					{
							break_fall_floor(fall_floor_temp);
					}
				}

				sfx(8, 1);
			}
		}
	}
	else if (this->delay > 0)
	{
		this->delay -= 1;
		if (this->delay <= 0)
		{
			this->obj.spr = 18;
		}
	}
	//-- begin hiding
	if (this->hide_in > 0)
	{
		this->hide_in -= 1;
		if (this->hide_in <= 0)
		{
			this->hide_for = 60;
			this->obj.spr = 0;
		}
	}
}

void spring_draw(SPRING* this)
{
	spr_not_flip(this->obj.spr, this->obj.x, this->obj.y);
}

void break_spring(SPRING* this)
{
	this->hide_in = 15;
}


void balloon_init(BALLOON* this, short x, short y)
{
	init_object(&(this->obj), type_balloon, x, y);

	this->offset = 0;
	this->start = this->obj.y;
	this->timer = 0;
	this->obj.hitbox.x = -1;
	this->obj.hitbox.y = -1;
	this->obj.hitbox.w = 10;
	this->obj.hitbox.h = 10;
}

void balloon_update(BALLOON* this)
{
	if (this->obj.spr == 22)
	{
		this->offset += 655; //0.01
		this->obj.y = this->start + ((sin(this->offset) * 2) >> 16);
		if (player.djump < max_djump)
		{
			if (obj_collide(&(this->obj), &(player.obj), 0,0))
			{
				sfx(6, 2);
				smoke_init(this->obj.x, this->obj.y);
				player.djump = max_djump;
				this->obj.spr = 0;
				this->timer = 60;
			}
		}
	}
	else if (this->timer > 0) {
		this->timer -= 1;
	}
	else
	{
		sfx(7, 2);
		smoke_init(this->obj.x, this->obj.y);
		this->obj.spr = 22;
	}
}

void balloon_draw(BALLOON* this)
{
	if(this->obj.spr == 22)
	{
		spr_not_flip(22, this->obj.x, this->obj.y);
		spr_not_flip(13 + ((this->offset >> 14) % 3), this->obj.x, this->obj.y + 6);
	}
}

void fall_floor_init(FALL_FLOOR* this, short x, short y)
{
	init_object(&(this->obj), type_fall_floor, x, y);

	this->state = 0;
	this->obj.hitbox.w = 8;
	this->obj.hitbox.h = 8;
}

void fall_floor_update(FALL_FLOOR* this)
{
	//-- idling
	if (this->state == 0)
	{
		if (obj_collide(&(this->obj), &(player.obj), 0, -1) || obj_collide(&(this->obj), &(player.obj), -1, 0) || obj_collide(&(this->obj), &(player.obj), 1, 0))
		{
			break_fall_floor(this);
		}
	}
	//-- shaking
	else if (this->state == 1) {
		this->delay -= 1;
		room_tiles[this->obj.x / 8][this->obj.y / 8] = 23 + (15 - this->delay) / 5;
		if (this->delay <= 0)
		{
			this->state = 2;
			this->delay = 60; //--how long it hides for
			this->obj.collideable = false;
			room_tiles[this->obj.x / 8][this->obj.y / 8] = 0;
		}
	}
	//-- invisible, waiting to reset
	else if (this->state == 2)
	{
		if (this->delay > 0)
			this->delay -= 1;
		else
		{
			if (!obj_collide(&(this->obj), &(player.obj), 0, 0))
			{
				sfx(7, 2);
				this->state = 0;
				this->obj.collideable = true;
				room_tiles[this->obj.x / 8][this->obj.y / 8] = 23;
				smoke_init(this->obj.x, this->obj.y);
			}
		}
	}
}

void break_fall_floor(FALL_FLOOR* this)
{
	if (this->state == 0)
	{
		sfx(15, 1);
		this->state = 1;
		this->delay = 15; //--how long until it falls
		smoke_init(this->obj.x, this->obj.y);

		SPRING* spring_temp;

		for (int i = 0; i < MAX_SPRINGS; i++)
		{
			spring_temp = &(springs[i]);

			if (spring_temp->obj.x == this->obj.x && spring_temp->obj.y == this->obj.y - 8)
			{
				break_spring(spring_temp);
			}
		}
	}

}

void fall_floor_draw(FALL_FLOOR* this)
{
	if (this->state != 2)
	{
		if (this->state != 1)
		{
			spr_not_flip(23,this->obj.x,this->obj.y);
		}
		else
		{
			spr_not_flip(23+(15-this->delay)/5,this->obj.x,this->obj.y);
		}
	}
}

void smoke_init(short x, short y)
{
	for(int i = 0; i < MAX_SMOKE; i++)
	{
		SMOKE* this = &(smoke[i]);
		if(!this->obj.active)
		{
			init_object(&(this->obj), type_smoke, x, y);

			this->obj.spr = 29;
			this->obj.spd.y = -6554; // -6554 mean -0.1 of fixed
			this->obj.spd.x = 19661 + rnd(13107); // 0.3 
			this->obj.x += -1 + rnd(2);
			this->obj.y += -1 + rnd(2);
			this->obj.flip.x = maybe();
			this->obj.flip.y = maybe();
			this->obj.solids = false;
			this->timer = 0;

			break;
		}
	}
}

void smoke_update(SMOKE* this)
{
	this->timer += 1;
	
	if(this->timer == 5)
	{
		this->timer = 0;
		this->obj.spr += 1;
		
	}

	if (this->obj.spr >= 32)
	{
		destroy_object(&(this->obj));
	}
}

void smoke_draw(SMOKE* this)
{
	spr(this->obj.spr, this->obj.x, this->obj.y, this->obj.flip.x, this->obj.flip.y);
}

void lifeup_init(LIFEUP* this,short x, short y)
{
	init_object(&(this->obj), type_lifeup, x - 8, y - 4);

	this->obj.spd.y = -16384; // -0.25
	this->duration = 30;
	this->flash = 0;
	this->obj.solids = false;
}

void lifeup_update(LIFEUP* this)
{
	this->duration -= 1;

	if (this->duration <= 0)
	{
		destroy_object(&(this->obj));
	}
}

void lifeup_draw(LIFEUP* this)
{
	if(frames % 2)
	{
		this->flash += 1;
	}
	short flash = (this->flash % 2) * 3;

	spr_with_palette(129, this->obj.x, this->obj.y, flash, 0, 0);
	spr_with_palette(128, this->obj.x + 8, this->obj.y, flash, 0, 0);
}


void fruit_init(FRUIT* this, short x, short y)
{
	if(got_fruit[1 + level_index()] == true)
	{
		fruit.obj.active = false;
		return;
	}

	init_object(&(this->obj), type_fruit, x, y);
	this->start = this->obj.y;
	this->off = 0;
	this->obj.hitbox.w = 8;
	this->obj.hitbox.h = 8;
}

void fruit_draw(FRUIT* this)
{
	spr_not_flip(type_fruit, this->obj.x, this->obj.y);
}

void fruit_update(FRUIT* this)
{
	if (obj_collide(&(this->obj), &(player.obj), 0, 0) )
	{
		player.djump = max_djump;
		sfx(13, 2);
		got_fruit[1 + level_index()] = true;
		lifeup_init(&lifeup, this->obj.x, this->obj.y);
		this->obj.active = false;
	}
	this->off += 1310; //0.02
	this->obj.y = this->start + ((sin(this->off) * 4) >> 16);
}

void fly_fruit_init(FLY_FRUIT* this, short x, short y)
{
	if (got_fruit[1 + level_index()] == true)
	{
		this->obj.active = false;
		return;
	}

	init_object(&(this->obj), type_fly_fruit, x, y);
	this->start = this->obj.y;
	this->fly = false;
	this->step = 32768; // 0.5
	this->obj.solids = false;
	this->sfx_delay = 8;
	this->obj.hitbox.w = 8;
	this->obj.hitbox.h = 8;
}

void fly_fruit_update(FLY_FRUIT* this)
{
	//--fly away
	if (this->fly)
	{
		if (this->sfx_delay > 0)
			{
			this->sfx_delay -= 1;
			if (this->sfx_delay <= 0)
			{
				sfx(14, 2);
			}
		}
		this->obj.spd.y = appr(this->obj.spd.y, -229376, 16384); // -3.5, 0.25
		if (this->obj.y < -32)
		{
			this->obj.active = false;
		}
	}
	//-- wait
	else
	{
		if (has_dashed)
		{
			this->fly = true;
		}
		this->step += 3277; //0.05
		this->obj.spd.y = FixedMul(sin(this->step) , 32768); //0.5
	}
	//-- collect
	if (obj_collide(&(this->obj), &(player.obj), 0, 0))
	{
		player.djump = max_djump;
		sfx(13, 2);
		got_fruit[1 + level_index()] = true;
		lifeup_init(&lifeup, this->obj.x, this->obj.y);
		this->obj.active = false;
	}
}

void fly_fruit_draw(FLY_FRUIT* this)
{
	short off = 0;

	if (!this->fly)
	{
		if (sin(this->step) < 0)
		{
			off = 1 + max(0, (sign(this->start - this->obj.y)));
		}
	}
	else
	{
		off = 0;
	}

	spr(45 + off, this->obj.x - 6, this->obj.y - 2, true, false);
	spr_not_flip(this->obj.spr, this->obj.x, this->obj.y);
	spr(45 + off, this->obj.x + 6, this->obj.y - 2, false, false);

}

void fake_wall_init(OBJ* this,short  x, short y)
{
	if (got_fruit[1 + level_index()] == true)
	{
		this->active = false;
		return;
	}

	init_object(this, type_fake_wall, x, y);

	this->hitbox.x = 0;
	this->hitbox.y = 0;
	this->hitbox.w = 16;
	this->hitbox.h = 16;
}

void fake_wall_update(OBJ* this)
{

	if(player.dash_effect_time > 0)
	{
		this->hitbox.x = -1;
		this->hitbox.y = -1;
		this->hitbox.w = 18;
		this->hitbox.h = 18;

		if(obj_collide(this, &(player.obj), 0, 0))
		{
			player.obj.spd.x = -sign(player.obj.spd.x) * 98304;
			player.obj.spd.y = -98304; //-1.5
			player.dash_time = 0;
			sfx(16, 1);
			this->active = false;
			smoke_init(this->x, this->y);
			smoke_init(this->x + 8, this->y);
			smoke_init(this->x, this->y + 8);
			smoke_init(this->x + 4, this->y + 4);
			fruit_init(&fruit, this->x + 4, this->y + 4);
		}

		this->hitbox.x = 0;
		this->hitbox.y = 0;
		this->hitbox.w = 16;
		this->hitbox.h = 16;

	}

}

void fake_wall_draw(OBJ* this)
{
	spr_double(64, this->x, this->y);
	spr_double(80, this->x, this->y + 8);
}

void key_init(OBJ* this, short x, short y)
{
	init_object(this, type_key, x, y);

		this->hitbox.x = 0;
		this->hitbox.y = 0;
		this->hitbox.w = 8;
		this->hitbox.h = 8;
}

void key_update(OBJ* this)
{
	short was = flr(this->spd.x) >> 16;
	this->spd.x = 589824 + (sin((frames << 16) / 30) + 32768); // 32768 mean 05
	short is = flr(this->spd.x) >> 16;
	this->spr = is;
	if (is == 10 && is != was)
	{
		this->flip.x = !this->flip.x;
	}
	if (obj_collide(this, &(player.obj), 0, 0))
	{
		sfx(23, 1);
		this->active = false;
		has_key = true;
	}
}

void key_draw(OBJ* this)
{
	spr(this->spr, this->x, this->y, this->flip.x, 0);
}


void chest_init(CHEST* this,short x, short y)
{
	if (got_fruit[1 + level_index()] == true)
	{
		this->obj.active = false;
		return;
	}

	init_object(&(this->obj), type_chest, x - 4, y);
	this->start = this->obj.x;
	this->timer = 20;
	this->obj.hitbox.w = 8;
	this->obj.hitbox.h = 8;
}

void chest_update(CHEST* this)
{
	if (has_key)
	{
		this->timer -= 1;
		this->obj.x = this->start - 1 + rnd(3);
		if (this->timer <= 0)
		{
			sfx(16, 1);
			fruit_init(&(fruit), this->obj.x, this->obj.y - 4);
			destroy_object(&(this->obj));
		}
	}
}

void chest_draw(CHEST* this)
{
	spr_not_flip(type_chest, this->obj.x, this->obj.y);
}

void platform_init(PLATFORM* this, short x, short y, short dir)
{
	init_object(&(this->obj), type_platform, x, y);

	this->obj.x -= 4;
	this->obj.solids = false;
	this->obj.hitbox.w = 16;
	this->last = this->obj.x;
	this->obj.spd.x = dir * 42598;
	this->offset = this->obj.spd.x;
}

void platform_update(PLATFORM* this)
{
	this->offset += this->obj.spd.x;
	this->obj.x += this->offset / 65536;
	if(65536 <= this->offset)
	{
		this->offset -= 65536;
	}
	else if(-65536 >= this->offset)
	{
		this->offset += 65536;
	}
	
	if (this->obj.x < -16)
	{
		this->obj.x = 128;
	}
	else if (this->obj.x > 128)
	{
		this->obj.x = -16;
	}

	if (player.obj.spd.y >= 0)
	{
		if (obj_collide(&(this->obj), &(player.obj), 0, -1))
		{
			obj_move_x(&(player.obj), this->obj.x - this->last, 1);
		}
	}

	this->last = this->obj.x;

}

void platform_draw(PLATFORM* this)
{
	spr_double(11, this->obj.x, this->obj.y);
}

void message_init(MESSAGE* this, short x, short y)
{
	init_object(&(this->obj), type_message, x, y);
	this->obj.hitbox.w = 8;
	this->obj.hitbox.h = 8;
	this->index = 0;
}

void message_draw(MESSAGE* this)
{
	int i, x, y;
	char text[] = "-- celeste mountain --#this memorial to those# perished on theclimb ";

	if(obj_collide(&(this->obj), &(player.obj), 0, 0))
	{
		if(text[this->index] != '\0')
		{
			if(frames % 2 == 0)
			{
				this->index += 1;
			}

			if(frames % 3 == 0)
			{
				sfx(35, 1);
			}
		}
		
		text[this->index] = '\0';

		x = 0;
		y = 0;
		for(i = 0;text[i] != '\0';i++)
		{
			if(text[i] == '#')
			{
				x = 0;
				y++;
			}
			else
			{
				spr_with_palette(128 + text[i], 20 + (x * 4), 80 + (y * 8), 6, 0, 0);
				x++;
			}
		}
	}
	else
	{
		this->index=0;
	}
}

void big_chest_init(BIG_CHEST* this,short x, short y)
{
	init_object(&(this->obj), type_big_chest, x, y);

	this->state = 0;
	this->timer = 0;
	this->obj.hitbox.w = 8;
	this->obj.hitbox.h = 8;
}

void big_chest_draw(BIG_CHEST* this)
{
	if (this->obj.active == false)
	{
		return;
	}

	if (this->state == 0)
	{
		if (obj_collide(&(this->obj), &(player.obj), 0, 8) && obj_is_solid(&(player.obj), 0, 1))
		{
			music(-1, 500, 7);
			sfx(37, 1);
			pause_player = true;
			player.obj.spd.x = 0;
			player.obj.spd.y = 0;
			this->state = 1;
			smoke_init(this->obj.x, this->obj.y);
			smoke_init(this->obj.x + 8, this->obj.y);
			this->timer = 75;
			//this.particles = {}
		}
		spr_double(96, this->obj.x, this->obj.y);
	}
	else if (this->state == 1)
	{
		this->timer -= 1;
		shake = 5;
		fill_background_color(frames / 5);

		if(this->timer <= 45)
		{

		}

		if(this->timer < 0)
		{
			this->state = 2;
			fill_background_color(2);
			for(int i = 0;i < MAX_CLOUDS;i++)
			{
				CLOUD* cloud_temp = &(clouds[i]);
				new_bg(cloud_temp, 14);
			}

			orb_init(&(orb), this->obj.x + 4, this->obj.y + 4);
			pause_player=false;
		}

		spr_double(112, this->obj.x, this->obj.y + 8);
	}
}

void orb_init(ORB* this, short x, short y)
{
	init_object(&(this->obj), type_orb, x, y);

	this->obj.spd.y = -262144;
	this->obj.solids = false;

	this->obj.hitbox.w = 8;
	this->obj.hitbox.h = 8;
}

void orb_draw(ORB* this)
{
	this->obj.spd.y = appr(this->obj.spd.y, 0, 32768);

	if (this->obj.spd.y == 0 && obj_collide(&(this->obj), &(player.obj), 0, 0))
	{
		sfx(51, 2);
		music_timer = 45;
		freeze = 10;
		shake = 10;
		destroy_object(&(this->obj));
		max_djump = 2;
		player.djump = 2;
	}

	spr_not_flip(102, this->obj.x, this->obj.y);

	int off =  (frames << 16) / 30;
	for(int i = 0;i <= 7;i++)
	{
		spr_not_flip(74, this->obj.x + ((cos((off + (i << 16)) / 8) * 8) >> 16) + 1, this->obj.y + ((sin((off + (i << 16)) / 8) * 8) >> 16) + 1);
	}
}

void flag_init(FLAG* this,short x, short y)
{
	short i;

	init_object(&(this->obj), type_flag, x, y);
	this->score = 0;
	this->show = false;
	this->obj.hitbox.w = 8;
	this->obj.hitbox.h = 8;

	for (i = 0; i < MAX_FRUIT; i++)
	{
		if (got_fruit[i])
		{
			this->score += 1;
		}
	}
}

void flag_draw(FLAG* this)
{
	this->obj.spr = 118 + (frames / 5) % 3;

	spr_not_flip(this->obj.spr, this->obj.x, this->obj.y);

	if(this->show)
	{
		draw_time(48, 24);

		spr_not_flip(type_fruit, 53, 14);
		char str[8] = "x  ";
		if(this->score < 10)
		{
				str[1] = (this->score % 10) + '0';
		}
		else
		{
				str[1] = (this->score / 10) + '0';
				str[2] = (this->score % 10) + '0';
		}
		print_with_spr(str, 62, 16, 4);

		char str2[16] = "deaths:9999";
		short offset = 0;
		if(deaths < 9999)
		{
			short temp = deaths;

			short thousands = temp / 1000;
			temp -= thousands * 1000;

			short hundreds = temp / 100;
			temp -= hundreds * 100;

			short tens = temp / 10;
			temp -= tens * 10;

			short ones = temp % 10;

			if(deaths >= 1000)
			{
				str2[7] = thousands + '0';
				str2[8] = hundreds + '0';
				str2[9] = tens + '0';
				str2[10] = ones + '0';
				offset = 6;
			}
			else if(deaths >= 100)
			{
				str2[7] = hundreds + '0';
				str2[8] = tens + '0';
				str2[9] = ones + '0';
				str2[10] = '\0';
				offset = 4;
			}
			else if(deaths >= 10)
			{
				str2[7] = tens + '0';
				str2[8] = ones + '0';
				str2[9] = '\0';
				offset = 2;
			}
			else
			{
				str2[7] = ones + '0';
				str2[8] = '\0';
			}
		}

		print_with_spr(str2, 48 - offset, 32, 4);
	}
	else if(obj_collide(&(this->obj),&(player.obj),0,0))
	{
		sfx(55, 2);
		this->show = true;
	}
}

void room_title_init(ROOM_TITLE* this)
{
	this->active = true;
	this->delay = 5;
}

void room_title_draw(ROOM_TITLE* this)
{
	this->delay -= 1;
	if(this->delay < -30)
	{
		can_pause = true;
		this->active = false;
	}
	else if(this->delay < 0)
	{
		int i;

		if(room.x == 3 && room.y == 1)
		{
			print_with_spr("old site", 48, 62, 4);
		}
		else if (level_index() == 30)
		{
			print_with_spr("summit",52,62, 4);
		}
		else
		{
			char str[8] = " 000 M";
			short level = (1 + level_index()) * 100;

			if(level >= 1000)
			{
				str[0] = (level / 1000) + '0';
			}
			str[1] = ((level % 1000) / 100) + '0';
			
			print_with_spr(str, 52, 62, 4);
		}

		for(i = 0;i < 12;i++)
		{
			spr_with_palette(255, 16 + (i * 8), 58, 4, 0, 0);
			spr_with_palette(255, 16 + (i * 8), 66, 4, 0, 0);

		}

		draw_time(8, 8);
	}
}

void particle_init()
{
	short i;

	for(i = 0;i < MAX_PARTICLES;i++)
	{
		PARTICLE* this = &(particles[i]);
		this->x = rnd(128);
		this->y = rnd(128);
		this->s = 73;
		if(rnd(3) < 1)
		{
			this->s = 74;
		}
		this->spd = (rnd(5) << 16) + 65536;
		this->off = maybe();
		this->c = 6;
	}
}

void clouds_init()
{
	int i;

	for(i = 0;i < MAX_CLOUDS;i++)
	{
		CLOUD* this = &(clouds[i]);
		this->x = rnd(128);
		this->y = rnd(128);
		this->s = 77;
		if(rnd(5) < 1)
		{
			this->s = 79;
		}
		this->c = 0;
		this->spd = (rnd(3) + 1) * 2;
	}
}

// -- update function --
// ---------------------

void celeste_update()
{
	int i;

	frames = ((frames+1)%30);
	if (frames == 0 && level_index()<30)
	{
		seconds = ((seconds+1)%60);
		if (seconds == 0)
		{
			minutes += 1;
		}
	}

	if (music_timer > 0) {
		music_timer -= 1;
		if (music_timer <= 0) {
			music(10,0,7);
		}
	}

	//-- cancel if freeze
	if (freeze > 0 ) 
	{
		freeze -= 1;
		return;
	}

	//-- screenshake
	if (shake > 0)
	{
		shake -= 1;
		camera(0,0);
		if (can_shake && shake > 0)
		{
			camera(-2-rnd(5),-2+rnd(5));
		}
	}

	//-- restart (soon)
	if (will_restart && delay_restart > 0)
	{
		delay_restart -= 1;
		if (delay_restart <= 0)
		{
			will_restart = false;
			load_room(room.x,room.y);
		}
	}

	//-- update each object
	if (player_spawn.active)
	{
		player_spawn_update(&player_spawn);
	}

	if (player.obj.active)
	{
		player_update(&player);
		obj_move(&(player.obj), player.obj.spd.x, player.obj.spd.y);
	}

	if (fake_wall.active)
	{
		fake_wall_update(&fake_wall);
	}

	if(key.active)
	{
		key_update(&key);
	}

	if(chest.obj.active)
	{
		chest_update(&chest);
	}

	if (fly_fruit.obj.active)
	{
		fly_fruit_update(&fly_fruit);
		obj_move(&(fly_fruit.obj), fly_fruit.obj.spd.x, fly_fruit.obj.spd.y);
	}

	if (fruit.obj.active)
	{
		fruit_update(&fruit);
	}

	for(i = 0;i < MAX_BALLOONS;i++)
	{
		BALLOON* this = &(balloons[i]);
		if(this->obj.active)
		{
			balloon_update(this);
		}
	}

	for(i = 0;i < MAX_SPRINGS;i++)
	{
		SPRING* this = &(springs[i]);
		if(this->obj.active)
		{
			spring_update(this);
		}
	}

	for(i = 0;i < MAX_FALL_FLOORS;i++)
	{
		FALL_FLOOR* this = &(fall_floors[i]);
		if(this->obj.active)
		{
			fall_floor_update(this);
		}
	}

	for(i = 0;i < MAX_PLATFORMS;i++)
	{
		PLATFORM* this = &(platforms[i]);
		if(this->obj.active)
		{
			platform_update(this);
		}
	}

	// -- start game
	if (is_title())
	{
		if (start_game == false && jump_now)
		{
			music(-1,0,0);
			start_game_flash = 50;
			start_game = true;
			sfx(38, 1);
		}

		if (start_game)
		{
			start_game_flash -= 1;
			if (start_game_flash <= -30)
			{
				begin_game();
			}
		}
	}

}


// -- drawing functions --
// -----------------------

void fill_palette(short c)
{
	_Far unsigned short *sprram;
	_FP_SEG(sprram) = 0x130;
	_FP_OFF(sprram) = 0x2000;

	for(int i = 0;i < 16;i++)
	{
		sprram[i] = base_palette[c];
	}
}

void reset_palette()
{
	SPR_setPaletteBlock(256, 1, (char *)base_palette);
}

void fill_background_color(short c)
{
	_Far unsigned short *vram;
	_FP_SEG(vram) = 0x104;
	_FP_OFF(vram) = 0x0;

	int offset = 0;

	for(int y = 0;y < 24;y++)
	{
		for(int x = 0;x < 26;x++)
		{
			vram[offset++] = base_palette[c];
		}
		offset += 230;
	}
}

void camera(int x, int y)
{
	SPR_setOffset(x, y);
}

void new_bg(CLOUD* cloud, bool set)
{
	if(set)
	{
		cloud->c = 7;
	}
	else
	{
		cloud->c = 0;
	}
}

void spr_reset()
{
	_Far unsigned short *sprram;
	_FP_SEG(sprram) = 0x130;
	_FP_OFF(sprram) = 0x0;

	int sprram_offset = 0;

	for(int i = 0; i < 1024;i++)
	{
		sprram[sprram_offset++] = 0;
		sprram[sprram_offset++] = 0;
		sprram[sprram_offset++] = 0;
		sprram[sprram_offset++] = 0;
	}
}

void spr_not_flip(short n, short x, short y)
{
	_Far unsigned short *sprram;
	_FP_SEG(sprram) = 0x130;
	_FP_OFF(sprram) = 0x0;

	//spr(obj.spr,obj.x,obj.y,1,1,obj.flip.x,obj.flip.y)
	if (sprite_number >= 1024)
	{
		return;
	}

	int sprram_offset = (1023 - sprite_number) << 2;

	sprram[sprram_offset++] = x;
	sprram[sprram_offset++] = y;

	sprram[sprram_offset++] = 0x8c80 + n;
	sprram[sprram_offset] = 0x8100;

	sprite_number++;
}

void spr(short n, short x, short y, bool flip_x, bool flip_y)
{
	_Far unsigned short *sprram;
	_FP_SEG(sprram) = 0x130;
	_FP_OFF(sprram) = 0x0;

	//spr(obj.spr,obj.x,obj.y,1,1,obj.flip.x,obj.flip.y)

	if (sprite_number >= 1024)
	{
		return;
	}

	int sprram_offset = (1023 - sprite_number) << 2;

	sprram[sprram_offset++] = x;
	sprram[sprram_offset++] = y;

	if (flip_x == true)
	{
		if(flip_y == true)
		{
			sprram[sprram_offset++] = 0x9c80 + n;
		}
		else
		{
			sprram[sprram_offset++] = 0xac80 + n;
		}

	}
	else
	{
		if(flip_y == true)
		{
			sprram[sprram_offset++] = 0xbc80 + n;
		}
		else
		{
			sprram[sprram_offset++] = 0x8c80 + n;
		}
	}

	sprram[sprram_offset] = 0x8100;

	sprite_number++;
}

void spr_double(short n, short x, short y)
{
	_Far unsigned short *sprram;
	_FP_SEG(sprram) = 0x130;
	_FP_OFF(sprram) = 0x0;

	//spr(obj.spr,obj.x,obj.y,1,1,obj.flip.x,obj.flip.y)
	if (sprite_number >= 1023)
	{
		return;
	}

	int sprram_offset = ((1023 - sprite_number) << 2) + 3;

	sprram[sprram_offset--] = 0x8100;
	n = 0x8c80 + n;
	sprram[sprram_offset--] = n++;
	sprram[sprram_offset--] = y;
	sprram[sprram_offset--] = x;
	x += 8;
	sprram[sprram_offset--] = 0x8100;
	sprram[sprram_offset--] = n;
	sprram[sprram_offset--] = y;
	sprram[sprram_offset] = x;

	sprite_number += 2;
}

void spr_with_palette(short n, short x, short y, short c, bool flip_x, bool flip_y)
{
	_Far unsigned short *sprram;
	_FP_SEG(sprram) = 0x130;
	_FP_OFF(sprram) = 0x0;

	//spr(obj.spr,obj.x,obj.y,1,1,obj.flip.x,obj.flip.y)

	if (sprite_number >= 1024)
	{
		return;
	}

	int sprram_offset = (1023 - sprite_number) << 2;

	sprram[sprram_offset++] = x;
	sprram[sprram_offset++] = y;

	if (flip_x == true)
	{
		if(flip_y == true)
		{
			sprram[sprram_offset++] = 0x9c80 + n;
		}
		else
		{
			sprram[sprram_offset++] = 0xac80 + n;
		}

	}
	else
	{
		if(flip_y == true)
		{
			sprram[sprram_offset++] = 0xbc80 + n;
		}
		else
		{
			sprram[sprram_offset++] = 0x8c80 + n;
		}
	}

	sprram[sprram_offset] = 0x8100 + c;

	sprite_number++;
}

void print_with_spr(char const* str, short x, short y, short c)
{
	for(int i = 0; str[i] != '\0';i++, x += 4)
	{
		spr_with_palette(128 + str[i], x, y, c, 0, 0);
	}
}

void draw_time(short x, short y)
{
	short s = seconds;
	short m = minutes % 60;
	short h = minutes / 60;

	char str[9] = "00:00:00";
	str[0] = (h/10) + '0';
	str[1] = (h%10) + '0';
	str[3] = (m/10) + '0';
	str[4] = (m%10) + '0';
	str[6] = (s/10) + '0';
	str[7] = (s%10) + '0';

	print_with_spr(str,x,y, 4);
}

void particle_draw()
{
	_Far unsigned short *sprram;
	_FP_SEG(sprram) = 0x130;
	_FP_OFF(sprram) = 0x0;

	short i;

	int sprram_offset = ((1023 - sprite_number) << 2) + 3;

	for(i = 0;i < MAX_PARTICLES;i++)
	{
		PARTICLE* this = &(particles[i]);
		this->x += this->spd >> 16;
		this->y += sin(this->off) >> 15;
		this->off += min(3276,this->spd / 32);
		if(this->x > 132)
		{
			this->x = -8;
			this->y = rnd(128);
		}

		sprram[sprram_offset--] = 0x8100;
		sprram[sprram_offset--] = 0x8c80 + this->s;
		sprram[sprram_offset--] = this->y;
		sprram[sprram_offset--] = this->x;

	}

	sprite_number += MAX_PARTICLES;
}

void clouds_draw()
{
	_Far unsigned short *sprram;
	_FP_SEG(sprram) = 0x130;
	_FP_OFF(sprram) = 0x0;

	short i, x, c, s;

	int sprram_offset = ((1023 - sprite_number) << 2) + 3;

	for(i = 0;i < MAX_CLOUDS;i++)
	{
		CLOUD* this = &(clouds[i]);
		this->x += this->spd;
		if(this->x >= 130)
		{
			this->x = -40;
			this->y = rnd(120);
		}

		c = 0x8100 + this->c;
		s = 0x8c80 + this->s;

		for(x=0; x <= 32;x += 8)
		{
			sprram[sprram_offset--] = c;
			sprram[sprram_offset--] = s;
			sprram[sprram_offset--] = this->y;
			sprram[sprram_offset--] = this->x + x;
		}
		sprite_number += 5;

	}
}

void bg_draw()
{
	_Far unsigned short *sprram;
	_FP_SEG(sprram) = 0x130;
	_FP_OFF(sprram) = 0x0;

	short x, y;

	if (sprite_number >= 769)
	{
		return;
	}

	int sprram_offset = ((1023 - sprite_number) << 2) + 3;

	for(x = 0; x < 16;x++)
	{
		for(y = 0; y < 16;y++)
		{
			sprram[sprram_offset--] = 0x8100;
			sprram[sprram_offset--] = 0x8c80 + room_tiles[x][y];
			sprram[sprram_offset--] = y << 3;
			sprram[sprram_offset--] = x << 3;
		}
	}

	sprite_number += 256;
}

void spr_hidden(int number)
{
	_Far unsigned short *sprram;
	_FP_SEG(sprram) = 0x130;
	_FP_OFF(sprram) = 0x0;

	sprram[((1023 - sprite_number) << 2) + 3] = 0xa100;
}

void celeste_draw()
{
	short i;

	particle_draw();

	// --objects

	for (i = 0; i < MAX_SMOKE; i++)
	{
		SMOKE* this= &(smoke[i]);
		if(this->obj.active)
		{
			obj_move(&(this->obj), this->obj.spd.x, this->obj.spd.y);
			smoke_draw(this);
			smoke_update(this);
		}
	}

	if(game_pause)
	{
		draw_time(8, 8);
		print_with_spr("PAUSE", 54, 58, 4);
		print_with_spr(" PRESS SELECT TO EXIT ", 20, 66, 4);

		for(i = 0;i < 11;i++)
		{
			spr_with_palette(255, 20 + (i * 8), 58, 4, 0, 0);
		}
	}

	if(room_title.active)
	{
		room_title_draw(&(room_title));
	}

	if (player_spawn.active)
	{
		player_spawn_draw(&(player_spawn));
	}

	if (player.obj.active)
	{
		player_draw(&(player));
	}
	else
	{
		for (i = 0; i < MAX_DEAD_PARTICLES;i++)
		{
			DEAD_PARTICLE* this = &(dead_particles[i]);
			if(this->active)
			{
				this->x += this->spd.x >> 16;
				this->y += this->spd.y >> 16;
				this->t -= 1;
				if(this->t <= 0)
				{
					this->active = false;
				}
				dead_particle_draw(this);
			}
		}
	}

	// --big chest
	if(big_chest.obj.active)
	{
		big_chest_draw(&(big_chest));
	}

	if(orb.obj.active)
	{
		obj_move(&(orb.obj), orb.obj.spd.x, orb.obj.spd.y);
		orb_draw(&(orb));
	}

	if(chest.obj.active)
	{
		chest_draw(&(chest));
	}

	if(fake_wall.active)
	{
		fake_wall_draw(&(fake_wall));
	}

	if(fly_fruit.obj.active)
	{
		fly_fruit_draw(&(fly_fruit));
	}

	if(fruit.obj.active)
	{
		fruit_draw(&(fruit));
	}

	if(lifeup.obj.active)
	{
		obj_move(&(lifeup.obj), lifeup.obj.spd.x, lifeup.obj.spd.y);
		lifeup_update(&(lifeup));
		lifeup_draw(&(lifeup));
	}

	if(key.active)
	{
		key_draw(&(key));
	}

	if(message.obj.active)
	{
		message_draw(&(message));
	}

	if(flag.obj.active)
	{
		flag_draw(&(flag));
	}

	for(i = 0; i < MAX_BALLOONS; i++)
	{
		BALLOON* this = &(balloons[i]);
		if(this->obj.active)
		{
			balloon_draw(this);
		}
	}

	for(i = 0; i < MAX_SPRINGS; i++)
	{
		SPRING* this = &(springs[i]);
		if(this->obj.active)
		{
			spring_draw(this);
		}
	}

	for(i = 0;i < MAX_FALL_FLOORS;i++)
	{
		FALL_FLOOR* this = &(fall_floors[i]);
		if(this->obj.active)
		{
			fall_floor_draw(this);
		}
	}

	// --platforms
	for (i = 0; i < MAX_PLATFORMS; i++)
	{
		PLATFORM* this = &(platforms[i]);
		if(this->obj.active)
		{
			platform_draw(this);
		}
	}

	if(is_title())
	{
		short flash;

		if(!start_game)
		{
			flash = 5;
		}
		else if(start_game_flash > 10)
		{
			flash = 5 - ((abs(start_game_flash) / 3) % 2);
		}
		else if(start_game_flash > 5)
		{
			fill_palette(2);
			flash = 255;
		}
		else if(start_game_flash > 0)
		{
			fill_palette(1);
			flash = 255;
		}
		else
		{
			fill_palette(0);
			flash = 255;
		}

		print_with_spr("Press A Button",40,68, flash);
		print_with_spr("MADDY THORSON",42,80, flash);
		print_with_spr("NOEL BERRY",48,88, flash);
		print_with_spr("FM TOWNS Porting",36,104, flash);
		print_with_spr("by BCC",56,112, flash);
	}

	bg_draw();

	// --draw clouds
	if(cloud_scroll)
	{
		clouds_draw();
	}

	if (sprite_number >= 448)
	{
		sprite_number = 448;
	}

	SPR_display( 1, sprite_number );
	sprite_number = 0;
}

int main()
{
	int joy;

	celeste_init();

	particle_init();

	title_screen();

	while(_kbhit() == 0)
	{
		if(!game_pause)
		{
			celeste_update();
		}
		SPR_display (2 , 0);
		celeste_draw();

		SND_joy_in_1(0, &joy);
		up_now = ~joy  & 0x1;
		down_now = ~(joy >> 1) & 0x1;
		left_now = ~(joy >> 2) & 0x1;
		right_now = ~(joy >> 3) & 0x1;
		jump_now = ~(joy >> 4) & 0x1;
		dash_now = ~(joy >> 5) & 0x1;

		if(left_now && right_now && !game_pause_was && can_pause)
		{
			game_pause = !game_pause;
			game_pause_was = true;
		}
		if(!left_now && !right_now)
		{
			game_pause_was = false;
		}

		if(up_now && down_now && game_pause)
		{
			game_pause = false;
			title_screen();
		}
	}

	SPR_display(0, 1);
	SPR_init();
	EGB_init(work, 1536);

	SND_elevol_mute(0x00);
	MSV_play_stop();
	MSV_end();
	SND_end();

	free(work);
	free(snd_work);
	free(msvwork);
	return 0;
}
