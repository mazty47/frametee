#include "demo.h"
#include "ddnet_physics/vmath.h"
#include "nfd.h"
#include "net_events.h"
#include "timeline/timeline_model.h"
#include "user_interface/timeline/timeline_types.h"
#include <ddnet_physics/collision.h>
#include <ddnet_physics/gamecore.h>
#include <logger/logger.h>
#include <renderer/graphics_backend.h>
#include <stdio.h>
#include <string.h>
#include <system/fs.h>

#define DDNET_DEMO_IMPLEMENTATION
#include <ddnet_demo/ddnet_demo.h>

static const char *LOG_SOURCE = "DemoExport";

// ... (SHA-256 implementation remains same) ...

// SHA-256 Implementation
// Necessary for creating a valid demo header
typedef struct {
  uint8_t data[64];
  uint32_t datalen;
  uint64_t bitlen;
  uint32_t state[8];
} SHA256_CTX;

#define DBL_INT_ADD(a, b, c)     \
  if (a > 0xffffffff - (c)) ++b; \
  a += c;
#define ROTLEFT(a, b) (((a) << (b)) | ((a) >> (32 - (b))))
#define ROTRIGHT(a, b) (((a) >> (b)) | ((a) << (32 - (b))))

#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x, 2) ^ ROTRIGHT(x, 13) ^ ROTRIGHT(x, 22))
#define EP1(x) (ROTRIGHT(x, 6) ^ ROTRIGHT(x, 11) ^ ROTRIGHT(x, 25))
#define SIG0(x) (ROTRIGHT(x, 7) ^ ROTRIGHT(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x, 17) ^ ROTRIGHT(x, 19) ^ ((x) >> 10))

static const uint32_t k[64] = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
                               0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
                               0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
                               0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                               0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
                               0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
                               0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

void map_sha256_transform(SHA256_CTX *ctx, const uint8_t data[]) {
  uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];
  for (i = 0, j = 0; i < 16; ++i, j += 4)
    m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
  for (; i < 64; ++i)
    m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
  a = ctx->state[0];
  b = ctx->state[1];
  c = ctx->state[2];
  d = ctx->state[3];
  e = ctx->state[4];
  f = ctx->state[5];
  g = ctx->state[6];
  h = ctx->state[7];
  for (i = 0; i < 64; ++i) {
    t1 = h + EP1(e) + CH(e, f, g) + k[i] + m[i];
    t2 = EP0(a) + MAJ(a, b, c);
    h = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }
  ctx->state[0] += a;
  ctx->state[1] += b;
  ctx->state[2] += c;
  ctx->state[3] += d;
  ctx->state[4] += e;
  ctx->state[5] += f;
  ctx->state[6] += g;
  ctx->state[7] += h;
}

void map_sha256_init(SHA256_CTX *ctx) {
  ctx->datalen = 0;
  ctx->bitlen = 0;
  ctx->state[0] = 0x6a09e667;
  ctx->state[1] = 0xbb67ae85;
  ctx->state[2] = 0x3c6ef372;
  ctx->state[3] = 0xa54ff53a;
  ctx->state[4] = 0x510e527f;
  ctx->state[5] = 0x9b05688c;
  ctx->state[6] = 0x1f83d9ab;
  ctx->state[7] = 0x5be0cd19;
}

void map_sha256_update(SHA256_CTX *ctx, const uint8_t data[], size_t len) {
  for (size_t i = 0; i < len; ++i) {
    ctx->data[ctx->datalen] = data[i];
    ctx->datalen++;
    if (ctx->datalen == 64) {
      map_sha256_transform(ctx, ctx->data);
      DBL_INT_ADD(ctx->bitlen, ctx->bitlen, 512);
      ctx->datalen = 0;
    }
  }
}

void map_sha256_final(SHA256_CTX *ctx, uint8_t hash[]) {
  uint32_t i = ctx->datalen;
  if (ctx->datalen < 56) {
    ctx->data[i++] = 0x80;
    while (i < 56)
      ctx->data[i++] = 0x00;
  } else {
    ctx->data[i++] = 0x80;
    while (i < 64)
      ctx->data[i++] = 0x00;
    map_sha256_transform(ctx, ctx->data);
    memset(ctx->data, 0, 56);
  }
  DBL_INT_ADD(ctx->bitlen, ctx->bitlen, ctx->datalen * 8);
  ctx->data[63] = ctx->bitlen;
  ctx->data[62] = ctx->bitlen >> 8;
  ctx->data[61] = ctx->bitlen >> 16;
  ctx->data[60] = ctx->bitlen >> 24;
  ctx->data[59] = ctx->bitlen >> 32;
  ctx->data[58] = ctx->bitlen >> 40;
  ctx->data[57] = ctx->bitlen >> 48;
  ctx->data[56] = ctx->bitlen >> 56;
  map_sha256_transform(ctx, ctx->data);
  for (i = 0; i < 4; ++i) {
    hash[i] = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
    hash[i + 4] = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
    hash[i + 8] = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
    hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
    hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
    hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
    hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
    hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
  }
}

// CRC32 Implementation
uint32_t map_crc32_for_byte(uint32_t r) {
  for (int j = 0; j < 8; ++j)
    r = (r & 1 ? 0 : (uint32_t)0xEDB88320L) ^ r >> 1;
  return r ^ (uint32_t)0xFF000000L;
}

uint32_t map_crc32(const void *data, size_t n_bytes) {
  static uint32_t table[0x100];
  if (!*table)
    for (size_t i = 0; i < 0x100; ++i)
      table[i] = map_crc32_for_byte(i);
  uint32_t crc = 0;
  for (size_t i = 0; i < n_bytes; ++i)
    crc = table[(uint8_t)crc ^ ((uint8_t *)data)[i]] ^ crc >> 8;
  return crc;
}

void str_to_ints(int *pInts, size_t NumInts, const char *pStr) {
  const size_t StrSize = strlen(pStr) + 1;
  for (size_t i = 0; i < NumInts; i++) {
    char aBuf[sizeof(int)] = {0, 0, 0, 0};
    for (size_t c = 0; c < sizeof(int) && i * sizeof(int) + c < StrSize; c++)
      aBuf[c] = pStr[i * sizeof(int) + c];
    pInts[i] = ((aBuf[0] + 128) << 24) | ((aBuf[1] + 128) << 16) | ((aBuf[2] + 128) << 8) | (aBuf[3] + 128);
  }
  pInts[NumInts - 1] &= 0xFFFFFF00;
}

static void ints_to_str(const int *pInts, size_t NumInts, char *pStr, size_t MaxLength) {
  size_t p = 0;
  for (size_t i = 0; i < NumInts; i++) {
    int val = pInts[i];
    char aBuf[4];
    aBuf[0] = ((val >> 24) & 0xFF) - 128;
    aBuf[1] = ((val >> 16) & 0xFF) - 128;
    aBuf[2] = ((val >> 8) & 0xFF) - 128;
    aBuf[3] = (val & 0xFF) - 128;
    for (size_t c = 0; c < 4; c++) {
      if (p < MaxLength - 1) {
        pStr[p++] = aBuf[c];
        if (aBuf[c] == '\0') break; // Early exit on null terminator
      }
    }
    if (p > 0 && pStr[p - 1] == '\0') break;
  }
  pStr[MaxLength - 1] = '\0';
}

int round_to_int(float f) {
  if (f >= 0.0f) return (int)(f + 0.5f);
  else return (int)(f - 0.5f);
}

static void on_hammer_hit(mvec2 pos, int type, int cid, void *data) {
  (void)cid;
  demo_exporter_t *exporter = data;
  if (type == PARTICLE_TYPE_HAMMER_HIT)
    if (exporter->num_hammerhits < MAX_HAMMERHITS_PER_TICK) exporter->hammerhits[exporter->num_hammerhits++] = pos;
}

static void snap_world(dd_snapshot_builder *sb, timeline_state_t *ts, SWorldCore *prev, SWorldCore *cur) {
  int next_item_id = cur->m_NumCharacters; // start after reserved player ids

  // do pickups first since they have static ids basically
  for (int i = 0; i < ts->ui->num_pickups; ++i) {
    const SPickup pickup = ts->ui->pickups[i];
    dd_netobj_ddnet_pickup *p = demo_sb_add_item(sb, DD_NETOBJTYPE_DDNETPICKUP, next_item_id++, sizeof(dd_netobj_ddnet_pickup));
    if (p) {
      p->m_X = vgetx(ts->ui->pickup_positions[i]) - MAP_EXPAND32;
      p->m_Y = vgety(ts->ui->pickup_positions[i]) - MAP_EXPAND32;
      p->m_Type = pickup.m_Type;
      p->m_Subtype = pickup.m_Subtype;
      p->m_SwitchNumber = pickup.m_Number;
      p->m_Flags = 0;
      // log_info("DemoExport", "Added pickup id %d at (%d, %d), type %d, subtype %d", next_item_id, p->m_X, p->m_Y, p->m_Type, p->m_Subtype);
    }
  }

  // game info
  dd_netobj_game_info *game_info = demo_sb_add_item(sb, DD_NETOBJTYPE_GAMEINFO, 0, sizeof(dd_netobj_game_info));
  *game_info = (dd_netobj_game_info){0};
  if (cur->m_NumCharacters > 0) {
    SCharacterCore *c = &cur->m_pCharacters[0];
    if (c->m_StartTick != -1) {
      game_info->m_WarmupTimer = -c->m_StartTick;
      game_info->m_GameStateFlags = DD_GAMESTATEFLAG_RACETIME;
    }
  }
  dd_netobj_game_info_ex *game_info_ex = demo_sb_add_item(sb, DD_NETOBJTYPE_GAMEINFOEX, 0, sizeof(dd_netobj_game_info_ex));
  game_info_ex->m_Version = 10;
  game_info_ex->m_Flags = DD_GAMEINFOFLAG_TIMESCORE | DD_GAMEINFOFLAG_GAMETYPE_RACE | DD_GAMEINFOFLAG_GAMETYPE_DDRACE |
                          DD_GAMEINFOFLAG_GAMETYPE_DDNET | DD_GAMEINFOFLAG_UNLIMITED_AMMO | DD_GAMEINFOFLAG_RACE_RECORD_MESSAGE |
                          DD_GAMEINFOFLAG_ALLOW_EYE_WHEEL | DD_GAMEINFOFLAG_ALLOW_HOOK_COLL | DD_GAMEINFOFLAG_ALLOW_ZOOM |
                          DD_GAMEINFOFLAG_BUG_DDRACE_GHOST | DD_GAMEINFOFLAG_BUG_DDRACE_INPUT | DD_GAMEINFOFLAG_PREDICT_DDRACE |
                          DD_GAMEINFOFLAG_PREDICT_DDRACE_TILES | DD_GAMEINFOFLAG_ENTITIES_DDNET | DD_GAMEINFOFLAG_ENTITIES_DDRACE |
                          DD_GAMEINFOFLAG_ENTITIES_RACE | DD_GAMEINFOFLAG_RACE;
  game_info_ex->m_Flags2 = DD_GAMEINFOFLAG2_HUD_DDRACE;

  for (int p = 0; p < cur->m_NumCharacters; ++p) {
    SCharacterCore *c_cur = &cur->m_pCharacters[p];
    SCharacterCore *c_prev = &prev->m_pCharacters[p];

    dd_netobj_client_info *ci = demo_sb_add_item(sb, DD_NETOBJTYPE_CLIENTINFO, p, sizeof(dd_netobj_client_info));
    const char *name = ts->player_tracks[p].player_info.name;
    if (name[0] == '\0') name = "nameless tee";
    str_to_ints(ci->m_aName, 4, name);
    str_to_ints(ci->m_aClan, 3, ts->player_tracks[p].player_info.clan);

    // 3 offset to get the correct name
    if (ts->player_tracks[p].player_info.skin >= 3)
      str_to_ints(ci->m_aSkin, 6, ts->ui->skin_manager.skins[ts->player_tracks[p].player_info.skin - 3].name);
    else *ci->m_aSkin = 0;
    ci->m_Country = 0;
    ci->m_UseCustomColor = ts->player_tracks[p].player_info.use_custom_color;
    ci->m_ColorBody = ts->player_tracks[p].player_info.color_body;
    ci->m_ColorFeet = ts->player_tracks[p].player_info.color_feet;

    dd_netobj_player_info *pi = demo_sb_add_item(sb, DD_NETOBJTYPE_PLAYERINFO, p, sizeof(dd_netobj_player_info));
    pi->m_Latency = 307;
    pi->m_Score = -9999;
    pi->m_Local = 0;
    pi->m_ClientId = p;
    pi->m_Team = 0;

    dd_netobj_ddnet_player *dp = demo_sb_add_item(sb, DD_NETOBJTYPE_DDNETPLAYER, p, sizeof(dd_netobj_ddnet_player));
    dp->m_AuthLevel = 0;
    dp->m_Flags = 0;

    dd_netobj_character *ch = demo_sb_add_item(sb, DD_NETOBJTYPE_CHARACTER, p, sizeof(dd_netobj_character));

    ch->core.m_X = round_to_int(vgetx(c_cur->m_Pos)) - MAP_EXPAND32;
    ch->core.m_Y = round_to_int(vgety(c_cur->m_Pos)) - MAP_EXPAND32;
    ch->core.m_VelX = round_to_int(vgetx(c_cur->m_Vel) * 256.0f);
    ch->core.m_VelY = round_to_int(vgety(c_cur->m_Vel) * 256.0f);
    ch->core.m_HookState = c_cur->m_HookState;
    ch->core.m_HookTick = c_cur->m_HookTick;
    ch->core.m_HookX = round_to_int(vgetx(c_cur->m_HookPos)) - MAP_EXPAND32;
    ch->core.m_HookY = round_to_int(vgety(c_cur->m_HookPos)) - MAP_EXPAND32;
    ch->core.m_HookDx = round_to_int(vgetx(c_cur->m_HookDir) * 256.0f);
    ch->core.m_HookDy = round_to_int(vgety(c_cur->m_HookDir) * 256.0f);
    ch->core.m_HookedPlayer = c_cur->m_HookedPlayer;
    ch->core.m_Jumped = c_cur->m_Jumped;
    ch->core.m_Direction = c_cur->m_Input.m_Direction;
    // setup angle
    float tmp_angle = atan2(c_cur->m_Input.m_TargetY, c_cur->m_Input.m_TargetX);
    if (tmp_angle < -(M_PI / 2.0f)) ch->core.m_Angle = (int)((tmp_angle + (2.0f * M_PI)) * 256.0f);
    else ch->core.m_Angle = (int)(tmp_angle * 256.0f);

    ch->core.m_Tick = cur->m_GameTick;
    ch->m_Emote = 2;

    ch->m_AttackTick = c_cur->m_AttackTick;
    ch->core.m_Direction = c_cur->m_Input.m_Direction;
    ch->m_Weapon = (c_cur->m_DeepFrozen || c_cur->m_FreezeTime > 0 || c_cur->m_LiveFrozen) ? WEAPON_NINJA : c_cur->m_ActiveWeapon;
    ch->m_AmmoCount = 0;
    ch->m_Health = 10;
    ch->m_Armor = 10;
    ch->m_PlayerFlags = 0;

    dd_netobj_ddnet_character *dc = demo_sb_add_item(sb, DD_NETOBJTYPE_DDNETCHARACTER, p, sizeof(dd_netobj_ddnet_character));
    dc->m_Flags = 0;
    if (c_cur->m_Solo) dc->m_Flags |= DD_CHARACTERFLAG_SOLO;
    if (c_cur->m_EndlessHook) dc->m_Flags |= DD_CHARACTERFLAG_ENDLESS_HOOK;
    if (c_cur->m_CollisionDisabled) dc->m_Flags |= DD_CHARACTERFLAG_COLLISION_DISABLED;
    if (c_cur->m_HookHitDisabled) dc->m_Flags |= DD_CHARACTERFLAG_HOOK_HIT_DISABLED;
    if (c_cur->m_EndlessJump) dc->m_Flags |= DD_CHARACTERFLAG_ENDLESS_JUMP;
    if (c_cur->m_Jetpack) dc->m_Flags |= DD_CHARACTERFLAG_JETPACK;
    if (c_cur->m_HammerHitDisabled) dc->m_Flags |= DD_CHARACTERFLAG_HAMMER_HIT_DISABLED;
    if (c_cur->m_ShotgunHitDisabled) dc->m_Flags |= DD_CHARACTERFLAG_SHOTGUN_HIT_DISABLED;
    if (c_cur->m_GrenadeHitDisabled) dc->m_Flags |= DD_CHARACTERFLAG_GRENADE_HIT_DISABLED;
    if (c_cur->m_LaserHitDisabled) dc->m_Flags |= DD_CHARACTERFLAG_LASER_HIT_DISABLED;
    if (c_cur->m_HasTelegunGun) dc->m_Flags |= DD_CHARACTERFLAG_TELEGUN_GUN;
    if (c_cur->m_HasTelegunGrenade) dc->m_Flags |= DD_CHARACTERFLAG_TELEGUN_GRENADE;
    if (c_cur->m_HasTelegunLaser) dc->m_Flags |= DD_CHARACTERFLAG_TELEGUN_LASER;
    if (c_cur->m_aWeaponGot[WEAPON_HAMMER]) dc->m_Flags |= DD_CHARACTERFLAG_WEAPON_HAMMER;
    if (c_cur->m_aWeaponGot[WEAPON_GUN]) dc->m_Flags |= DD_CHARACTERFLAG_WEAPON_GUN;
    if (c_cur->m_aWeaponGot[WEAPON_SHOTGUN]) dc->m_Flags |= DD_CHARACTERFLAG_WEAPON_SHOTGUN;
    if (c_cur->m_aWeaponGot[WEAPON_GRENADE]) dc->m_Flags |= DD_CHARACTERFLAG_WEAPON_GRENADE;
    if (c_cur->m_aWeaponGot[WEAPON_LASER]) dc->m_Flags |= DD_CHARACTERFLAG_WEAPON_LASER;
    if (c_cur->m_ActiveWeapon == WEAPON_NINJA) dc->m_Flags |= DD_CHARACTERFLAG_WEAPON_NINJA;
    if (c_cur->m_LiveFrozen) dc->m_Flags |= DD_CHARACTERFLAG_MOVEMENTS_DISABLED;

    dc->m_Jumps = c_cur->m_Jumps;
    dc->m_TeleCheckpoint = c_cur->m_TeleCheckpoint;
    dc->m_StrongWeakId = 0; // pCharacter->m_StrongWeakId;
    dc->m_JumpedTotal = c_cur->m_JumpedTotal;
    dc->m_NinjaActivationTick = c_cur->m_Ninja.m_ActivationTick;

    dc->m_FreezeStart = c_cur->m_FreezeStart;
    dc->m_FreezeEnd = c_cur->m_DeepFrozen ? -1 : c_cur->m_FreezeTime == 0 ? 0
                                                                          : cur->m_GameTick + c_cur->m_FreezeTime;

    if (c_cur->m_IsInFreeze) {
      dc->m_Flags |= DD_CHARACTERFLAG_IN_FREEZE;
    }
    dc->m_TargetX = c_cur->m_Input.m_TargetX;
    dc->m_TargetY = c_cur->m_Input.m_TargetY;

    if (c_cur->m_RespawnDelay > c_prev->m_RespawnDelay) {
      dd_netevent_sound_world *nss;
      nss = demo_sb_add_item(sb, DD_NETEVENTTYPE_SOUNDWORLD, next_item_id++, sizeof(dd_netevent_sound_world));
      nss->common.m_X = vgetx(c_cur->m_Pos) - MAP_EXPAND32;
      nss->common.m_Y = vgety(c_cur->m_Pos) - MAP_EXPAND32;
      nss->m_SoundId = DD_SOUND_PLAYER_SPAWN;
      nss = demo_sb_add_item(sb, DD_NETEVENTTYPE_SOUNDWORLD, next_item_id++, sizeof(dd_netevent_sound_world));
      nss->common.m_X = vgetx(c_prev->m_Pos) - MAP_EXPAND32;
      nss->common.m_Y = vgety(c_prev->m_Pos) - MAP_EXPAND32;
      nss->m_SoundId = DD_SOUND_PLAYER_DIE;

      dd_netevent_spawn *ns = demo_sb_add_item(sb, DD_NETEVENTTYPE_SPAWN, next_item_id++, sizeof(dd_netevent_spawn));
      ns->common.m_X = vgetx(c_cur->m_Pos) - MAP_EXPAND32;
      ns->common.m_Y = vgety(c_cur->m_Pos) - MAP_EXPAND32;

      dd_netevent_death *nd = demo_sb_add_item(sb, DD_NETEVENTTYPE_DEATH, next_item_id++, sizeof(dd_netevent_death));
      nd->common.m_X = vgetx(c_prev->m_Pos) - MAP_EXPAND32;
      nd->common.m_Y = vgety(c_prev->m_Pos) - MAP_EXPAND32;
      nd->m_ClientId = c_cur->m_Id;
    }
    if (c_prev->m_HookState != HOOK_GRABBED && c_cur->m_HookState == HOOK_GRABBED) {
      dd_netevent_sound_world *nhs = demo_sb_add_item(sb, DD_NETEVENTTYPE_SOUNDWORLD, next_item_id++, sizeof(dd_netevent_sound_world));
      nhs->common.m_X = vgetx(c_cur->m_Pos) - MAP_EXPAND32;
      nhs->common.m_Y = vgety(c_cur->m_Pos) - MAP_EXPAND32;
      if (c_prev->m_HookedPlayer == -1 && c_cur->m_HookedPlayer != -1) nhs->m_SoundId = DD_SOUND_HOOK_ATTACH_PLAYER;
      else nhs->m_SoundId = DD_SOUND_HOOK_ATTACH_GROUND;
    }
    if (c_cur->m_Jumped && c_cur->m_Grounded) {
      dd_netevent_sound_world *njs = demo_sb_add_item(sb, DD_NETEVENTTYPE_SOUNDWORLD, next_item_id++, sizeof(dd_netevent_sound_world));
      njs->common.m_X = vgetx(c_cur->m_Pos) - MAP_EXPAND32;
      njs->common.m_Y = vgety(c_cur->m_Pos) - MAP_EXPAND32;
      njs->m_SoundId = DD_SOUND_PLAYER_JUMP;
    }
    if (c_cur->m_ReloadTimer > c_prev->m_ReloadTimer) {
      if (c_cur->m_ActiveWeapon <= 1) {
        dd_netevent_sound_world *nhs = demo_sb_add_item(sb, DD_NETEVENTTYPE_SOUNDWORLD, next_item_id++, sizeof(dd_netevent_sound_world));
        nhs->common.m_X = vgetx(c_cur->m_Pos) - MAP_EXPAND32;
        nhs->common.m_Y = vgety(c_cur->m_Pos) - MAP_EXPAND32;
        nhs->m_SoundId = c_cur->m_ActiveWeapon == WEAPON_HAMMER ? DD_SOUND_HAMMER_FIRE : DD_SOUND_GUN_FIRE;
      }
    }
  }

  for (int i = 0; i < ts->ui->demo_exporter.num_hammerhits; ++i) {
    dd_netevent_hammer_hit *nhh = demo_sb_add_item(sb, DD_NETEVENTTYPE_HAMMERHIT, next_item_id++, sizeof(dd_netevent_hammer_hit));
    nhh->common.m_X = vgetx(ts->ui->demo_exporter.hammerhits[i]) - MAP_EXPAND32;
    nhh->common.m_Y = vgety(ts->ui->demo_exporter.hammerhits[i]) - MAP_EXPAND32;
  }

  // do entities
  for (SProjectile *proj = (SProjectile *)cur->m_apFirstEntityTypes[WORLD_ENTTYPE_PROJECTILE]; proj;
       proj = (SProjectile *)proj->m_Base.m_pNextTypeEntity) {
    dd_netobj_ddnet_projectile *p = demo_sb_add_item(sb, DD_NETOBJTYPE_DDNETPROJECTILE, next_item_id++, sizeof(dd_netobj_ddnet_projectile));
    if (p) {
      int Flags = 0;
      if (proj->m_Bouncing & 1) {
        Flags |= DD_PROJECTILEFLAG_BOUNCE_HORIZONTAL;
      }
      if (proj->m_Bouncing & 2) {
        Flags |= DD_PROJECTILEFLAG_BOUNCE_VERTICAL;
      }
      if (proj->m_Explosive) {
        Flags |= DD_PROJECTILEFLAG_EXPLOSIVE;
      }
      if (proj->m_Freeze) {
        Flags |= DD_PROJECTILEFLAG_FREEZE;
      }
      Flags |= DD_PROJECTILEFLAG_NORMALIZE_VEL;
      p->m_VelX = round_to_int(vgetx(proj->m_Direction) * 1e6f);
      p->m_VelY = round_to_int(vgety(proj->m_Direction) * 1e6f);
      p->m_X = round_to_int((vgetx(proj->m_Base.m_Pos) - MAP_EXPAND32) * 100.0f);
      p->m_Y = round_to_int((vgety(proj->m_Base.m_Pos) - MAP_EXPAND32) * 100.0f);
      p->m_Type = proj->m_Type;
      p->m_StartTick = proj->m_StartTick;
      p->m_Owner = proj->m_Owner;
      p->m_Flags = Flags;
      p->m_SwitchNumber = proj->m_Base.m_Number;
      p->m_TuneZone = 0;
    }

    const mvec2 pos = prj_get_pos(proj, (cur->m_GameTick - proj->m_StartTick) / (float)GAME_TICK_SPEED);
    const mvec2 next_pos = prj_get_pos(proj, (cur->m_GameTick - proj->m_StartTick + 1) / (float)GAME_TICK_SPEED);
    if (proj->m_Owner >= 0 && proj->m_Base.m_Spawned) {
      dd_netevent_sound_world *nf = demo_sb_add_item(sb, DD_NETEVENTTYPE_SOUNDWORLD, next_item_id++, sizeof(dd_netevent_sound_world));
      nf->common.m_X = vgetx(pos) - MAP_EXPAND32;
      nf->common.m_Y = vgety(pos) - MAP_EXPAND32;
      nf->m_SoundId = DD_SOUND_GRENADE_FIRE;
    }
    if (proj->m_Explosive) {
      mvec2 out, _out;
      if (intersect_line(proj->m_Base.m_pCollision, pos, next_pos, &out, &_out)) {
        dd_netevent_explosion *ne = demo_sb_add_item(sb, DD_NETEVENTTYPE_EXPLOSION, next_item_id++, sizeof(dd_netevent_explosion));
        ne->common.m_X = vgetx(out) - MAP_EXPAND32;
        ne->common.m_Y = vgety(out) - MAP_EXPAND32;
        dd_netevent_sound_world *nes = demo_sb_add_item(sb, DD_NETEVENTTYPE_SOUNDWORLD, next_item_id++, sizeof(dd_netevent_sound_world));
        nes->common.m_X = vgetx(out) - MAP_EXPAND32;
        nes->common.m_Y = vgety(out) - MAP_EXPAND32;
        nes->m_SoundId = DD_SOUND_GRENADE_EXPLODE;
      }
      if (proj->m_LifeSpan <= 0) {
        dd_netevent_explosion *ne = demo_sb_add_item(sb, DD_NETEVENTTYPE_EXPLOSION, next_item_id++, sizeof(dd_netevent_explosion));
        ne->common.m_X = vgetx(pos) - MAP_EXPAND32;
        ne->common.m_Y = vgety(pos) - MAP_EXPAND32;
        dd_netevent_sound_world *nes = demo_sb_add_item(sb, DD_NETEVENTTYPE_SOUNDWORLD, next_item_id++, sizeof(dd_netevent_sound_world));
        nes->common.m_X = vgetx(pos) - MAP_EXPAND32;
        nes->common.m_Y = vgety(pos) - MAP_EXPAND32;
        nes->m_SoundId = DD_SOUND_GRENADE_EXPLODE;
      }
    }
  }

  for (SLaser *laser = (SLaser *)cur->m_apFirstEntityTypes[WORLD_ENTTYPE_LASER]; laser; laser = (SLaser *)laser->m_Base.m_pNextTypeEntity) {
    dd_netobj_ddnet_laser *l = demo_sb_add_item(sb, DD_NETOBJTYPE_DDNETLASER, next_item_id++, sizeof(dd_netobj_ddnet_laser));
    // laser
    if (l) {
      l->m_ToX = (int)vgetx(laser->m_Base.m_Pos) - MAP_EXPAND32;
      l->m_ToY = (int)vgety(laser->m_Base.m_Pos) - MAP_EXPAND32;
      l->m_FromX = (int)vgetx(laser->m_From) - MAP_EXPAND32;
      l->m_FromY = (int)vgety(laser->m_From) - MAP_EXPAND32;
      l->m_StartTick = laser->m_EvalTick;
      l->m_Owner = laser->m_Owner;
      l->m_Type = laser->m_Type == DD_WEAPON_LASER ? DD_LASERTYPE_RIFLE : DD_LASERTYPE_SHOTGUN;
      l->m_Subtype = -1;
      l->m_SwitchNumber = laser->m_Base.m_Number;
      l->m_Flags = 0;
    }
    // sounds
    if (laser->m_Owner >= 0 && laser->m_Base.m_Spawned) {
      dd_netevent_sound_world *nlss = demo_sb_add_item(sb, DD_NETEVENTTYPE_SOUNDWORLD, next_item_id++, sizeof(dd_netevent_sound_world));
      if (nlss) {
        nlss->common.m_X = vgetx(laser->m_From) - MAP_EXPAND32;
        nlss->common.m_Y = vgety(laser->m_From) - MAP_EXPAND32;
        nlss->m_SoundId = laser->m_Type == DD_WEAPON_LASER ? DD_SOUND_LASER_FIRE : DD_SOUND_SHOTGUN_FIRE;
      }
    } else if (laser->m_EvalTick >= cur->m_GameTick) {
      dd_netevent_sound_world *nlbs = demo_sb_add_item(sb, DD_NETEVENTTYPE_SOUNDWORLD, next_item_id++, sizeof(dd_netevent_sound_world));
      if (nlbs) {
        nlbs->common.m_X = vgetx(laser->m_From) - MAP_EXPAND32;
        nlbs->common.m_Y = vgety(laser->m_From) - MAP_EXPAND32;
        nlbs->m_SoundId = DD_SOUND_LASER_BOUNCE;
      }
    }
  }
}

int import_demo(ui_handler_t *ui, const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return -1;
  dd_demo_reader *dr = demo_r_create();
  if (!demo_r_open(dr, f)) {
    printf("[import_demo] demo_r_open FAILED!\n");
    demo_r_destroy(&dr);
    fclose(f);
    return -1;
  }
  printf("[import_demo] demo_r_open SUCCESS!\n");

  dd_demo_info *info = demo_r_get_info(dr);
  if (info->map_size > 0) {
    long map_pos = ftell(f) - info->map_size;
    unsigned char *map_buf = malloc(info->map_size);
    fseek(f, map_pos, SEEK_SET);
    if (fread(map_buf, 1, info->map_size, f) == info->map_size) {
      on_map_load_mem(ui->gfx_handler, map_buf, info->map_size);
    }
    // Do not free map_buf here, physics_handler takes ownership and will free it in physics_free.
    fseek(f, map_pos + info->map_size, SEEK_SET);

    // Enable Dead Reckoning using the map we just loaded
    dd_phys_tuning tuning;
    dd_phys_init_tuning(&tuning);
    demo_r_enable_phys(dr, NULL, &tuning);
    dr->track_phys = false;
  }

  SCharacterCore *states[64] = {0};
  SPlayerInput *inputs[64] = {0};
  int counts_char[64] = {0};
  int counts_input[64] = {0};
  int caps[64] = {0};
  int max_tick = 0;
  int first_tick = -1;
  char player_names[64][16] = {0};

  int processed_chunks = 0;
  dd_demo_chunk chunk;
  uint8_t unpacked_snap[DD_SNAPSHOT_MAX_SIZE];

  int last_processed_norm_tick = -1;
  while (demo_r_next_chunk(dr, &chunk)) {
    if (processed_chunks++ < 20) {
      printf("CHUNK type=%d tick=%d\n", chunk.type, chunk.tick);
    }
    if (first_tick == -1) first_tick = chunk.tick;
    int norm_tick = chunk.tick - first_tick;

    const dd_snapshot *current_snap = NULL;
    if (chunk.type == DD_CHUNK_SNAP) {
      current_snap = (const dd_snapshot *)chunk.data;
    } else if (chunk.type == DD_CHUNK_SNAP_DELTA) {
      int unpacked_size = demo_r_unpack_delta(dr, chunk.data, unpacked_snap);
      if (unpacked_size > 0) current_snap = (const dd_snapshot *)unpacked_snap;
    }

    if (chunk.type == DD_CHUNK_SNAP || chunk.type == DD_CHUNK_SNAP_DELTA || chunk.type == DD_CHUNK_TICK_MARKER) {
      if (last_processed_norm_tick == -1) last_processed_norm_tick = norm_tick - 1;

      while (last_processed_norm_tick < norm_tick) {
        last_processed_norm_tick++;
        int actual_tick = last_processed_norm_tick + first_tick;
        if (last_processed_norm_tick > max_tick) max_tick = last_processed_norm_tick;

        // Propagate inputs to physics players for this tick
        for (int cid = 0; cid < 64; ++cid) {
          if (dr->phys.last_base_tick[cid] != -1) {
            if (last_processed_norm_tick >= caps[cid]) {
              int old_cap = caps[cid];
              caps[cid] = (last_processed_norm_tick + 1) * 2;
              if (caps[cid] < 128) caps[cid] = 128;
              states[cid] = realloc(states[cid], caps[cid] * sizeof(SCharacterCore));
              memset(states[cid] + old_cap, 0, (caps[cid] - old_cap) * sizeof(SCharacterCore));
              inputs[cid] = realloc(inputs[cid], caps[cid] * sizeof(SPlayerInput));
              memset(inputs[cid] + old_cap, 0, (caps[cid] - old_cap) * sizeof(SPlayerInput));
            }

            if (last_processed_norm_tick > 0) {
              inputs[cid][last_processed_norm_tick] = inputs[cid][last_processed_norm_tick - 1];
            }

            SPlayerInput *inp = &inputs[cid][last_processed_norm_tick];
            dr->phys.players[cid].input.m_Direction = inp->m_Direction;
            dr->phys.players[cid].input.m_TargetX = inp->m_TargetX;
            dr->phys.players[cid].input.m_TargetY = inp->m_TargetY;
            dr->phys.players[cid].input.m_Jump = inp->m_Jump;
            dr->phys.players[cid].input.m_Fire = inp->m_Fire;
            dr->phys.players[cid].input.m_Hook = inp->m_Hook;
            dr->phys.players[cid].input.m_PlayerFlags = inp->m_Flags;
            dr->phys.players[cid].input.m_WantedWeapon = inp->m_WantedWeapon;
            dr->phys.players[cid].input.m_NextWeapon = 0;
            dr->phys.players[cid].input.m_PrevWeapon = 0;
          }
        }

        if (last_processed_norm_tick == norm_tick && current_snap) {
          dd_demo_phys_update(&dr->phys, current_snap, actual_tick, &dr->tuning, &dr->map);
          dr->current_tick = actual_tick;
        } else {
          dd_demo_phys_advance_to(&dr->phys, actual_tick, &dr->tuning, &dr->map);
          dr->current_tick = actual_tick;
        }

        uint8_t phys_snap_buf[DD_SNAPSHOT_MAX_SIZE];
        int snap_size = demo_r_get_phys_snap(dr, phys_snap_buf);
        if (snap_size > 8) {
          static int ok_prints = 0;
          if (ok_prints++ < 20) printf("snap_size > 8! (%d)\n", snap_size);
        }
        if (snap_size > 0) {
          const dd_snapshot *phys_snap = (const dd_snapshot *)phys_snap_buf;
          for (int i = 0; i < phys_snap->num_items; i++) {
            const dd_snap_item *item = dd_snap_get_item(phys_snap, i);
            int item_type = dd_snap_item_type(item);
            int cid = dd_snap_item_id(item);
            if (cid >= 0 && cid < 64) {
              if (last_processed_norm_tick >= caps[cid]) {
                int old_cap = caps[cid];
                caps[cid] = (last_processed_norm_tick + 1) * 2;
                if (caps[cid] < 128) caps[cid] = 128;
                states[cid] = realloc(states[cid], caps[cid] * sizeof(SCharacterCore));
                memset(states[cid] + old_cap, 0, (caps[cid] - old_cap) * sizeof(SCharacterCore));
                inputs[cid] = realloc(inputs[cid], caps[cid] * sizeof(SPlayerInput));
                memset(inputs[cid] + old_cap, 0, (caps[cid] - old_cap) * sizeof(SPlayerInput));
              }

              if (item_type == DD_NETOBJTYPE_CHARACTER) {
                printf("Found character %d at norm_tick %d\n", cid, last_processed_norm_tick);
                const dd_netobj_character *character = (const dd_netobj_character *)dd_snap_item_data(item);

                SCharacterCore *c = &states[cid][last_processed_norm_tick];
                if (last_processed_norm_tick > 0) {
                  *c = states[cid][last_processed_norm_tick - 1];
                } else {
                  memset(c, 0, sizeof(SCharacterCore));
                }

                c->m_Pos[0] = character->core.m_X + MAP_EXPAND32;
                c->m_Pos[1] = character->core.m_Y + MAP_EXPAND32;
                c->m_Vel[0] = character->core.m_VelX / 256.0f;
                c->m_Vel[1] = character->core.m_VelY / 256.0f;
                c->m_HookPos[0] = character->core.m_HookX + MAP_EXPAND32;
                c->m_HookPos[1] = character->core.m_HookY + MAP_EXPAND32;
                c->m_HookDir[0] = character->core.m_HookDx / 256.0f;
                c->m_HookDir[1] = character->core.m_HookDy / 256.0f;
                c->m_HookState = character->core.m_HookState;
                c->m_HookTick = character->core.m_HookTick;
                c->m_HookedPlayer = character->core.m_HookedPlayer;
                c->m_Jumped = character->core.m_Jumped;
                
                c->m_Jumps = dr->phys.players[cid].jumps;
                c->m_JumpedTotal = dr->phys.players[cid].jumped_total;
                c->m_Solo = dr->phys.players[cid].solo;
                c->m_Jetpack = dr->phys.players[cid].jetpack;
                c->m_CollisionDisabled = dr->phys.players[cid].collision_disabled;
                c->m_EndlessHook = dr->phys.players[cid].endless_hook;
                c->m_EndlessJump = dr->phys.players[cid].endless_jump;

                float angle_rad = character->core.m_Angle / 256.0f;
                int tx = (int)(cosf(angle_rad) * 100.0f);
                int ty = (int)(sinf(angle_rad) * 100.0f);

                SPlayerInput *inp = &inputs[cid][last_processed_norm_tick];
                inp->m_Direction = character->core.m_Direction;
                inp->m_TargetX = tx;
                inp->m_TargetY = ty;
                inp->m_Hook = character->core.m_HookState != 0;

                c->m_Input = *inp;

                counts_char[cid] = last_processed_norm_tick + 1;
                counts_input[cid] = last_processed_norm_tick + 1;
                printf("Updated counts_char[%d] to %d\n", cid, counts_char[cid]);
              }
            }
          }
        }
      }

      if (current_snap) {
        for (int i = 0; i < current_snap->num_items; i++) {
          const dd_snap_item *item = dd_snap_get_item(current_snap, i);
          int item_type = dd_snap_item_type(item);
          int cid = dd_snap_item_id(item);
          if (cid >= 0 && cid < 64) {
            if (last_processed_norm_tick >= caps[cid]) {
              int old_cap = caps[cid];
              caps[cid] = (last_processed_norm_tick + 1) * 2;
              if (caps[cid] < 128) caps[cid] = 128;
              states[cid] = (SCharacterCore *)realloc(states[cid], caps[cid] * sizeof(SCharacterCore));
              inputs[cid] = (SPlayerInput *)realloc(inputs[cid], caps[cid] * sizeof(SPlayerInput));
              memset(states[cid] + old_cap, 0, (caps[cid] - old_cap) * sizeof(SCharacterCore));
              memset(inputs[cid] + old_cap, 0, (caps[cid] - old_cap) * sizeof(SPlayerInput));
            }
            if (item_type == DD_NETOBJTYPE_CLIENTINFO) {
              const dd_netobj_client_info *ci = (const dd_netobj_client_info *)dd_snap_item_data(item);
              ints_to_str(ci->m_aName, 4, player_names[cid], 16);
            } else if (item_type == DD_NETOBJTYPE_CHARACTER) {
              const dd_netobj_character *char_real = (const dd_netobj_character *)dd_snap_item_data(item);
              SCharacterCore *c = &states[cid][last_processed_norm_tick];
              SPlayerInput *inp = &inputs[cid][last_processed_norm_tick];
              c->m_ActiveWeapon = char_real->m_Weapon;
              c->m_AttackTick = char_real->m_AttackTick;
              inp->m_WantedWeapon = char_real->m_Weapon;
              
              inp->m_Direction = char_real->core.m_Direction;
              inp->m_Hook = char_real->core.m_HookState != 0;
              
              float angle_rad = char_real->core.m_Angle / 256.0f;
              inp->m_TargetX = (int)(cosf(angle_rad) * 100.0f);
              inp->m_TargetY = (int)(sinf(angle_rad) * 100.0f);
              
              if (last_processed_norm_tick > 0) {
                if (char_real->core.m_Jumped != states[cid][last_processed_norm_tick - 1].m_Jumped && char_real->core.m_Jumped > 0) {
                  inp->m_Jump = 1;
                } else {
                  inp->m_Jump = 0;
                }
              }
              
              if (char_real->m_PlayerFlags & 1) set_flag_sit(inp, 1);
              else set_flag_sit(inp, 0);
              c->m_Input = *inp;
            } else if (item_type == DD_NETOBJTYPE_DDNETCHARACTER) {
              const dd_netobj_ddnet_character *ddc = (const dd_netobj_ddnet_character *)dd_snap_item_data(item);
              SCharacterCore *c = &states[cid][last_processed_norm_tick];
              c->m_Jumps = ddc->m_Jumps;
              c->m_JumpedTotal = ddc->m_JumpedTotal;
              c->m_TeleCheckpoint = ddc->m_TeleCheckpoint;
            } else if (item_type == DD_NETOBJTYPE_PLAYERINPUT) {
              const dd_netobj_player_input *pi = (const dd_netobj_player_input *)dd_snap_item_data(item);
              SPlayerInput *inp = &inputs[cid][last_processed_norm_tick];
              inp->m_Direction = pi->m_Direction;
              inp->m_TargetX = pi->m_TargetX;
              inp->m_TargetY = pi->m_TargetY;
              inp->m_Jump = pi->m_Jump;
              inp->m_Fire = pi->m_Fire;
              inp->m_Hook = pi->m_Hook;
              inp->m_WantedWeapon = pi->m_WantedWeapon;

              dr->phys.players[cid].input.m_Direction = pi->m_Direction;
              dr->phys.players[cid].input.m_TargetX = pi->m_TargetX;
              dr->phys.players[cid].input.m_TargetY = pi->m_TargetY;
              dr->phys.players[cid].input.m_Jump = pi->m_Jump;
              dr->phys.players[cid].input.m_Fire = pi->m_Fire;
              dr->phys.players[cid].input.m_Hook = pi->m_Hook;
              dr->phys.players[cid].input.m_PlayerFlags = pi->m_PlayerFlags;
              dr->phys.players[cid].input.m_WantedWeapon = pi->m_WantedWeapon;
              dr->phys.players[cid].input.m_NextWeapon = pi->m_NextWeapon;
              dr->phys.players[cid].input.m_PrevWeapon = pi->m_PrevWeapon;

              SCharacterCore *c = &states[cid][last_processed_norm_tick];
              c->m_Input = *inp;

              counts_input[cid] = last_processed_norm_tick + 1;
            }
          }
        }
      }
    } else if (chunk.type == DD_CHUNK_MSG) {
      const uint8_t *p_data = chunk.data;
      const uint8_t *p_end = chunk.data + chunk.size;
      int msg_info;
      p_data = dd_variable_int_unpack(p_data, &msg_info, p_end - p_data);
      if (p_data) {
        int msg_type = msg_info >> 1;
        bool is_sys = msg_info & 1;

        if (!is_sys) {
          net_event_t ev = {0};
          ev.tick = norm_tick;

          if (msg_type == DD_NETMSGTYPE_SV_CHAT) {
            ev.type = NET_EVENT_CHAT;
            p_data = dd_variable_int_unpack(p_data, &ev.team, p_end - p_data);
            p_data = dd_variable_int_unpack(p_data, &ev.client_id, p_end - p_data);
            if (p_data) {
              strncpy(ev.message, (const char *)p_data, sizeof(ev.message) - 1);
            }
            net_events_add(&ui->timeline, ev);
          } else if (msg_type == DD_NETMSGTYPE_SV_KILLMSG) {
            ev.type = NET_EVENT_KILLMSG;
            p_data = dd_variable_int_unpack(p_data, &ev.killer, p_end - p_data);
            p_data = dd_variable_int_unpack(p_data, &ev.victim, p_end - p_data);
            p_data = dd_variable_int_unpack(p_data, &ev.weapon, p_end - p_data);
            p_data = dd_variable_int_unpack(p_data, &ev.mode_special, p_end - p_data);
            net_events_add(&ui->timeline, ev);
          } else if (msg_type == DD_NETMSGTYPE_SV_SOUNDGLOBAL) {
            ev.type = NET_EVENT_SOUND_GLOBAL;
            p_data = dd_variable_int_unpack(p_data, &ev.sound_id, p_end - p_data);
            net_events_add(&ui->timeline, ev);
          }
        }
      }
    }
  }

  demo_r_destroy(&dr);
  fclose(f);

  net_events_sort(&ui->timeline);

  int tracks_to_add = 0;
  for (int cid = 0; cid < 64; ++cid) {
    int max_count = counts_char[cid] > counts_input[cid] ? counts_char[cid] : counts_input[cid];
    if (max_count > 0 && cid >= tracks_to_add) tracks_to_add = cid + 1;
  }

  if (tracks_to_add > 0) {
    model_add_new_track(&ui->timeline, &ui->gfx_handler->physics_handler, tracks_to_add);
  }

  int tracks_added = 0;
  for (int cid = 0; cid < 64; ++cid) {
    int max_count = counts_char[cid] > counts_input[cid] ? counts_char[cid] : counts_input[cid];
    if (max_count > 0) {
      player_track_t *track = &ui->timeline.player_tracks[cid];
      if (max_tick >= caps[cid]) {
        int old_cap = caps[cid];
        caps[cid] = max_tick + 1;
        states[cid] = realloc(states[cid], caps[cid] * sizeof(SCharacterCore));
        memset(states[cid] + old_cap, 0, (caps[cid] - old_cap) * sizeof(SCharacterCore));
        inputs[cid] = realloc(inputs[cid], caps[cid] * sizeof(SPlayerInput));
        memset(inputs[cid] + old_cap, 0, (caps[cid] - old_cap) * sizeof(SPlayerInput));
      }
      for (int fill = counts_char[cid]; fill <= max_tick; ++fill) {
        if (fill > 0) states[cid][fill] = states[cid][fill - 1];
        else memset(&states[cid][fill], 0, sizeof(SCharacterCore));
      }
      for (int fill = counts_input[cid]; fill <= max_tick; ++fill) {
        if (fill > 0) inputs[cid][fill] = inputs[cid][fill - 1];
        else memset(&inputs[cid][fill], 0, sizeof(SPlayerInput));
      }
      
      // Interpolate TargetX and TargetY for smooth visualization
      int first_valid_tick = 0;
      while (first_valid_tick <= max_tick && vgetx(states[cid][first_valid_tick].m_Pos) == 0 && vgety(states[cid][first_valid_tick].m_Pos) == 0) {
        first_valid_tick++;
      }
      
      int last_change_tick = first_valid_tick;
      for (int t = first_valid_tick + 1; t <= max_tick; ++t) {
        if (inputs[cid][t].m_TargetX != inputs[cid][last_change_tick].m_TargetX ||
            inputs[cid][t].m_TargetY != inputs[cid][last_change_tick].m_TargetY) {
          int steps = t - last_change_tick;
          float start_x = inputs[cid][last_change_tick].m_TargetX;
          float start_y = inputs[cid][last_change_tick].m_TargetY;
          float end_x = inputs[cid][t].m_TargetX;
          float end_y = inputs[cid][t].m_TargetY;
          
          for (int i = 1; i < steps; ++i) {
            float t_lerp = (float)i / steps;
            inputs[cid][last_change_tick + i].m_TargetX = (int)(start_x + (end_x - start_x) * t_lerp);
            inputs[cid][last_change_tick + i].m_TargetY = (int)(start_y + (end_y - start_y) * t_lerp);
            states[cid][last_change_tick + i].m_Input.m_TargetX = inputs[cid][last_change_tick + i].m_TargetX;
            states[cid][last_change_tick + i].m_Input.m_TargetY = inputs[cid][last_change_tick + i].m_TargetY;
          }
          last_change_tick = t;
        }
      }

      // Ensure m_PrevPos is correctly set for visual intra-tick interpolation
      states[cid][0].m_PrevPos = states[cid][0].m_Pos;
      for (int fill = 1; fill <= max_tick; ++fill) {
        states[cid][fill].m_PrevPos = states[cid][fill - 1].m_Pos;
      }

      track->demo_data.states = states[cid];
      track->demo_data.num_states = max_tick + 1;

      snippet_t snippet = {0};
      snippet.id = ui->timeline.next_snippet_id++;
      snippet.type = SNIPPET_TYPE_CHARACTER;
      snippet.start_tick = 0;
      snippet.end_tick = max_tick + 1;
      snippet.layer = 0;
      snippet.is_active = true;
      snippet.character_count = max_tick + 1;
      snippet.characters = states[cid];
      snippet.input_count = max_tick + 1;
      snippet.inputs = inputs[cid];
      model_insert_snippet_into_track(track, &snippet);
      
      states[cid] = NULL;
      inputs[cid] = NULL;

      if (player_names[cid][0] != '\0') {
        strncpy(track->player_info.name, player_names[cid], sizeof(track->player_info.name) - 1);
      } else {
        snprintf(track->player_info.name, sizeof(track->player_info.name), "Demo %d", cid);
      }

      tracks_added++;
    } else {
      if (states[cid]) free(states[cid]);
      if (inputs[cid]) free(inputs[cid]);
    }
  }

  if (tracks_added > 0) {
    ui->timeline.selected_player_track_index = 0;
  }

  printf("tracks_added: %d\n", tracks_added);
  fflush(stdout);
  exit(0);
  return tracks_added > 0 ? 0 : -1;
}

int export_to_demo(ui_handler_t *ui, const char *path, const char *map_name, int ticks) {
  // set up demo things
  void *map_data = ui->gfx_handler->physics_handler.collision.m_MapData._map_file_data;
  size_t map_size = ui->gfx_handler->physics_handler.collision.m_MapData._map_file_size;
  uint32_t map_crc = map_crc32(map_data, map_size);
  uint8_t map_sha256[32];
  SHA256_CTX ctx = {0};
  map_sha256_init(&ctx);
  map_sha256_update(&ctx, map_data, map_size);
  map_sha256_final(&ctx, map_sha256);

  dd_demo_writer *writer = demo_w_create();
  FILE *f_demo = fs_open(path, "wb");
  if (!writer || !f_demo) {
    log_error(LOG_SOURCE, "Error: Could not create demo writer or open output file.");
    return 1;
  }

  demo_w_begin(writer, f_demo, map_name, map_crc, "Race");
  demo_w_write_map(writer, map_sha256, map_data, map_size);

  dd_snapshot_builder *sb = demo_sb_create();
  uint8_t snap_buf[DD_SNAPSHOT_MAX_SIZE];

  // empty previous and current worlds
  SWorldCore prev = wc_empty();
  SWorldCore cur = wc_empty();

  // get initial worlds
  model_get_world_state_at_tick(&ui->timeline, 0, &cur, false);
  wc_copy_world(&prev, &cur);
  cur.user_data = &ui->demo_exporter;
  cur.particle = on_hammer_hit;

  for (int t = 0; t < ticks; ++t) {
    demo_sb_clear(sb);

    // Apply character states from snippets before ticking (initial state for this frame)
    bool all_demo = true;
    for (int i = 0; i < cur.m_NumCharacters; ++i) {
      SCharacterCore char_state;
      if (!model_get_character_at_tick(&ui->timeline, i, cur.m_GameTick + 1, &char_state)) {
        all_demo = false;
        break;
      }
    }

    if (all_demo) {
      cur.m_GameTick++;
      for (int i = 0; i < cur.m_NumCharacters; ++i) {
        SCharacterCore char_state;
        model_get_character_at_tick(&ui->timeline, i, cur.m_GameTick, &char_state);
        cur.m_pCharacters[i] = char_state;
        cur.m_pCharacters[i].m_pWorld = &cur;
        cur.m_pCharacters[i].m_pCollision = cur.m_pCollision;
        cur.m_pCharacters[i].m_pTuning = &cur.m_pTunings[0];
        cc_calc_indices(&cur.m_pCharacters[i]);
      }
    } else {
      for (int i = 0; i < cur.m_NumCharacters; ++i) {
        SPlayerInput input = model_get_input_at_tick(&ui->timeline, i, cur.m_GameTick);
        cc_on_input(&cur.m_pCharacters[i], &input);
      }

      wc_tick(&cur);

      // Apply character states again after ticking (to ensure the snapshot contains the exact truth)
      for (int i = 0; i < cur.m_NumCharacters; ++i) {
        SCharacterCore char_state;
        if (model_get_character_at_tick(&ui->timeline, i, cur.m_GameTick, &char_state)) {
          cur.m_pCharacters[i] = char_state;
          cur.m_pCharacters[i].m_pWorld = &cur;
          cur.m_pCharacters[i].m_pCollision = cur.m_pCollision;
          cur.m_pCharacters[i].m_pTuning = &cur.m_pTunings[0];
          cc_calc_indices(&cur.m_pCharacters[i]);
        }
      }
    }

    snap_world(sb, &ui->timeline, &prev, &cur);
    wc_copy_world(&prev, &cur);
    ui->demo_exporter.num_hammerhits = 0;

    int snap_size = demo_sb_finish(sb, snap_buf);
    if (snap_size > 0) demo_w_write_snap(writer, t, snap_buf, snap_size);

    // Write Net Events
    for (int i = 0; i < ui->timeline.net_event_count; ++i) {
      net_event_t *ev = &ui->timeline.net_events[i];
      if (ev->tick == t) {
        if (ev->type == NET_EVENT_CHAT) {
          demo_w_write_msg_sv_chat(writer, ev->team, ev->client_id, ev->message);
          log_info("what", "Id: %d, team: %d, msg: %s", ev->client_id, ev->team, ev->message);
        } else if (ev->type == NET_EVENT_BROADCAST) {
          demo_w_write_msg_sv_broadcast(writer, ev->message);
        } else if (ev->type == NET_EVENT_KILLMSG) {
          demo_w_write_msg_sv_killmsg(writer, ev->killer, ev->victim, ev->weapon, ev->mode_special);
        } else if (ev->type == NET_EVENT_SOUND_GLOBAL) {
          demo_w_write_msg_sv_sound_global(writer, ev->sound_id);
        } else if (ev->type == NET_EVENT_EMOTICON) {
          demo_w_write_msg_sv_emoticon(writer, ev->client_id, ev->emoticon);
        } else if (ev->type == NET_EVENT_VOTE_SET) {
          demo_w_write_msg_sv_vote_set(writer, ev->vote_timeout, ev->message, ev->reason);
        } else if (ev->type == NET_EVENT_VOTE_STATUS) {
          demo_w_write_msg_sv_vote_status(writer, ev->vote_yes, ev->vote_no, ev->vote_pass, ev->vote_total);
        } else if (ev->type == NET_EVENT_DDRACE_TIME) {
          demo_w_write_msg_sv_ddrace_time_legacy(writer, ev->time, ev->check, ev->finish);
        } else if (ev->type == NET_EVENT_RECORD) {
          demo_w_write_msg_sv_record_legacy(writer, ev->server_time_best, ev->player_time_best);
        }
      }
    }
  }
  demo_w_finish(writer);
  demo_w_destroy(&writer);
  demo_sb_destroy(&sb);
  wc_free(&cur);
  return 0;
}

void render_demo_window(ui_handler_t *ui) {
  demo_exporter_t *dx = &ui->demo_exporter;
  float dpi_scale = gfx_get_ui_scale();

  // Center the popup on first appearance
  ImGuiViewport *viewport = igGetMainViewport();
  ImVec2 center;
  ImGuiViewport_GetCenter(&center, viewport);
  igSetNextWindowPos(center, ImGuiCond_Appearing, (ImVec2){0.5f, 0.5f});

  if (igBeginPopupModal("Demo Export", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    // Path
    igText("Export Path");
    igInputText("##Path", dx->export_path, sizeof(dx->export_path), 0, NULL, NULL);
    igSameLine(0, 5.0f * dpi_scale);
    if (igButton("Browse...", (ImVec2){0, 0})) {
      nfdu8char_t *save_path;
      nfdu8filteritem_t filters[] = {{"DDNet Demo", "demo"}};
      nfdresult_t result = NFD_SaveDialogU8(&save_path, filters, 1, NULL, "unnamed.demo");
      if (result == NFD_OKAY) {
        strncpy(dx->export_path, save_path, sizeof(dx->export_path) - 1);
        dx->export_path[sizeof(dx->export_path) - 1] = '\0'; // Ensure null termination
        NFD_FreePathU8(save_path);
      }
    }

    // Map Name
    igText("Map Name (in demo)");
    igInputText("##MapName", dx->map_name, sizeof(dx->map_name), 0, NULL, NULL);

    // Ticks
    igText("Number of Ticks");
    igInputInt("##Ticks", &dx->num_ticks, 1, 100, 0);
    igSameLine(0, 5.0f * dpi_scale);
    if (igButton("Max Ticks", (ImVec2){0, 0})) {
      dx->num_ticks = model_get_max_timeline_tick(&ui->timeline);
    }

    igSeparator();
    igSpacing();

    if (igButton("Export", (ImVec2){120 * dpi_scale, 0})) {
      if (strlen(dx->export_path) > 0) {
        const char *map_name_to_use = (strlen(dx->map_name) > 0) ? dx->map_name : "unnamed_map";
        int result = export_to_demo(ui, dx->export_path, map_name_to_use, dx->num_ticks);
        if (result == 0) {
          log_info(LOG_SOURCE, "Demo exported successfully to '%s'", dx->export_path);
        } else {
          log_error(LOG_SOURCE, "Failed to export demo to '%s'", dx->export_path);
        }
      } else {
        log_warn(LOG_SOURCE, "Export path is empty. Cannot export demo.");
      }
      igCloseCurrentPopup();
    }

    igSetItemDefaultFocus();
    igSameLine(0, 10.0f * dpi_scale);

    if (igButton("Cancel", (ImVec2){120 * dpi_scale, 0})) {
      igCloseCurrentPopup();
    }

    igEndPopup();
  }
}
