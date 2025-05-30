/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <limits>

#include <engine/editor.h>
#include <engine/engine.h>
#include <engine/friends.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/demo.h>
#include <engine/map.h>
#include <engine/storage.h>
#include <engine/sound.h>
#include <engine/serverbrowser.h>
#include <engine/updater.h>
#include <engine/shared/demo.h>
#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <base/math.h>
#include <base/vmath.h>

#include <game/extrainfo.h>
#include <game/localization.h>
#include <game/version.h>
#include "race.h"
#include "render.h"

#include "gameclient.h"

#include "components/background.h"
#include "components/binds.h"
#include "components/broadcast.h"
#include "components/camera.h"
#include "components/chat.h"
#include "components/console.h"
#include "components/controls.h"
#include "components/countryflags.h"
#include "components/damageind.h"
#include "components/debughud.h"
#include "components/effects.h"
#include "components/emoticon.h"
#include "components/flow.h"
#include "components/hud.h"
#include "components/items.h"
#include "components/killmessages.h"
#include "components/mapimages.h"
#include "components/maplayers.h"
#include "components/mapsounds.h"
#include "components/menus.h"
#include "components/motd.h"
#include "components/particles.h"
#include "components/players.h"
#include "components/nameplates.h"
#include "components/scoreboard.h"
#include "components/skins.h"
#include "components/sounds.h"
#include "components/spectator.h"
#include "components/statboard.h"
#include "components/voting.h"

#include <base/system.h>
#include "components/race_demo.h"
#include "components/ghost.h"

CGameClient g_GameClient;

// instantiate all systems
static CKillMessages gs_KillMessages;
static CCamera gs_Camera;
static CChat gs_Chat;
static CMotd gs_Motd;
static CBroadcast gs_Broadcast;
static CGameConsole gs_GameConsole;
static CBinds gs_Binds;
static CParticles gs_Particles;
static CMenus gs_Menus;
static CSkins gs_Skins;
static CCountryFlags gs_CountryFlags;
static CFlow gs_Flow;
static CHud gs_Hud;
static CDebugHud gs_DebugHud;
static CControls gs_Controls;
static CEffects gs_Effects;
static CScoreboard gs_Scoreboard;
static CStatboard gs_Statboard;
static CSounds gs_Sounds;
static CEmoticon gs_Emoticon;
static CDamageInd gsDamageInd;
static CVoting gs_Voting;
static CSpectator gs_Spectator;

static CPlayers gs_Players;
static CNamePlates gs_NamePlates;
static CItems gs_Items;
static CMapImages gs_MapImages;

static CMapLayers gs_MapLayersBackGround(CMapLayers::TYPE_BACKGROUND);
static CMapLayers gs_MapLayersForeGround(CMapLayers::TYPE_FOREGROUND);
static CBackground gs_BackGround;

static CMapSounds gs_MapSounds;

static CRaceDemo gs_RaceDemo;
static CGhost gs_Ghost;

CGameClient::CStack::CStack() { m_Num = 0; }
void CGameClient::CStack::Add(class CComponent *pComponent) { m_paComponents[m_Num++] = pComponent; }

const char *CGameClient::Version() { return GAME_VERSION; }
const char *CGameClient::NetVersion() { return GAME_NETVERSION; }
const char *CGameClient::GetItemName(int Type) { return m_NetObjHandler.GetObjName(Type); }

void CGameClient::OnConsoleInit()
{
	m_pEngine = Kernel()->RequestInterface<IEngine>();
	m_pClient = Kernel()->RequestInterface<IClient>();
	m_pTextRender = Kernel()->RequestInterface<ITextRender>();
	m_pSound = Kernel()->RequestInterface<ISound>();
	m_pInput = Kernel()->RequestInterface<IInput>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_pDemoPlayer = Kernel()->RequestInterface<IDemoPlayer>();
	m_pServerBrowser = Kernel()->RequestInterface<IServerBrowser>();
	m_pEditor = Kernel()->RequestInterface<IEditor>();
	m_pFriends = Kernel()->RequestInterface<IFriends>();
	m_pFoes = Client()->Foes();
#if defined(CONF_FAMILY_WINDOWS) || defined(CONF_PLATFORM_LINUX)
	m_pUpdater = Kernel()->RequestInterface<IUpdater>();
#endif

	// setup pointers
	m_pBinds = &::gs_Binds;
	m_pGameConsole = &::gs_GameConsole;
	m_pParticles = &::gs_Particles;
	m_pMenus = &::gs_Menus;
	m_pSkins = &::gs_Skins;
	m_pCountryFlags = &::gs_CountryFlags;
	m_pChat = &::gs_Chat;
	m_pFlow = &::gs_Flow;
	m_pCamera = &::gs_Camera;
	m_pControls = &::gs_Controls;
	m_pEffects = &::gs_Effects;
	m_pSounds = &::gs_Sounds;
	m_pMotd = &::gs_Motd;
	m_pDamageind = &::gsDamageInd;
	m_pMapimages = &::gs_MapImages;
	m_pVoting = &::gs_Voting;
	m_pScoreboard = &::gs_Scoreboard;
	m_pStatboard = &::gs_Statboard;
	m_pItems = &::gs_Items;
	m_pMapLayersBackGround = &::gs_MapLayersBackGround;
	m_pMapLayersForeGround = &::gs_MapLayersForeGround;
	m_pBackGround = &::gs_BackGround;

	m_pMapSounds = &::gs_MapSounds;
	m_pPlayers = &::gs_Players;

	m_pRaceDemo = &::gs_RaceDemo;
	m_pGhost = &::gs_Ghost;

	gs_NamePlates.SetPlayers(m_pPlayers);

	// make a list of all the systems, make sure to add them in the correct render order
	m_All.Add(m_pSkins);
	m_All.Add(m_pCountryFlags);
	m_All.Add(m_pMapimages);
	m_All.Add(m_pEffects); // doesn't render anything, just updates effects
	m_All.Add(m_pParticles);
	m_All.Add(m_pBinds);
	m_All.Add(&m_pBinds->m_SpecialBinds);
	m_All.Add(m_pControls);
	m_All.Add(m_pCamera);
	m_All.Add(m_pSounds);
	m_All.Add(m_pVoting);
	m_All.Add(m_pParticles); // doesn't render anything, just updates all the particles
	m_All.Add(m_pRaceDemo);
	m_All.Add(m_pMapSounds);

	m_All.Add(&gs_BackGround);	//render instead of gs_MapLayersBackGround when g_Config.m_ClOverlayEntities == 100
	m_All.Add(&gs_MapLayersBackGround); // first to render
	m_All.Add(&m_pParticles->m_RenderTrail);
	m_All.Add(m_pItems);
	m_All.Add(m_pPlayers);
	m_All.Add(m_pGhost);
	m_All.Add(&gs_MapLayersForeGround);
	m_All.Add(&m_pParticles->m_RenderExplosions);
	m_All.Add(&gs_NamePlates);
	m_All.Add(&m_pParticles->m_RenderGeneral);
	m_All.Add(m_pDamageind);
	m_All.Add(&gs_Hud);
	m_All.Add(&gs_Spectator);
	m_All.Add(&gs_Emoticon);
	m_All.Add(&gs_KillMessages);
	m_All.Add(m_pChat);
	m_All.Add(&gs_Broadcast);
	m_All.Add(&gs_DebugHud);
	m_All.Add(&gs_Scoreboard);
	m_All.Add(&gs_Statboard);
	m_All.Add(m_pMotd);
	m_All.Add(m_pMenus);
	m_All.Add(&m_pMenus->m_Binder);
	m_All.Add(m_pGameConsole);

	// build the input stack
	m_Input.Add(&m_pMenus->m_Binder); // this will take over all input when we want to bind a key
	m_Input.Add(&m_pBinds->m_SpecialBinds);
	m_Input.Add(m_pGameConsole);
	m_Input.Add(m_pChat); // chat has higher prio due to tha you can quit it by pressing esc
	m_Input.Add(m_pMotd); // for pressing esc to remove it
	m_Input.Add(m_pMenus);
	m_Input.Add(&gs_Spectator);
	m_Input.Add(&gs_Emoticon);
	m_Input.Add(m_pControls);
	m_Input.Add(m_pBinds);

	// add the some console commands
	Console()->Register("team", "i[team-id]", CFGFLAG_CLIENT, ConTeam, this, "Switch team");
	Console()->Register("kill", "", CFGFLAG_CLIENT, ConKill, this, "Kill yourself");

	// register server dummy commands for tab completion
	Console()->Register("tune", "s[tuning] i[value]", CFGFLAG_SERVER, 0, 0, "Tune variable to value");
	Console()->Register("tune_reset", "", CFGFLAG_SERVER, 0, 0, "Reset tuning");
	Console()->Register("tune_dump", "", CFGFLAG_SERVER, 0, 0, "Dump tuning");
	Console()->Register("change_map", "?r[map]", CFGFLAG_SERVER, 0, 0, "Change map");
	Console()->Register("restart", "?i[seconds]", CFGFLAG_SERVER, 0, 0, "Restart in x seconds");
	Console()->Register("broadcast", "r[message]", CFGFLAG_SERVER, 0, 0, "Broadcast message");
	Console()->Register("say", "r[message]", CFGFLAG_SERVER, 0, 0, "Say in chat");
	Console()->Register("set_team", "i[id] i[team-id] ?i[delay in minutes]", CFGFLAG_SERVER, 0, 0, "Set team of player to team");
	Console()->Register("set_team_all", "i[team-id]", CFGFLAG_SERVER, 0, 0, "Set team of all players to team");
	Console()->Register("add_vote", "s[name] r[command]", CFGFLAG_SERVER, 0, 0, "Add a voting option");
	Console()->Register("remove_vote", "s[name]", CFGFLAG_SERVER, 0, 0, "remove a voting option");
	Console()->Register("force_vote", "s[name] s[command] ?r[reason]", CFGFLAG_SERVER, 0, 0, "Force a voting option");
	Console()->Register("clear_votes", "", CFGFLAG_SERVER, 0, 0, "Clears the voting options");
	Console()->Register("vote", "r['yes'|'no']", CFGFLAG_SERVER, 0, 0, "Force a vote to yes/no");
	Console()->Register("swap_teams", "", CFGFLAG_SERVER, 0, 0, "Swap the current teams");
	Console()->Register("shuffle_teams", "", CFGFLAG_SERVER, 0, 0, "Shuffle the current teams");


	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->m_pClient = this;

	// let all the other components register their console commands
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnConsoleInit();


	//
	Console()->Chain("player_name", ConchainSpecialInfoupdate, this);
	Console()->Chain("player_clan", ConchainSpecialInfoupdate, this);
	Console()->Chain("player_country", ConchainSpecialInfoupdate, this);
	Console()->Chain("player_use_custom_color", ConchainSpecialInfoupdate, this);
	Console()->Chain("player_color_body", ConchainSpecialInfoupdate, this);
	Console()->Chain("player_color_feet", ConchainSpecialInfoupdate, this);
	Console()->Chain("player_skin", ConchainSpecialInfoupdate, this);

	Console()->Chain("dummy_name", ConchainSpecialDummyInfoupdate, this);
	Console()->Chain("dummy_clan", ConchainSpecialDummyInfoupdate, this);
	Console()->Chain("dummy_country", ConchainSpecialDummyInfoupdate, this);
	Console()->Chain("dummy_use_custom_color", ConchainSpecialDummyInfoupdate, this);
	Console()->Chain("dummy_color_body", ConchainSpecialDummyInfoupdate, this);
	Console()->Chain("dummy_color_feet", ConchainSpecialDummyInfoupdate, this);
	Console()->Chain("dummy_skin", ConchainSpecialDummyInfoupdate, this);

	Console()->Chain("cl_dummy", ConchainSpecialDummy, this);
	Console()->Chain("cl_text_entities_size", ConchainClTextEntitiesSize, this);

	//
	m_SuppressEvents = false;
}

void CGameClient::OnInit()
{
	m_pGraphics = Kernel()->RequestInterface<IGraphics>();

	m_pGraphics->AddWindowResizeListener(OnWindowResizeCB, this);

	// propagate pointers
	m_UI.SetGraphics(Graphics(), TextRender());
	m_RenderTools.m_pGraphics = Graphics();
	m_RenderTools.m_pUI = UI();

	int64 Start = time_get();

	// set the language
	g_Localization.Load(g_Config.m_ClLanguagefile, Storage(), Console());

	// TODO: this should be different
	// setup item sizes
	for(int i = 0; i < NUM_NETOBJTYPES; i++)
		Client()->SnapSetStaticsize(i, m_NetObjHandler.GetObjSize(i));

	Client()->LoadFont();

	// init all components
	for(int i = m_All.m_Num-1; i >= 0; --i)
		m_All.m_paComponents[i]->OnInit();

	char aBuf[256];

	// setup load amount// load textures
	for(int i = 0; i < g_pData->m_NumImages; i++)
	{
		g_pData->m_aImages[i].m_Id = Graphics()->LoadTexture(g_pData->m_aImages[i].m_pFilename, IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
		g_GameClient.m_pMenus->RenderLoading();
	}

	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnReset();

	m_ServerMode = SERVERMODE_PURE;

	m_DDRaceMsgSent[0] = false;
	m_DDRaceMsgSent[1] = false;
	m_ShowOthers[0] = -1;
	m_ShowOthers[1] = -1;

	// Set free binds to DDRace binds if it's active
	if(!g_Config.m_ClDDRaceBindsSet && g_Config.m_ClDDRaceBinds)
		gs_Binds.SetDDRaceBinds(true);

	if(g_Config.m_ClTimeoutCode[0] == '\0' || str_comp(g_Config.m_ClTimeoutCode, "hGuEYnfxicsXGwFq") == 0)
	{
		for(unsigned int i = 0; i < 16; i++)
		{
			if(rand() % 2)
				g_Config.m_ClTimeoutCode[i] =(char)((rand() % 26) + 97);
			else
				g_Config.m_ClTimeoutCode[i] = (char)((rand() % 26) + 65);
		}
	}

	if(g_Config.m_ClDummyTimeoutCode[0] == '\0' || str_comp(g_Config.m_ClDummyTimeoutCode, "hGuEYnfxicsXGwFq") == 0)
	{
		for(unsigned int i = 0; i < 16; i++)
		{
			if(rand() % 2)
				g_Config.m_ClDummyTimeoutCode[i] = (char)((rand() % 26) + 97);
			else
				g_Config.m_ClDummyTimeoutCode[i] = (char)((rand() % 26) + 65);
		}
	}

	int64 End = time_get();
	str_format(aBuf, sizeof(aBuf), "initialisation finished after %.2fms", ((End-Start)*1000)/(float)time_freq());
	Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "gameclient", aBuf);

	m_GameWorld.m_GameTickSpeed = SERVER_TICK_SPEED;
	m_GameWorld.m_pCollision = Collision();

	m_pMapimages->SetTextureScale(g_Config.m_ClTextEntitiesSize);
}

void CGameClient::OnUpdate()
{
	// handle mouse movement
	float x = 0.0f, y = 0.0f;
	Input()->MouseRelative(&x, &y);
	if(x != 0.0f || y != 0.0f)
	{
		for(int h = 0; h < m_Input.m_Num; h++)
		{
			if(m_Input.m_paComponents[h]->OnMouseMove(x, y))
				break;
		}
	}

	// handle key presses
	for(int i = 0; i < Input()->NumEvents(); i++)
	{
		IInput::CEvent e = Input()->GetEvent(i);
		if(!Input()->IsEventValid(&e))
			continue;

		for(int h = 0; h < m_Input.m_Num; h++)
		{
			if(m_Input.m_paComponents[h]->OnInput(e))
				break;
		}
	}
}

void CGameClient::OnDummySwap()
{
	if(g_Config.m_ClDummyResetOnSwitch)
	{
		m_pControls->ResetInput(!g_Config.m_ClDummy);
		m_pControls->m_InputData[!g_Config.m_ClDummy].m_Hook = 0;
	}
	int tmp = m_DummyInput.m_Fire;
	m_DummyInput = m_pControls->m_InputData[!g_Config.m_ClDummy];
	m_pControls->m_InputData[g_Config.m_ClDummy].m_Fire = tmp;
}

int CGameClient::OnSnapInput(int *pData, bool Dummy, bool Force)
{
	m_LocalIDs[g_Config.m_ClDummy] = m_Snap.m_LocalClientID;

	if(!Dummy)
	{
		return m_pControls->SnapInput(pData);
	}

	if(!g_Config.m_ClDummyHammer)
	{
		if(m_DummyFire != 0)
		{
			m_DummyInput.m_Fire = m_HammerInput.m_Fire;
			m_DummyFire = 0;
		}

		if(!Force && (!m_DummyInput.m_Direction && !m_DummyInput.m_Jump && !m_DummyInput.m_Hook))
		{
			return 0;
		}

		mem_copy(pData, &m_DummyInput, sizeof(m_DummyInput));
		return sizeof(m_DummyInput);
	}
	else
	{
		if((m_DummyFire / 12.5f) - (int)(m_DummyFire / 12.5f) > 0.01f)
		{
			m_DummyFire++;
			return 0;
		}
		m_DummyFire++;

		m_HammerInput.m_Fire += 2;
		m_HammerInput.m_WantedWeapon = WEAPON_HAMMER + 1;
		if(!g_Config.m_ClDummyRestoreWeapon)
		{
			m_DummyInput.m_WantedWeapon = WEAPON_HAMMER + 1;
		}

		vec2 Main = m_LocalCharacterPos;
		vec2 Dummy = m_aClients[m_LocalIDs[!g_Config.m_ClDummy]].m_Predicted.m_Pos;
		vec2 Dir = Main - Dummy;
		m_HammerInput.m_TargetX = (int)(Dir.x);
		m_HammerInput.m_TargetY = (int)(Dir.y);

		mem_copy(pData, &m_HammerInput, sizeof(m_HammerInput));
		return sizeof(m_HammerInput);
	}
}

void CGameClient::OnConnected()
{
	m_Layers.Init(Kernel());
	m_Collision.Init(Layers());

	RenderTools()->RenderTilemapGenerateSkip(Layers());

	CRaceHelper::ms_aFlagIndex[0] = -1;
	CRaceHelper::ms_aFlagIndex[1] = -1;

	CTile *pGameTiles = static_cast<CTile *>(Layers()->Map()->GetData(Layers()->GameLayer()->m_Data));

	// get flag positions
	for(int i = 0; i < m_Collision.GetWidth()*m_Collision.GetHeight(); i++)
	{
		if(pGameTiles[i].m_Index - ENTITY_OFFSET == ENTITY_FLAGSTAND_RED)
			CRaceHelper::ms_aFlagIndex[TEAM_RED] = i;
		else if(pGameTiles[i].m_Index - ENTITY_OFFSET == ENTITY_FLAGSTAND_BLUE)
			CRaceHelper::ms_aFlagIndex[TEAM_BLUE] = i;
		i += pGameTiles[i].m_Skip;
	}

	for(int i = 0; i < m_All.m_Num; i++)
	{
		m_All.m_paComponents[i]->OnMapLoad();
		m_All.m_paComponents[i]->OnReset();
	}

	m_ServerMode = SERVERMODE_PURE;

	// send the initial info
	SendInfo(true);
	// we should keep this in for now, because otherwise you can't spectate
	// people at start as the other info 64 packet is only sent after the first
	// snap
	Client()->Rcon("crashmeplx");

	m_GameWorld.Clear();
	m_GameWorld.m_WorldConfig.m_InfiniteAmmo = true;
	for(int i = 0; i < MAX_CLIENTS; i++)
		m_aLastWorldCharacters[i].m_Alive = false;
}

void CGameClient::OnReset()
{
	// clear out the invalid pointers
	m_LastNewPredictedTick[0] = -1;
	m_LastNewPredictedTick[1] = -1;
	mem_zero(&g_GameClient.m_Snap, sizeof(g_GameClient.m_Snap));

	for(int i = 0; i < MAX_CLIENTS; i++)
		m_aClients[i].Reset();

	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnReset();

	m_DemoSpecID = SPEC_FOLLOW;
	m_FlagDropTick[TEAM_RED] = 0;
	m_FlagDropTick[TEAM_BLUE] = 0;
	m_LastRoundStartTick = -1;
	m_LastFlagCarrierRed = -4;
	m_LastFlagCarrierBlue = -4;
	m_Tuning[g_Config.m_ClDummy] = CTuningParams();

	m_Teams.Reset();
	m_DDRaceMsgSent[0] = false;
	m_DDRaceMsgSent[1] = false;
	m_ShowOthers[0] = -1;
	m_ShowOthers[1] = -1;
}


void CGameClient::UpdatePositions()
{
	// local character position
	if(g_Config.m_ClPredict && Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		if(!AntiPingPlayers())
		{
			if(!m_Snap.m_pLocalCharacter || (m_Snap.m_pGameInfoObj && m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER))
			{
				// don't use predicted
			}
			else
				m_LocalCharacterPos = mix(m_PredictedPrevChar.m_Pos, m_PredictedChar.m_Pos, Client()->PredIntraGameTick());
		}
		else
		{
			if(!(m_Snap.m_pGameInfoObj && m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER))
			{
				if(m_Snap.m_pLocalCharacter)
					m_LocalCharacterPos = mix(m_PredictedPrevChar.m_Pos, m_PredictedChar.m_Pos, Client()->PredIntraGameTick());
			}
	//		else
	//			m_LocalCharacterPos = mix(m_PredictedPrevChar.m_Pos, m_PredictedChar.m_Pos, Client()->PredIntraGameTick());
		}
	}
	else if(m_Snap.m_pLocalCharacter && m_Snap.m_pLocalPrevCharacter)
	{
		m_LocalCharacterPos = mix(
			vec2(m_Snap.m_pLocalPrevCharacter->m_X, m_Snap.m_pLocalPrevCharacter->m_Y),
			vec2(m_Snap.m_pLocalCharacter->m_X, m_Snap.m_pLocalCharacter->m_Y), Client()->IntraGameTick());
	}

	// spectator position
	if(m_Snap.m_SpecInfo.m_Active)
	{
		if(Client()->State() == IClient::STATE_DEMOPLAYBACK && m_DemoSpecID != SPEC_FOLLOW && m_Snap.m_SpecInfo.m_SpectatorID != SPEC_FREEVIEW)
		{
			m_Snap.m_SpecInfo.m_Position = mix(
				vec2(m_Snap.m_aCharacters[m_Snap.m_SpecInfo.m_SpectatorID].m_Prev.m_X, m_Snap.m_aCharacters[m_Snap.m_SpecInfo.m_SpectatorID].m_Prev.m_Y),
				vec2(m_Snap.m_aCharacters[m_Snap.m_SpecInfo.m_SpectatorID].m_Cur.m_X, m_Snap.m_aCharacters[m_Snap.m_SpecInfo.m_SpectatorID].m_Cur.m_Y),
				Client()->IntraGameTick());
			m_Snap.m_SpecInfo.m_UsePosition = true;
		}
		else if(m_Snap.m_pSpectatorInfo && ((Client()->State() == IClient::STATE_DEMOPLAYBACK && m_DemoSpecID == SPEC_FOLLOW) || (Client()->State() != IClient::STATE_DEMOPLAYBACK && m_Snap.m_SpecInfo.m_SpectatorID != SPEC_FREEVIEW)))
		{
			if(m_Snap.m_pPrevSpectatorInfo)
				m_Snap.m_SpecInfo.m_Position = mix(vec2(m_Snap.m_pPrevSpectatorInfo->m_X, m_Snap.m_pPrevSpectatorInfo->m_Y),
													vec2(m_Snap.m_pSpectatorInfo->m_X, m_Snap.m_pSpectatorInfo->m_Y), Client()->IntraGameTick());
			else
				m_Snap.m_SpecInfo.m_Position = vec2(m_Snap.m_pSpectatorInfo->m_X, m_Snap.m_pSpectatorInfo->m_Y);
			m_Snap.m_SpecInfo.m_UsePosition = true;
		}
	}

	UpdateRenderedCharacters();
}


static void Evolve(CNetObj_Character *pCharacter, int Tick)
{
	CWorldCore TempWorld;
	CCharacterCore TempCore;
	CTeamsCore TempTeams;
	mem_zero(&TempCore, sizeof(TempCore));
	mem_zero(&TempTeams, sizeof(TempTeams));
	TempCore.Init(&TempWorld, g_GameClient.Collision(), &TempTeams);
	TempCore.Read(pCharacter);
	TempCore.m_ActiveWeapon = pCharacter->m_Weapon;

	while(pCharacter->m_Tick < Tick)
	{
		pCharacter->m_Tick++;
		TempCore.Tick(false);
		TempCore.Move();
		TempCore.Quantize();
	}

	TempCore.Write(pCharacter);
}

void CGameClient::OnRender()
{
	// update the local character and spectate position
	UpdatePositions();

	// render all systems
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnRender();

	// clear all events/input for this frame
	Input()->Clear();

	// clear new tick flags
	m_NewTick = false;
	m_NewPredictedTick = false;

	if(g_Config.m_ClDummy && !Client()->DummyConnected())
		g_Config.m_ClDummy = 0;

	// resend player and dummy info if it was filtered by server
	if(Client()->State() == IClient::STATE_ONLINE && !m_pMenus->IsActive()) {
		if(m_CheckInfo[0] == 0) {
			if(
			str_comp(m_aClients[m_LocalIDs[0]].m_aName, g_Config.m_PlayerName) ||
			str_comp(m_aClients[m_LocalIDs[0]].m_aClan, g_Config.m_PlayerClan) ||
			m_aClients[m_LocalIDs[0]].m_Country != g_Config.m_PlayerCountry ||
			str_comp(m_aClients[m_LocalIDs[0]].m_aSkinName, g_Config.m_ClPlayerSkin) ||
			m_aClients[m_LocalIDs[0]].m_UseCustomColor != g_Config.m_ClPlayerUseCustomColor ||
			m_aClients[m_LocalIDs[0]].m_ColorBody != (int)g_Config.m_ClPlayerColorBody ||
			m_aClients[m_LocalIDs[0]].m_ColorFeet != (int)g_Config.m_ClPlayerColorFeet
			)
				SendInfo(false);
			else
				m_CheckInfo[0] = -1;
		}

		if(m_CheckInfo[0] > 0)
			m_CheckInfo[0]--;

		if(Client()->DummyConnected()) {
			if(m_CheckInfo[1] == 0) {
				if(
				str_comp(m_aClients[m_LocalIDs[1]].m_aName, g_Config.m_ClDummyName) ||
				str_comp(m_aClients[m_LocalIDs[1]].m_aClan, g_Config.m_ClDummyClan) ||
				m_aClients[m_LocalIDs[1]].m_Country != g_Config.m_ClDummyCountry ||
				str_comp(m_aClients[m_LocalIDs[1]].m_aSkinName, g_Config.m_ClDummySkin) ||
				m_aClients[m_LocalIDs[1]].m_UseCustomColor != g_Config.m_ClDummyUseCustomColor ||
				m_aClients[m_LocalIDs[1]].m_ColorBody != (int)g_Config.m_ClDummyColorBody ||
				m_aClients[m_LocalIDs[1]].m_ColorFeet != (int)g_Config.m_ClDummyColorFeet
				)
					SendDummyInfo(false);
				else
					m_CheckInfo[1] = -1;
			}

			if(m_CheckInfo[1] > 0)
				m_CheckInfo[1]--;
		}
	}
}

void CGameClient::OnDummyDisconnect()
{
	m_DDRaceMsgSent[1] = false;
	m_ShowOthers[1] = -1;
	m_LastNewPredictedTick[1] = -1;
}

int CGameClient::GetLastRaceTick()
{
	return m_pGhost->GetLastRaceTick();
}

void CGameClient::OnRelease()
{
	// release all systems
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnRelease();
}

void CGameClient::OnMessage(int MsgId, CUnpacker *pUnpacker, bool IsDummy)
{
	// special messages
	if(MsgId == NETMSGTYPE_SV_EXTRAPROJECTILE && !IsDummy)
	{
		int Num = pUnpacker->GetInt();

		for(int k = 0; k < Num; k++)
		{
			CNetObj_Projectile Proj;
			for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
				((int *)&Proj)[i] = pUnpacker->GetInt();

			if(pUnpacker->Error())
				return;

			g_GameClient.m_pItems->AddExtraProjectile(&Proj);
		}

		return;
	}
	else if(MsgId == NETMSGTYPE_SV_TUNEPARAMS)
	{
		// unpack the new tuning
		CTuningParams NewTuning;
		int *pParams = (int *)&NewTuning;
		// No jetpack on DDNet incompatible servers:
		NewTuning.m_JetpackStrength = 0;
		for(unsigned i = 0; i < sizeof(CTuningParams)/sizeof(int); i++)
		{
			int value = pUnpacker->GetInt();

			// check for unpacking errors
			if(pUnpacker->Error())
				break;

			pParams[i] = value;
		}

		m_ServerMode = SERVERMODE_PURE;

		// apply new tuning
		m_Tuning[IsDummy ? !g_Config.m_ClDummy : g_Config.m_ClDummy] = NewTuning;
		return;
	}

	void *pRawMsg = m_NetObjHandler.SecureUnpackMsg(MsgId, pUnpacker);
	if(!pRawMsg)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "dropped weird message '%s' (%d), failed on '%s'", m_NetObjHandler.GetMsgName(MsgId), MsgId, m_NetObjHandler.FailedMsgOn());
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);
		return;
	}

	if(IsDummy)
	{
		if(MsgId == NETMSGTYPE_SV_CHAT
			&& m_LocalIDs[0] >= 0
			&& m_LocalIDs[1] >= 0)
		{
			CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;

			if((pMsg->m_Team == 1
					&& (m_aClients[m_LocalIDs[0]].m_Team != m_aClients[m_LocalIDs[1]].m_Team
						|| m_Teams.Team(m_LocalIDs[0]) != m_Teams.Team(m_LocalIDs[1])))
				|| pMsg->m_Team > 1)
			{
				m_pChat->OnMessage(MsgId, pRawMsg);
			}
		}
		return; // no need of all that stuff for the dummy
	}

	// TODO: this should be done smarter
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnMessage(MsgId, pRawMsg);

	if(MsgId == NETMSGTYPE_SV_READYTOENTER)
	{
		Client()->EnterGame();
	}
	else if(MsgId == NETMSGTYPE_SV_EMOTICON)
	{
		CNetMsg_Sv_Emoticon *pMsg = (CNetMsg_Sv_Emoticon *)pRawMsg;

		// apply
		m_aClients[pMsg->m_ClientID].m_Emoticon = pMsg->m_Emoticon;
		m_aClients[pMsg->m_ClientID].m_EmoticonStart = Client()->GameTick();
	}
	else if(MsgId == NETMSGTYPE_SV_SOUNDGLOBAL)
	{
		if(m_SuppressEvents)
			return;

		// don't enqueue pseudo-global sounds from demos (created by PlayAndRecord)
		CNetMsg_Sv_SoundGlobal *pMsg = (CNetMsg_Sv_SoundGlobal *)pRawMsg;
		if(pMsg->m_SoundID == SOUND_CTF_DROP || pMsg->m_SoundID == SOUND_CTF_RETURN ||
			pMsg->m_SoundID == SOUND_CTF_CAPTURE || pMsg->m_SoundID == SOUND_CTF_GRAB_EN ||
			pMsg->m_SoundID == SOUND_CTF_GRAB_PL)
		{
			if(g_Config.m_SndGame)
				g_GameClient.m_pSounds->Enqueue(CSounds::CHN_GLOBAL, pMsg->m_SoundID);
		}
		else
		{
			if(g_Config.m_SndGame)
				g_GameClient.m_pSounds->Play(CSounds::CHN_GLOBAL, pMsg->m_SoundID, 1.0f);
		}
	}
	else if(MsgId == NETMSGTYPE_SV_TEAMSSTATE)
	{
		unsigned int i;

		for(i = 0; i < MAX_CLIENTS; i++)
		{
			int Team = pUnpacker->GetInt();
			bool WentWrong = false;

			if(pUnpacker->Error())
				WentWrong = true;

			if(!WentWrong && Team >= 0 && Team < MAX_CLIENTS)
				m_Teams.Team(i, Team);
			else if(Team != MAX_CLIENTS)
				WentWrong = true;

			if(WentWrong)
			{
				m_Teams.Team(i, 0);
				break;
			}
		}

		if(i <= 16)
			m_Teams.m_IsDDRace16 = true;

		m_pGhost->m_AllowRestart = true;
		m_pRaceDemo->m_AllowRestart = true;
	}
	else if(MsgId == NETMSGTYPE_SV_PLAYERTIME)
	{
		CNetMsg_Sv_PlayerTime *pMsg = (CNetMsg_Sv_PlayerTime *)pRawMsg;
		m_aClients[pMsg->m_ClientID].m_Score = pMsg->m_Time;
	}
	else if(MsgId == NETMSGTYPE_SV_KILLMSG)
	{
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;
		// reset character prediction
		if(!(m_GameWorld.m_WorldConfig.m_IsFNG && pMsg->m_Weapon == WEAPON_RIFLE))
		{
			m_CharOrder.GiveWeak(pMsg->m_Victim);
			m_aLastWorldCharacters[pMsg->m_Victim].m_Alive = false;
			if(CCharacter *pChar = m_GameWorld.GetCharacterByID(pMsg->m_Victim))
				pChar->ResetPrediction();
			m_GameWorld.ReleaseHooked(pMsg->m_Victim);
		}
	}
}

void CGameClient::OnStateChange(int NewState, int OldState)
{
	// reset everything when not already connected (to keep gathered stuff)
	if(NewState < IClient::STATE_ONLINE)
		OnReset();

	// then change the state
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnStateChange(NewState, OldState);
}

void CGameClient::OnShutdown()
{
	m_pRaceDemo->OnReset();
	m_pGhost->OnReset();
}

void CGameClient::OnEnterGame()
{
	g_GameClient.m_pEffects->ResetDamageIndicator();
}

void CGameClient::OnGameOver()
{
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK && g_Config.m_ClEditor == 0)
		Client()->AutoScreenshot_Start();
}

void CGameClient::OnStartGame()
{
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
		Client()->DemoRecorder_HandleAutoStart();
	m_pStatboard->OnReset();
}

void CGameClient::OnFlagGrab(int TeamID)
{
	if(TeamID == TEAM_RED)
		m_aStats[m_Snap.m_pGameDataObj->m_FlagCarrierRed].m_FlagGrabs++;
	else
		m_aStats[m_Snap.m_pGameDataObj->m_FlagCarrierBlue].m_FlagGrabs++;
}

void CGameClient::OnWindowResize()
{
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnWindowResize();

	TextRender()->OnWindowResize();
}

void CGameClient::OnWindowResizeCB(void *pUser)
{
	CGameClient *pClient = (CGameClient*)pUser;
	pClient->OnWindowResize();
}

void CGameClient::OnRconType(bool UsernameReq)
{
	m_pGameConsole->RequireUsername(UsernameReq);
}

void CGameClient::OnRconLine(const char *pLine)
{
	m_pGameConsole->PrintLine(CGameConsole::CONSOLETYPE_REMOTE, pLine);
}

void CGameClient::ProcessEvents()
{
	if(m_SuppressEvents)
		return;

	int SnapType = IClient::SNAP_CURRENT;
	int Num = Client()->SnapNumItems(SnapType);
	for(int Index = 0; Index < Num; Index++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(SnapType, Index, &Item);

		if(Item.m_Type == NETEVENTTYPE_DAMAGEIND)
		{
			CNetEvent_DamageInd *ev = (CNetEvent_DamageInd *)pData;
			g_GameClient.m_pEffects->DamageIndicator(vec2(ev->m_X, ev->m_Y), GetDirection(ev->m_Angle));
		}
		else if(Item.m_Type == NETEVENTTYPE_EXPLOSION)
		{
			CNetEvent_Explosion *ev = (CNetEvent_Explosion *)pData;
			g_GameClient.m_pEffects->Explosion(vec2(ev->m_X, ev->m_Y));
		}
		else if(Item.m_Type == NETEVENTTYPE_HAMMERHIT)
		{
			CNetEvent_HammerHit *ev = (CNetEvent_HammerHit *)pData;
			g_GameClient.m_pEffects->HammerHit(vec2(ev->m_X, ev->m_Y));
		}
		else if(Item.m_Type == NETEVENTTYPE_SPAWN)
		{
			CNetEvent_Spawn *ev = (CNetEvent_Spawn *)pData;
			g_GameClient.m_pEffects->PlayerSpawn(vec2(ev->m_X, ev->m_Y));
		}
		else if(Item.m_Type == NETEVENTTYPE_DEATH)
		{
			CNetEvent_Death *ev = (CNetEvent_Death *)pData;
			g_GameClient.m_pEffects->PlayerDeath(vec2(ev->m_X, ev->m_Y), ev->m_ClientID);
		}
		else if(Item.m_Type == NETEVENTTYPE_SOUNDWORLD)
		{
			CNetEvent_SoundWorld *ev = (CNetEvent_SoundWorld *)pData;
			if(g_Config.m_SndGame && (ev->m_SoundID != SOUND_GUN_FIRE || g_Config.m_SndGun) && (ev->m_SoundID != SOUND_PLAYER_PAIN_LONG || g_Config.m_SndLongPain))
				g_GameClient.m_pSounds->PlayAt(CSounds::CHN_WORLD, ev->m_SoundID, 1.0f, vec2(ev->m_X, ev->m_Y));
		}
	}
}

static CGameInfo GetGameInfo(const CNetObj_GameInfoEx *pInfoEx, int InfoExSize, CServerInfo *pFallbackServerInfo)
{
	int Version = -1;
	if(InfoExSize >= 8)
	{
		Version = pInfoEx->m_Version;
	}
	else if(InfoExSize >= 4)
	{
		Version = 0;
	}
	int Flags = 0;
	if(Version >= 0)
	{
		Flags = pInfoEx->m_Flags;
	}
	bool Race;
	bool FastCap;
	bool FNG;
	bool DDRace;
	bool DDNet;
	bool BlockWorlds;
	bool Vanilla;
	bool Plus;
	if(Version < 1)
	{
		Race = IsRace(pFallbackServerInfo);
		FastCap = IsFastCap(pFallbackServerInfo);
		FNG = IsFNG(pFallbackServerInfo);
		DDRace = IsDDRace(pFallbackServerInfo);
		DDNet = IsDDNet(pFallbackServerInfo);
		BlockWorlds = IsBlockWorlds(pFallbackServerInfo);
		Vanilla = IsVanilla(pFallbackServerInfo);
		Plus = IsPlus(pFallbackServerInfo);
	}
	else
	{
		Race = Flags&GAMEINFOFLAG_GAMETYPE_RACE;
		FastCap = Flags&GAMEINFOFLAG_GAMETYPE_FASTCAP;
		FNG = Flags&GAMEINFOFLAG_GAMETYPE_FNG;
		DDRace = Flags&GAMEINFOFLAG_GAMETYPE_DDRACE;
		DDNet = Flags&GAMEINFOFLAG_GAMETYPE_DDNET;
		BlockWorlds = Flags&GAMEINFOFLAG_GAMETYPE_BLOCK_WORLDS;
		Vanilla = Flags&GAMEINFOFLAG_GAMETYPE_VANILLA;
		Plus = Flags&GAMEINFOFLAG_GAMETYPE_PLUS;

		// Ensure invariants upheld by the server info parsing business.
		DDRace = DDRace || DDNet;
		Race = Race || FastCap || DDRace;
	}

	CGameInfo Info;
	Info.m_FlagStartsRace = FastCap;
	Info.m_TimeScore = Race;
	Info.m_UnlimitedAmmo = Race;
	Info.m_DDRaceRecordMessage = DDRace && !DDNet;
	Info.m_RaceRecordMessage = DDNet || (Race && !DDRace);
	Info.m_AllowEyeWheel = DDRace || BlockWorlds || Plus;
	Info.m_AllowHookColl = DDRace;
	Info.m_AllowZoom = Race || BlockWorlds;
	Info.m_BugDDRaceGhost = DDRace;
	Info.m_BugDDRaceInput = DDRace;
	Info.m_BugFNGLaserRange = FNG;
	Info.m_BugVanillaBounce = Vanilla;
	Info.m_PredictFNG = FNG;
	Info.m_PredictDDRace = DDRace;
	Info.m_PredictDDRaceTiles = DDRace && !BlockWorlds;
	Info.m_PredictVanilla = Vanilla || FastCap;
	Info.m_EntitiesDDNet = DDNet;
	Info.m_EntitiesDDRace = DDRace;
	Info.m_EntitiesRace = Race;
	Info.m_EntitiesFNG = FNG;
	Info.m_EntitiesVanilla = Vanilla;
	Info.m_Race = Race;
	Info.m_DontMaskEntities = !DDNet;

	if(Version >= 0)
	{
		Info.m_TimeScore = Flags&GAMEINFOFLAG_TIMESCORE;
	}
	if(Version >= 2)
	{
		Info.m_FlagStartsRace = Flags&GAMEINFOFLAG_FLAG_STARTS_RACE;
		Info.m_UnlimitedAmmo = Flags&GAMEINFOFLAG_UNLIMITED_AMMO;
		Info.m_DDRaceRecordMessage = Flags&GAMEINFOFLAG_DDRACE_RECORD_MESSAGE;
		Info.m_RaceRecordMessage = Flags&GAMEINFOFLAG_RACE_RECORD_MESSAGE;
		Info.m_AllowEyeWheel = Flags&GAMEINFOFLAG_ALLOW_EYE_WHEEL;
		Info.m_AllowHookColl = Flags&GAMEINFOFLAG_ALLOW_HOOK_COLL;
		Info.m_AllowZoom = Flags&GAMEINFOFLAG_ALLOW_ZOOM;
		Info.m_BugDDRaceGhost = Flags&GAMEINFOFLAG_BUG_DDRACE_GHOST;
		Info.m_BugDDRaceInput = Flags&GAMEINFOFLAG_BUG_DDRACE_INPUT;
		Info.m_BugFNGLaserRange = Flags&GAMEINFOFLAG_BUG_FNG_LASER_RANGE;
		Info.m_BugVanillaBounce = Flags&GAMEINFOFLAG_BUG_VANILLA_BOUNCE;
		Info.m_PredictFNG = Flags&GAMEINFOFLAG_PREDICT_FNG;
		Info.m_PredictDDRace = Flags&GAMEINFOFLAG_PREDICT_DDRACE;
		Info.m_PredictDDRaceTiles = Flags&GAMEINFOFLAG_PREDICT_DDRACE_TILES;
		Info.m_PredictVanilla = Flags&GAMEINFOFLAG_PREDICT_VANILLA;
		Info.m_EntitiesDDNet = Flags&GAMEINFOFLAG_ENTITIES_DDNET;
		Info.m_EntitiesDDRace = Flags&GAMEINFOFLAG_ENTITIES_DDRACE;
		Info.m_EntitiesRace = Flags&GAMEINFOFLAG_ENTITIES_RACE;
		Info.m_EntitiesFNG = Flags&GAMEINFOFLAG_ENTITIES_FNG;
		Info.m_EntitiesVanilla = Flags&GAMEINFOFLAG_ENTITIES_VANILLA;
	}
	if(Version >= 3)
	{
		Info.m_Race = Flags&GAMEINFOFLAG_RACE;
		Info.m_DontMaskEntities = Flags&GAMEINFOFLAG_DONT_MASK_ENTITIES;
	}
	return Info;
}

void CGameClient::OnNewSnapshot()
{
	m_NewTick = true;

	// clear out the invalid pointers
	mem_zero(&g_GameClient.m_Snap, sizeof(g_GameClient.m_Snap));
	m_Snap.m_LocalClientID = -1;

	// secure snapshot
	{
		int Num = Client()->SnapNumItems(IClient::SNAP_CURRENT);
		for(int Index = 0; Index < Num; Index++)
		{
			IClient::CSnapItem Item;
			void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, Index, &Item);
			if(m_NetObjHandler.ValidateObj(Item.m_Type, pData, Item.m_DataSize) != 0)
			{
				if(g_Config.m_Debug && Item.m_Type != UUID_UNKNOWN)
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "invalidated index=%d type=%d (%s) size=%d id=%d", Index, Item.m_Type, m_NetObjHandler.GetObjName(Item.m_Type), Item.m_DataSize, Item.m_ID);
					Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
				}
				Client()->SnapInvalidateItem(IClient::SNAP_CURRENT, Index);
			}
		}
	}

	ProcessEvents();

#ifdef CONF_DEBUG
	if(g_Config.m_DbgStress)
	{
		if((Client()->GameTick()%100) == 0)
		{
			char aMessage[64];
			int MsgLen = rand()%(sizeof(aMessage)-1);
			for(int i = 0; i < MsgLen; i++)
				aMessage[i] = (char)('a' + (rand() % ('z' - 'a')));
			aMessage[MsgLen] = 0;

			CNetMsg_Cl_Say Msg;
			Msg.m_Team = rand()&1;
			Msg.m_pMessage = aMessage;
			Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
		}
	}
#endif

	bool FoundGameInfoEx = false;

	// go trough all the items in the snapshot and gather the info we want
	{
		m_Snap.m_aTeamSize[TEAM_RED] = m_Snap.m_aTeamSize[TEAM_BLUE] = 0;

		int Num = Client()->SnapNumItems(IClient::SNAP_CURRENT);
		for(int i = 0; i < Num; i++)
		{
			IClient::CSnapItem Item;
			const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

			if(Item.m_Type == NETOBJTYPE_CLIENTINFO)
			{
				const CNetObj_ClientInfo *pInfo = (const CNetObj_ClientInfo *)pData;
				int ClientID = Item.m_ID;
				IntsToStr(&pInfo->m_Name0, 4, m_aClients[ClientID].m_aName);
				IntsToStr(&pInfo->m_Clan0, 3, m_aClients[ClientID].m_aClan);
				m_aClients[ClientID].m_Country = pInfo->m_Country;
				IntsToStr(&pInfo->m_Skin0, 6, m_aClients[ClientID].m_aSkinName);

				m_aClients[ClientID].m_UseCustomColor = pInfo->m_UseCustomColor;
				m_aClients[ClientID].m_ColorBody = pInfo->m_ColorBody;
				m_aClients[ClientID].m_ColorFeet = pInfo->m_ColorFeet;

				// prepare the info
				if(m_aClients[ClientID].m_aSkinName[0] == 'x' || m_aClients[ClientID].m_aSkinName[1] == '_')
					str_copy(m_aClients[ClientID].m_aSkinName, "default", 64);

				m_aClients[ClientID].m_SkinInfo.m_ColorBody = color_cast<ColorRGBA>(ColorHSLA(m_aClients[ClientID].m_ColorBody).Lighten());
				m_aClients[ClientID].m_SkinInfo.m_ColorFeet = color_cast<ColorRGBA>(ColorHSLA(m_aClients[ClientID].m_ColorFeet).Lighten());
				m_aClients[ClientID].m_SkinInfo.m_Size = 64;

				// find new skin
				m_aClients[ClientID].m_SkinID = g_GameClient.m_pSkins->Find(m_aClients[ClientID].m_aSkinName);

				if(m_aClients[ClientID].m_UseCustomColor)
					m_aClients[ClientID].m_SkinInfo.m_Texture = g_GameClient.m_pSkins->Get(m_aClients[ClientID].m_SkinID)->m_ColorTexture;
				else
				{
					m_aClients[ClientID].m_SkinInfo.m_Texture = g_GameClient.m_pSkins->Get(m_aClients[ClientID].m_SkinID)->m_OrgTexture;
					m_aClients[ClientID].m_SkinInfo.m_ColorBody = ColorRGBA(1,1,1);
					m_aClients[ClientID].m_SkinInfo.m_ColorFeet = ColorRGBA(1,1,1);
				}

				m_aClients[ClientID].UpdateRenderInfo();

			}
			else if(Item.m_Type == NETOBJTYPE_PLAYERINFO)
			{
				const CNetObj_PlayerInfo *pInfo = (const CNetObj_PlayerInfo *)pData;

				m_aClients[pInfo->m_ClientID].m_Team = pInfo->m_Team;
				m_aClients[pInfo->m_ClientID].m_Active = true;
				m_Snap.m_paPlayerInfos[pInfo->m_ClientID] = pInfo;
				m_Snap.m_NumPlayers++;

				if(pInfo->m_Local)
				{
					m_Snap.m_LocalClientID = Item.m_ID;
					m_Snap.m_pLocalInfo = pInfo;

					if(pInfo->m_Team == TEAM_SPECTATORS)
					{
						m_Snap.m_SpecInfo.m_Active = true;
						m_Snap.m_SpecInfo.m_SpectatorID = SPEC_FREEVIEW;
					}
				}

				// calculate team-balance
				if(pInfo->m_Team != TEAM_SPECTATORS)
				{
					m_Snap.m_aTeamSize[pInfo->m_Team]++;
					if(!m_aStats[pInfo->m_ClientID].IsActive())
						m_aStats[pInfo->m_ClientID].JoinGame(Client()->GameTick());
				}
				else if(m_aStats[pInfo->m_ClientID].IsActive())
					m_aStats[pInfo->m_ClientID].JoinSpec(Client()->GameTick());

			}
			else if(Item.m_Type == NETOBJTYPE_DDNETPLAYER)
			{
				const CNetObj_DDNetPlayer *pInfo = (const CNetObj_DDNetPlayer *)pData;
				m_aClients[Item.m_ID].m_AuthLevel = pInfo->m_AuthLevel;
				m_aClients[Item.m_ID].m_Afk = pInfo->m_Flags & EXPLAYERFLAG_AFK;
			}
			else if(Item.m_Type == NETOBJTYPE_CHARACTER)
			{
				const void *pOld = Client()->SnapFindItem(IClient::SNAP_PREV, NETOBJTYPE_CHARACTER, Item.m_ID);
				m_Snap.m_aCharacters[Item.m_ID].m_Cur = *((const CNetObj_Character *)pData);
				if(pOld)
				{
					m_Snap.m_aCharacters[Item.m_ID].m_Active = true;
					m_Snap.m_aCharacters[Item.m_ID].m_Prev = *((const CNetObj_Character *)pOld);

					if(m_Snap.m_aCharacters[Item.m_ID].m_Prev.m_Tick)
						Evolve(&m_Snap.m_aCharacters[Item.m_ID].m_Prev, Client()->PrevGameTick());
					if(m_Snap.m_aCharacters[Item.m_ID].m_Cur.m_Tick)
						Evolve(&m_Snap.m_aCharacters[Item.m_ID].m_Cur, Client()->GameTick());
				}
			}
			else if(Item.m_Type == NETOBJTYPE_DDNETCHARACTER)
			{
				const CNetObj_DDNetCharacter *pCharacterData = (const CNetObj_DDNetCharacter *)pData;

				m_Snap.m_aCharacters[Item.m_ID].m_ExtendedData = *pCharacterData;
				m_Snap.m_aCharacters[Item.m_ID].m_HasExtendedData = true;

				// Collision
				m_aClients[Item.m_ID].m_Solo = pCharacterData->m_Flags & CHARACTERFLAG_SOLO;
				m_aClients[Item.m_ID].m_NoCollision = pCharacterData->m_Flags & CHARACTERFLAG_NO_COLLISION;
				m_aClients[Item.m_ID].m_NoHammerHit = pCharacterData->m_Flags & CHARACTERFLAG_NO_HAMMER_HIT;
				m_aClients[Item.m_ID].m_NoGrenadeHit = pCharacterData->m_Flags & CHARACTERFLAG_NO_GRENADE_HIT;
				m_aClients[Item.m_ID].m_NoRifleHit = pCharacterData->m_Flags & CHARACTERFLAG_NO_RIFLE_HIT;
				m_aClients[Item.m_ID].m_NoShotgunHit = pCharacterData->m_Flags & CHARACTERFLAG_NO_SHOTGUN_HIT;
				m_aClients[Item.m_ID].m_NoHookHit = pCharacterData->m_Flags & CHARACTERFLAG_NO_HOOK;
				m_aClients[Item.m_ID].m_Super = pCharacterData->m_Flags & CHARACTERFLAG_SUPER;

				// Endless
				m_aClients[Item.m_ID].m_EndlessHook = pCharacterData->m_Flags & CHARACTERFLAG_ENDLESS_HOOK;
				m_aClients[Item.m_ID].m_EndlessJump = pCharacterData->m_Flags & CHARACTERFLAG_ENDLESS_JUMP;

				// Freeze
				m_aClients[Item.m_ID].m_FreezeEnd = pCharacterData->m_FreezeEnd;
				m_aClients[Item.m_ID].m_DeepFrozen = pCharacterData->m_FreezeEnd == -1;

				// Telegun
				m_aClients[Item.m_ID].m_HasTelegunGrenade = pCharacterData->m_Flags & CHARACTERFLAG_TELEGUN_GRENADE;
				m_aClients[Item.m_ID].m_HasTelegunGun = pCharacterData->m_Flags & CHARACTERFLAG_TELEGUN_GUN;
				m_aClients[Item.m_ID].m_HasTelegunLaser = pCharacterData->m_Flags & CHARACTERFLAG_TELEGUN_LASER;

				m_aClients[Item.m_ID].m_Predicted.ReadDDNet(pCharacterData);
			}
			else if(Item.m_Type == NETOBJTYPE_SPECTATORINFO)
			{
				m_Snap.m_pSpectatorInfo = (const CNetObj_SpectatorInfo *)pData;
				m_Snap.m_pPrevSpectatorInfo = (const CNetObj_SpectatorInfo *)Client()->SnapFindItem(IClient::SNAP_PREV, NETOBJTYPE_SPECTATORINFO, Item.m_ID);

				m_Snap.m_SpecInfo.m_SpectatorID = m_Snap.m_pSpectatorInfo->m_SpectatorID;
			}
			else if(Item.m_Type == NETOBJTYPE_GAMEINFO)
			{
				static bool s_GameOver = 0;
				static bool s_GamePaused = 0;
				m_Snap.m_pGameInfoObj = (const CNetObj_GameInfo *)pData;
				bool CurrentTickGameOver = (bool)(m_Snap.m_pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER);
				if(!s_GameOver && CurrentTickGameOver)
					OnGameOver();
				else if(s_GameOver && !CurrentTickGameOver)
					OnStartGame();
				// Reset statboard when new round is started (RoundStartTick changed)
				// New round is usually started after `restart` on server
				if(m_Snap.m_pGameInfoObj->m_RoundStartTick != m_LastRoundStartTick
						// In GamePaused or GameOver state RoundStartTick is updated on each tick
						// hence no need to reset stats until player leaves GameOver
						// and it would be a mistake to reset stats after or during the pause
						&& !(CurrentTickGameOver || m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_PAUSED || s_GamePaused))
					m_pStatboard->OnReset();
				m_LastRoundStartTick = m_Snap.m_pGameInfoObj->m_RoundStartTick;
				s_GameOver = CurrentTickGameOver;
				s_GamePaused = (bool)(m_Snap.m_pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_PAUSED);
			}
			else if(Item.m_Type == NETOBJTYPE_GAMEINFOEX)
			{
				if(FoundGameInfoEx)
				{
					continue;
				}
				FoundGameInfoEx = true;
				CServerInfo ServerInfo;
				Client()->GetServerInfo(&ServerInfo);
				m_GameInfo = GetGameInfo((const CNetObj_GameInfoEx *)pData, Client()->SnapItemSize(IClient::SNAP_CURRENT, i), &ServerInfo);
			}
			else if(Item.m_Type == NETOBJTYPE_GAMEDATA)
			{
				m_Snap.m_pGameDataObj = (const CNetObj_GameData *)pData;
				m_Snap.m_GameDataSnapID = Item.m_ID;
				if(m_Snap.m_pGameDataObj->m_FlagCarrierRed == FLAG_TAKEN)
				{
					if(m_FlagDropTick[TEAM_RED] == 0)
						m_FlagDropTick[TEAM_RED] = Client()->GameTick();
				}
				else if(m_FlagDropTick[TEAM_RED] != 0)
						m_FlagDropTick[TEAM_RED] = 0;
				if(m_Snap.m_pGameDataObj->m_FlagCarrierBlue == FLAG_TAKEN)
				{
					if(m_FlagDropTick[TEAM_BLUE] == 0)
						m_FlagDropTick[TEAM_BLUE] = Client()->GameTick();
				}
				else if(m_FlagDropTick[TEAM_BLUE] != 0)
						m_FlagDropTick[TEAM_BLUE] = 0;
				if(m_LastFlagCarrierRed == FLAG_ATSTAND && m_Snap.m_pGameDataObj->m_FlagCarrierRed >= 0)
					OnFlagGrab(TEAM_RED);
				else if(m_LastFlagCarrierBlue == FLAG_ATSTAND && m_Snap.m_pGameDataObj->m_FlagCarrierBlue >= 0)
					OnFlagGrab(TEAM_BLUE);

				m_LastFlagCarrierRed = m_Snap.m_pGameDataObj->m_FlagCarrierRed;
				m_LastFlagCarrierBlue = m_Snap.m_pGameDataObj->m_FlagCarrierBlue;
			}
			else if(Item.m_Type == NETOBJTYPE_FLAG)
				m_Snap.m_paFlags[Item.m_ID%2] = (const CNetObj_Flag *)pData;
		}
	}

	if(!FoundGameInfoEx)
	{
		CServerInfo ServerInfo;
		Client()->GetServerInfo(&ServerInfo);
		m_GameInfo = GetGameInfo(0, 0, &ServerInfo);
	}

	// setup local pointers
	if(m_Snap.m_LocalClientID >= 0)
	{
		CSnapState::CCharacterInfo *c = &m_Snap.m_aCharacters[m_Snap.m_LocalClientID];
		if(c->m_Active)
		{
			if(!m_Snap.m_SpecInfo.m_Active)
			{
				m_Snap.m_pLocalCharacter = &c->m_Cur;
				m_Snap.m_pLocalPrevCharacter = &c->m_Prev;
				m_LocalCharacterPos = vec2(m_Snap.m_pLocalCharacter->m_X, m_Snap.m_pLocalCharacter->m_Y);
			}
		}
		else if(Client()->SnapFindItem(IClient::SNAP_PREV, NETOBJTYPE_CHARACTER, m_Snap.m_LocalClientID))
		{
			// player died
			m_pControls->OnPlayerDeath();
		}
	}
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		if(m_DemoSpecID != SPEC_FOLLOW)
		{
			m_Snap.m_SpecInfo.m_Active = true;
			m_Snap.m_SpecInfo.m_SpectatorID = m_Snap.m_LocalClientID;
			if(m_DemoSpecID > SPEC_FREEVIEW && m_Snap.m_aCharacters[m_DemoSpecID].m_Active)
				m_Snap.m_SpecInfo.m_SpectatorID = m_DemoSpecID;
			else
				m_Snap.m_SpecInfo.m_SpectatorID = SPEC_FREEVIEW;
		}
	}

	// clear out unneeded client data
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(!m_Snap.m_paPlayerInfos[i] && m_aClients[i].m_Active)
		{
			m_aClients[i].Reset();
			m_aStats[i].Reset();
		}
	}

	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		// update friend state
		m_aClients[i].m_Friend = !(i == m_Snap.m_LocalClientID
			|| !m_Snap.m_paPlayerInfos[i]
			|| !Friends()->IsFriend(m_aClients[i].m_aName, m_aClients[i].m_aClan, true));

		// update foe state
		m_aClients[i].m_Foe = !(i == m_Snap.m_LocalClientID
			|| !m_Snap.m_paPlayerInfos[i]
			|| !Foes()->IsFriend(m_aClients[i].m_aName, m_aClients[i].m_aClan, true));
	}

	// sort player infos by name
	mem_copy(m_Snap.m_paInfoByName, m_Snap.m_paPlayerInfos, sizeof(m_Snap.m_paInfoByName));
	std::stable_sort(m_Snap.m_paInfoByName, m_Snap.m_paInfoByName + MAX_CLIENTS,
		[this](const CNetObj_PlayerInfo* p1, const CNetObj_PlayerInfo* p2) -> bool
		{
			if (!p2)
				return static_cast<bool>(p1);
			if (!p1)
				return false;
			return str_comp_nocase(m_aClients[p1->m_ClientID].m_aName, m_aClients[p2->m_ClientID].m_aName) < 0;
		});

	bool TimeScore = m_GameInfo.m_TimeScore;

	// sort player infos by score
	mem_copy(m_Snap.m_paInfoByScore, m_Snap.m_paInfoByName, sizeof(m_Snap.m_paInfoByScore));
	std::stable_sort(m_Snap.m_paInfoByScore, m_Snap.m_paInfoByScore + MAX_CLIENTS,
		[TimeScore](const CNetObj_PlayerInfo* p1, const CNetObj_PlayerInfo* p2) -> bool
		{
			if (!p2)
				return static_cast<bool>(p1);
			if (!p1)
				return false;
			return (((TimeScore && p1->m_Score == -9999) ? std::numeric_limits<int>::min() : p1->m_Score) >
				((TimeScore && p2->m_Score == -9999) ? std::numeric_limits<int>::min() : p2->m_Score));
		});

	// sort player infos by DDRace Team (and score between)
	int Index = 0;
	for(int Team = 0; Team <= MAX_CLIENTS; ++Team)
	{
		for(int i = 0; i < MAX_CLIENTS && Index < MAX_CLIENTS; ++i)
		{
			if(m_Snap.m_paInfoByScore[i] && m_Teams.Team(m_Snap.m_paInfoByScore[i]->m_ClientID) == Team)
				m_Snap.m_paInfoByDDTeam[Index++] = m_Snap.m_paInfoByScore[i];
		}
	}

	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);
	CTuningParams StandardTuning;
	if(CurrentServerInfo.m_aGameType[0] != '0')
	{
		if(str_comp(CurrentServerInfo.m_aGameType, "DM") != 0 && str_comp(CurrentServerInfo.m_aGameType, "TDM") != 0 && str_comp(CurrentServerInfo.m_aGameType, "CTF") != 0)
			m_ServerMode = SERVERMODE_MOD;
		else if(mem_comp(&StandardTuning, &m_Tuning[g_Config.m_ClDummy], 33) == 0)
			m_ServerMode = SERVERMODE_PURE;
		else
			m_ServerMode = SERVERMODE_PUREMOD;
	}

	// add tuning to demo
	bool AnyRecording = false;
	for(int i = 0; i < RECORDER_MAX; i++)
		if(DemoRecorder(i)->IsRecording())
		{
			AnyRecording = true;
			break;
		}
	if(AnyRecording && mem_comp(&StandardTuning, &m_Tuning[g_Config.m_ClDummy], sizeof(CTuningParams)) != 0)
	{
		CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
		int *pParams = (int *)&m_Tuning[g_Config.m_ClDummy];
		for(unsigned i = 0; i < sizeof(m_Tuning[0])/sizeof(int); i++)
			Msg.AddInt(pParams[i]);
		Client()->SendMsg(&Msg, MSGFLAG_RECORD|MSGFLAG_NOSEND);
	}

	if(!m_DDRaceMsgSent[0] && m_Snap.m_pLocalInfo)
	{
		CMsgPacker Msg(NETMSGTYPE_CL_ISDDNET);
		Msg.AddInt(CLIENT_VERSIONNR);
		Client()->SendMsgExY(&Msg, MSGFLAG_VITAL,false, 0);
		m_DDRaceMsgSent[0] = true;
	}

	if(!m_DDRaceMsgSent[1] && m_Snap.m_pLocalInfo && Client()->DummyConnected())
	{
		CMsgPacker Msg(NETMSGTYPE_CL_ISDDNET);
		Msg.AddInt(CLIENT_VERSIONNR);
		Client()->SendMsgExY(&Msg, MSGFLAG_VITAL,false, 1);
		m_DDRaceMsgSent[1] = true;
	}

	if(m_ShowOthers[g_Config.m_ClDummy] == -1 || (m_ShowOthers[g_Config.m_ClDummy] != -1 && m_ShowOthers[g_Config.m_ClDummy] != g_Config.m_ClShowOthers))
	{
		// no need to send, default settings
		//if(!(m_ShowOthers == -1 && g_Config.m_ClShowOthers))
		{
			CNetMsg_Cl_ShowOthers Msg;
			Msg.m_Show = g_Config.m_ClShowOthers;
			Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
		}

		// update state
		m_ShowOthers[g_Config.m_ClDummy] = g_Config.m_ClShowOthers;
	}

	m_pGhost->OnNewSnapshot();
	m_pRaceDemo->OnNewSnapshot();

	// detect air jump for unpredicted players
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(m_Snap.m_aCharacters[i].m_Active && (m_Snap.m_aCharacters[i].m_Cur.m_Jumped&2) && !(m_Snap.m_aCharacters[i].m_Prev.m_Jumped&2))
			if(!Predict() || (!AntiPingPlayers() && i != m_Snap.m_LocalClientID))
			{
				vec2 Pos = mix(vec2(m_Snap.m_aCharacters[i].m_Prev.m_X, m_Snap.m_aCharacters[i].m_Prev.m_Y),
						vec2(m_Snap.m_aCharacters[i].m_Cur.m_X, m_Snap.m_aCharacters[i].m_Cur.m_Y),
						Client()->IntraGameTick());
				m_pEffects->AirJump(Pos);
			}

	// update prediction data
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
		UpdatePrediction();
}

void CGameClient::OnPredict()
{
	// store the previous values so we can detect prediction errors
	CCharacterCore BeforePrevChar = m_PredictedPrevChar;
	CCharacterCore BeforeChar = m_PredictedChar;

	// we can't predict without our own id or own character
	if(m_Snap.m_LocalClientID == -1 || !m_Snap.m_aCharacters[m_Snap.m_LocalClientID].m_Active)
		return;

	// don't predict anything if we are paused
	if(m_Snap.m_pGameInfoObj && m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_PAUSED)
	{
		if(m_Snap.m_pLocalCharacter)
		{
			m_PredictedChar.Read(m_Snap.m_pLocalCharacter);
			m_PredictedChar.m_ActiveWeapon = m_Snap.m_pLocalCharacter->m_Weapon;
		}
		if(m_Snap.m_pLocalPrevCharacter)
		{
			m_PredictedPrevChar.Read(m_Snap.m_pLocalPrevCharacter);
			m_PredictedPrevChar.m_ActiveWeapon = m_Snap.m_pLocalPrevCharacter->m_Weapon;
		}
		return;
	}

	vec2 aBeforeRender[MAX_CLIENTS];
	for(int i = 0; i < MAX_CLIENTS; i++)
		aBeforeRender[i] = GetSmoothPos(i);

	// init
	m_PredictedWorld.CopyWorld(&m_GameWorld);

	// don't predict inactive players
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(CCharacter *pChar = m_PredictedWorld.GetCharacterByID(i))
			if(!m_Snap.m_aCharacters[i].m_Active && pChar->m_SnapTicks > 10)
				pChar->Destroy();

	CCharacter *pLocalChar = m_PredictedWorld.GetCharacterByID(m_Snap.m_LocalClientID);
	if(!pLocalChar)
		return;

	// predict
	for(int Tick = Client()->GameTick() + 1; Tick <= Client()->PredGameTick(); Tick++)
	{
		// fetch the previous characters
		if(Tick == Client()->PredGameTick())
		{
			m_PrevPredictedWorld.CopyWorld(&m_PredictedWorld);
			m_PredictedPrevChar = pLocalChar->GetCore();
			for(int i = 0; i < MAX_CLIENTS; i++)
				if(CCharacter *pChar = m_PredictedWorld.GetCharacterByID(i))
					m_aClients[i].m_PrevPredicted = pChar->GetCore();
		}

		// optionally allow some movement in freeze by not predicting freeze the last one to two ticks
		if(g_Config.m_ClPredictFreeze == 2 && Client()->PredGameTick() - 1 - Client()->PredGameTick()%2 <= Tick)
			pLocalChar->m_CanMoveInFreeze = true;

		// apply inputs and tick
		CNetObj_PlayerInput *pInputData = (CNetObj_PlayerInput*) Client()->GetDirectInput(Tick);
		if(pInputData)
			pLocalChar->OnDirectInput(pInputData);
		m_PredictedWorld.m_GameTick = Tick;
		if(pInputData)
			pLocalChar->OnPredictedInput(pInputData);
		m_PredictedWorld.Tick();

		// fetch the current characters
		if(Tick == Client()->PredGameTick())
		{
			m_PredictedChar = pLocalChar->GetCore();
			for(int i = 0; i < MAX_CLIENTS; i++)
				if(CCharacter *pChar = m_PredictedWorld.GetCharacterByID(i))
					m_aClients[i].m_Predicted = pChar->GetCore();
		}

		for(int i = 0; i < MAX_CLIENTS; i++)
			if(CCharacter *pChar = m_PredictedWorld.GetCharacterByID(i))
			{
				m_aClients[i].m_PredPos[Tick % 200] = pChar->Core()->m_Pos;
				m_aClients[i].m_PredTick[Tick % 200] = Tick;
			}

		// check if we want to trigger effects
		if(Tick > m_LastNewPredictedTick[g_Config.m_ClDummy])
		{
			m_LastNewPredictedTick[g_Config.m_ClDummy] = Tick;
			m_NewPredictedTick = true;
			vec2 Pos = pLocalChar->Core()->m_Pos;
			int Events = pLocalChar->Core()->m_TriggeredEvents;
			if(g_Config.m_ClPredict)
				if(Events&COREEVENT_AIR_JUMP)
					m_pEffects->AirJump(Pos);
			if(g_Config.m_SndGame)
			{
				if(Events&COREEVENT_GROUND_JUMP)
					m_pSounds->PlayAndRecord(CSounds::CHN_WORLD, SOUND_PLAYER_JUMP, 1.0f, Pos);
				if(Events&COREEVENT_HOOK_ATTACH_GROUND)
					m_pSounds->PlayAndRecord(CSounds::CHN_WORLD, SOUND_HOOK_ATTACH_GROUND, 1.0f, Pos);
				if(Events&COREEVENT_HOOK_HIT_NOHOOK)
					m_pSounds->PlayAndRecord(CSounds::CHN_WORLD, SOUND_HOOK_NOATTACH, 1.0f, Pos);
			}
		}
	}

	// detect mispredictions of other players and make corrections smoother when possible
	static vec2 s_aLastPos[MAX_CLIENTS] = {{0,0}};
	static bool s_aLastActive[MAX_CLIENTS] = {0};

    if(g_Config.m_ClAntiPingSmooth && Predict() && AntiPingPlayers() && m_NewTick && abs(m_PredictedTick - Client()->PredGameTick()) <= 1 && abs(Client()->GameTick() - Client()->PrevGameTick()) <= 2)
	{
		int PredTime = clamp(Client()->GetPredictionTime(), 0, 800);
		float SmoothPace = 4 - 1.5f * PredTime/800.f; // smoothing pace (a lower value will make the smoothing quicker)
		int64 Len = 1000 * PredTime * SmoothPace;

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!m_Snap.m_aCharacters[i].m_Active || i == m_Snap.m_LocalClientID || !s_aLastActive[i])
				continue;
			vec2 NewPos = (m_PredictedTick == Client()->PredGameTick()) ? m_aClients[i].m_Predicted.m_Pos : m_aClients[i].m_PrevPredicted.m_Pos;
			vec2 PredErr = (s_aLastPos[i] - NewPos)/(float)minimum(Client()->GetPredictionTime(), 200);
			if(in_range(length(PredErr), 0.05f, 5.f))
			{
				vec2 PredPos = mix(m_aClients[i].m_PrevPredicted.m_Pos, m_aClients[i].m_Predicted.m_Pos, Client()->PredIntraGameTick());
				vec2 CurPos = mix(
						vec2(m_Snap.m_aCharacters[i].m_Prev.m_X, m_Snap.m_aCharacters[i].m_Prev.m_Y),
						vec2(m_Snap.m_aCharacters[i].m_Cur.m_X, m_Snap.m_aCharacters[i].m_Cur.m_Y),
						Client()->IntraGameTick());
				vec2 RenderDiff = PredPos - aBeforeRender[i];
				vec2 PredDiff = PredPos - CurPos;

				float MixAmount[2];
				for(int j = 0; j < 2; j++)
				{
					MixAmount[j] = 1.0f;
					if(fabs(PredErr[j]) > 0.05f)
					{
						MixAmount[j] = 0.0f;
						if(fabs(RenderDiff[j]) > 0.01f)
						{
							MixAmount[j] = 1.f - clamp(RenderDiff[j] / PredDiff[j], 0.f, 1.f);
							MixAmount[j] = 1.f - powf(1.f - MixAmount[j], 1/1.2f);
						}
					}
					int64 TimePassed = time_get() - m_aClients[i].m_SmoothStart[j];
					if(in_range(TimePassed, (int64)0, Len-1))
						MixAmount[j] = minimum(MixAmount[j], (float)(TimePassed/(double)Len));

				}
				for(int j = 0; j < 2; j++)
					if(fabs(RenderDiff[j]) < 0.01f && fabs(PredDiff[j]) < 0.01f && fabs(m_aClients[i].m_PrevPredicted.m_Pos[j] - m_aClients[i].m_Predicted.m_Pos[j]) < 0.01f && MixAmount[j] > MixAmount[j^1])
						MixAmount[j] = MixAmount[j^1];
				for(int j = 0; j < 2; j++)
				{
					int64 Remaining = minimum((1.f-MixAmount[j])*Len, minimum(time_freq()*0.700f, (1.f-MixAmount[j^1])*Len + time_freq()*0.300f)); // don't smooth for longer than 700ms, or more than 300ms longer along one axis than the other axis
					int64 Start = time_get() - (Len - Remaining);
					if(!in_range(Start + Len, m_aClients[i].m_SmoothStart[j], m_aClients[i].m_SmoothStart[j] + Len))
					{
						m_aClients[i].m_SmoothStart[j] = Start;
						m_aClients[i].m_SmoothLen[j] = Len;
					}
				}
			}
		}
	}

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_Snap.m_aCharacters[i].m_Active)
		{
			s_aLastPos[i] = m_aClients[i].m_Predicted.m_Pos;
			s_aLastActive[i] = true;
		}
		else
			s_aLastActive[i] = false;
	}

	if(g_Config.m_Debug && g_Config.m_ClPredict && m_PredictedTick == Client()->PredGameTick())
	{
		CNetObj_CharacterCore Before = {0}, Now = {0}, BeforePrev = {0}, NowPrev = {0};
		BeforeChar.Write(&Before);
		BeforePrevChar.Write(&BeforePrev);
		m_PredictedChar.Write(&Now);
		m_PredictedPrevChar.Write(&NowPrev);

		if(mem_comp(&Before, &Now, sizeof(CNetObj_CharacterCore)) != 0)
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", "prediction error");
			for(unsigned i = 0; i < sizeof(CNetObj_CharacterCore)/sizeof(int); i++)
				if(((int *)&Before)[i] != ((int *)&Now)[i])
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "	%d %d %d (%d %d)", i, ((int *)&Before)[i], ((int *)&Now)[i], ((int *)&BeforePrev)[i], ((int *)&NowPrev)[i]);
					Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", aBuf);
				}
		}
	}

	m_PredictedTick = Client()->PredGameTick();

	if(m_NewPredictedTick)
		m_pGhost->OnNewPredictedSnapshot();
}

void CGameClient::OnActivateEditor()
{
	OnRelease();
}

CGameClient::CClientStats::CClientStats()
{
	Reset();
}

void CGameClient::CClientStats::Reset()
{
	m_JoinTick = 0;
	m_IngameTicks = 0;
	m_Active = false;
	m_Frags = 0;
	m_Deaths = 0;
	m_Suicides = 0;
	m_BestSpree = 0;
	m_CurrentSpree = 0;
	for(int j = 0; j < NUM_WEAPONS; j++)
	{
		m_aFragsWith[j] = 0;
		m_aDeathsFrom[j] = 0;
	}
	m_FlagGrabs = 0;
	m_FlagCaptures = 0;
}

void CGameClient::CClientData::UpdateRenderInfo()
{
	m_RenderInfo = m_SkinInfo;

	// force team colors
	if(g_GameClient.m_Snap.m_pGameInfoObj && g_GameClient.m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS)
	{
		m_RenderInfo.m_Texture = g_GameClient.m_pSkins->Get(m_SkinID)->m_ColorTexture;
		const int TeamColors[2] = {65461, 10223541};
		if(m_Team >= TEAM_RED && m_Team <= TEAM_BLUE)
		{
			m_RenderInfo.m_ColorBody = color_cast<ColorRGBA>(ColorHSLA(TeamColors[m_Team]));
			m_RenderInfo.m_ColorFeet = color_cast<ColorRGBA>(ColorHSLA(TeamColors[m_Team]));
		}
		else
		{
			m_RenderInfo.m_ColorBody = color_cast<ColorRGBA>(ColorHSLA(12829350));
			m_RenderInfo.m_ColorFeet = color_cast<ColorRGBA>(ColorHSLA(12829350));
		}
	}
}

void CGameClient::CClientData::Reset()
{
	m_aName[0] = 0;
	m_aClan[0] = 0;
	m_Country = -1;
	m_SkinID = 0;
	m_Team = 0;
	m_Angle = 0;
	m_Emoticon = 0;
	m_EmoticonStart = -1;
	m_Active = false;
	m_ChatIgnore = false;
	m_Friend = false;
	m_Foe = false;
	m_AuthLevel = AUTHED_NO;
	m_Afk = false;
	m_SkinInfo.m_Texture = g_GameClient.m_pSkins->Get(0)->m_ColorTexture;
	m_SkinInfo.m_ColorBody = ColorRGBA(1,1,1);
	m_SkinInfo.m_ColorFeet = ColorRGBA(1,1,1);

	m_Solo = false;
	m_Jetpack = false;
	m_NoCollision = false;
	m_EndlessHook = false;
	m_EndlessJump = false;
	m_NoHammerHit = false;
	m_NoGrenadeHit = false;
	m_NoRifleHit = false;
	m_NoShotgunHit = false;
	m_NoHookHit = false;
	m_Super = false;
	m_HasTelegunGun = false;
	m_HasTelegunGrenade = false;
	m_HasTelegunLaser = false;
	m_FreezeEnd = 0;
	m_DeepFrozen = false;

	UpdateRenderInfo();
}

void CGameClient::SendSwitchTeam(int Team)
{
	CNetMsg_Cl_SetTeam Msg;
	Msg.m_Team = Team;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);

	if(Team != TEAM_SPECTATORS)
		m_pCamera->OnReset();
}

void CGameClient::SendInfo(bool Start)
{
	if(Start)
	{
		CNetMsg_Cl_StartInfo Msg;
		Msg.m_pName = g_Config.m_PlayerName;
		Msg.m_pClan = g_Config.m_PlayerClan;
		Msg.m_Country = g_Config.m_PlayerCountry;
		Msg.m_pSkin = g_Config.m_ClPlayerSkin;
		Msg.m_UseCustomColor = g_Config.m_ClPlayerUseCustomColor;
		Msg.m_ColorBody = g_Config.m_ClPlayerColorBody;
		Msg.m_ColorFeet = g_Config.m_ClPlayerColorFeet;
		CMsgPacker Packer(Msg.MsgID());
		Msg.Pack(&Packer);
		Client()->SendMsgExY(&Packer, MSGFLAG_VITAL, false, 0);
		m_CheckInfo[0] = -1;
	}
	else
	{
		CNetMsg_Cl_ChangeInfo Msg;
		Msg.m_pName = g_Config.m_PlayerName;
		Msg.m_pClan = g_Config.m_PlayerClan;
		Msg.m_Country = g_Config.m_PlayerCountry;
		Msg.m_pSkin = g_Config.m_ClPlayerSkin;
		Msg.m_UseCustomColor = g_Config.m_ClPlayerUseCustomColor;
		Msg.m_ColorBody = g_Config.m_ClPlayerColorBody;
		Msg.m_ColorFeet = g_Config.m_ClPlayerColorFeet;
		CMsgPacker Packer(Msg.MsgID());
		Msg.Pack(&Packer);
		Client()->SendMsgExY(&Packer, MSGFLAG_VITAL, false, 0);
		m_CheckInfo[0] = Client()->GameTickSpeed();
	}
}

void CGameClient::SendDummyInfo(bool Start)
{
	if(Start)
	{
		CNetMsg_Cl_StartInfo Msg;
		Msg.m_pName = g_Config.m_ClDummyName;
		Msg.m_pClan = g_Config.m_ClDummyClan;
		Msg.m_Country = g_Config.m_ClDummyCountry;
		Msg.m_pSkin = g_Config.m_ClDummySkin;
		Msg.m_UseCustomColor = g_Config.m_ClDummyUseCustomColor;
		Msg.m_ColorBody = g_Config.m_ClDummyColorBody;
		Msg.m_ColorFeet = g_Config.m_ClDummyColorFeet;
		CMsgPacker Packer(Msg.MsgID());
		Msg.Pack(&Packer);
		Client()->SendMsgExY(&Packer, MSGFLAG_VITAL, false, 1);
		m_CheckInfo[1] = -1;
	}
	else
	{
		CNetMsg_Cl_ChangeInfo Msg;
		Msg.m_pName = g_Config.m_ClDummyName;
		Msg.m_pClan = g_Config.m_ClDummyClan;
		Msg.m_Country = g_Config.m_ClDummyCountry;
		Msg.m_pSkin = g_Config.m_ClDummySkin;
		Msg.m_UseCustomColor = g_Config.m_ClDummyUseCustomColor;
		Msg.m_ColorBody = g_Config.m_ClDummyColorBody;
		Msg.m_ColorFeet = g_Config.m_ClDummyColorFeet;
		CMsgPacker Packer(Msg.MsgID());
		Msg.Pack(&Packer);
		Client()->SendMsgExY(&Packer, MSGFLAG_VITAL,false, 1);
		m_CheckInfo[1] = Client()->GameTickSpeed();
	}
}

void CGameClient::SendKill(int ClientID)
{
	CNetMsg_Cl_Kill Msg;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);

	if(g_Config.m_ClDummyCopyMoves)
	{
		CMsgPacker Msg(NETMSGTYPE_CL_KILL);
		Client()->SendMsgExY(&Msg, MSGFLAG_VITAL, false, !g_Config.m_ClDummy);
	}
}

void CGameClient::ConTeam(IConsole::IResult *pResult, void *pUserData)
{
	((CGameClient*)pUserData)->SendSwitchTeam(pResult->GetInteger(0));
}

void CGameClient::ConKill(IConsole::IResult *pResult, void *pUserData)
{
	((CGameClient*)pUserData)->SendKill(-1);
}

void CGameClient::ConchainSpecialInfoupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
		((CGameClient*)pUserData)->SendInfo(false);
}

void CGameClient::ConchainSpecialDummyInfoupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
		((CGameClient*)pUserData)->SendDummyInfo(false);
}

void CGameClient::ConchainSpecialDummy(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
		if(g_Config.m_ClDummy && !((CGameClient*)pUserData)->Client()->DummyConnected())
			g_Config.m_ClDummy = 0;
}

void CGameClient::ConchainClTextEntitiesSize(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);

	if(pResult->NumArguments())
	{
		CGameClient *pGameClient = (CGameClient*)pUserData;
		pGameClient->m_pMapimages->SetTextureScale(g_Config.m_ClTextEntitiesSize);
	}
}

IGameClient *CreateGameClient()
{
	return &g_GameClient;
}

int CGameClient::IntersectCharacter(vec2 HookPos, vec2 NewPos, vec2& NewPos2, int ownID)
{
	float PhysSize = 28.0f;
	float Distance = 0.0f;
	int ClosestID = -1;

	CClientData OwnClientData = m_aClients[ownID];

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(i == ownID)
			continue;

		CClientData cData = m_aClients[i];

		if(!cData.m_Active)
			continue;

		CNetObj_Character Prev = m_Snap.m_aCharacters[i].m_Prev;
		CNetObj_Character Player = m_Snap.m_aCharacters[i].m_Cur;

		vec2 Position = mix(vec2(Prev.m_X, Prev.m_Y), vec2(Player.m_X, Player.m_Y), Client()->IntraGameTick());

		bool IsOneSuper = cData.m_Super || OwnClientData.m_Super;
		bool IsOneSolo = cData.m_Solo || OwnClientData.m_Solo;

		if(!IsOneSuper && (!m_Teams.SameTeam(i, ownID) || IsOneSolo || OwnClientData.m_NoHookHit))
			continue;

		vec2 ClosestPoint = closest_point_on_line(HookPos, NewPos, Position);
		if(distance(Position, ClosestPoint) < PhysSize + 2.0f)
		{
			if(ClosestID == -1 || distance(HookPos, Position) < Distance)
			{
				NewPos2 = ClosestPoint;
				ClosestID = i;
				Distance = distance(HookPos, Position);
			}
		}
	}

	return ClosestID;
}

ColorRGBA CalculateNameColor(ColorHSLA TextColorHSL)
{
	return color_cast<ColorRGBA>(ColorHSLA(TextColorHSL.h, TextColorHSL.s * 0.68f, TextColorHSL.l * 0.81f));
}

void CGameClient::UpdatePrediction()
{
	if(!m_Snap.m_pLocalCharacter)
	{
		if(CCharacter *pLocalChar = m_GameWorld.GetCharacterByID(m_Snap.m_LocalClientID))
			pLocalChar->Destroy();
		return;
	}

	m_GameWorld.m_WorldConfig.m_IsVanilla = m_GameInfo.m_PredictVanilla;
	m_GameWorld.m_WorldConfig.m_IsDDRace = m_GameInfo.m_PredictDDRace;
	m_GameWorld.m_WorldConfig.m_IsFNG = m_GameInfo.m_PredictFNG;
	m_GameWorld.m_WorldConfig.m_PredictDDRace = g_Config.m_ClPredictDDRace;
	m_GameWorld.m_WorldConfig.m_PredictTiles = g_Config.m_ClPredictDDRace && m_GameInfo.m_PredictDDRaceTiles;
	m_GameWorld.m_WorldConfig.m_PredictFreeze = g_Config.m_ClPredictFreeze;
	m_GameWorld.m_WorldConfig.m_PredictWeapons = AntiPingWeapons();
	if(m_Snap.m_pLocalCharacter->m_AmmoCount > 0 && m_Snap.m_pLocalCharacter->m_Weapon != WEAPON_NINJA)
		m_GameWorld.m_WorldConfig.m_InfiniteAmmo = false;
	m_GameWorld.m_WorldConfig.m_IsSolo = !m_Snap.m_aCharacters[m_Snap.m_LocalClientID].m_HasExtendedData && !m_Tuning[g_Config.m_ClDummy].m_PlayerCollision && !m_Tuning[g_Config.m_ClDummy].m_PlayerHooking;

	m_GameWorld.m_Core.m_Tuning[g_Config.m_ClDummy] = m_Tuning[g_Config.m_ClDummy];

	// restore characters from previously saved ones if they temporarily left the snapshot
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(m_aLastWorldCharacters[i].IsAlive() && m_Snap.m_aCharacters[i].m_Active && !m_GameWorld.GetCharacterByID(i))
			if(CCharacter *pCopy = new CCharacter(m_aLastWorldCharacters[i]))
			{
				m_GameWorld.InsertEntity(pCopy);
				if(pCopy->m_FreezeTime > 0)
					pCopy->m_FreezeTime = 0;
				if(pCopy->Core()->m_HookedPlayer > 0)
				{
					pCopy->Core()->m_HookedPlayer = -1;
					pCopy->Core()->m_HookState = HOOK_IDLE;
				}
			}

	CCharacter *pLocalChar = m_GameWorld.GetCharacterByID(m_Snap.m_LocalClientID);

	// update strong and weak hook
	if(pLocalChar && AntiPingPlayers())
	{
		if(m_Snap.m_aCharacters[m_Snap.m_LocalClientID].m_HasExtendedData)
		{
			int aIDs[MAX_CLIENTS];
			for(int i = 0; i < MAX_CLIENTS; i++)
				aIDs[i] = -1;
			for(int i = 0; i < MAX_CLIENTS; i++)
				if(CCharacter *pChar = m_GameWorld.GetCharacterByID(i))
					aIDs[pChar->GetStrongWeakID()] = i;
			for(int i = 0; i < MAX_CLIENTS; i++)
				if(aIDs[i] >= 0)
					m_CharOrder.GiveStrong(aIDs[i]);
		}
		else
		{
			// manual detection
			DetectStrongHook();
		}
		for(int i : m_CharOrder.m_IDs)
		{
			if(CCharacter *pChar = m_GameWorld.GetCharacterByID(i))
			{
				m_GameWorld.RemoveEntity(pChar);
				m_GameWorld.InsertEntity(pChar);
			}
		}
	}

	// advance the gameworld to the current gametick
	if(pLocalChar && abs(m_GameWorld.GameTick() - Client()->GameTick()) < SERVER_TICK_SPEED)
	{
		for(int Tick = m_GameWorld.GameTick() + 1; Tick <= Client()->GameTick(); Tick++)
		{
			CNetObj_PlayerInput *pInput = (CNetObj_PlayerInput*) Client()->GetDirectInput(Tick);
			if(pInput)
				pLocalChar->OnDirectInput(pInput);
			m_GameWorld.m_GameTick = Tick;
			if(pInput)
				pLocalChar->OnPredictedInput(pInput);
			m_GameWorld.Tick();

			for(int i = 0; i < MAX_CLIENTS; i++)
				if(CCharacter *pChar = m_GameWorld.GetCharacterByID(i))
				{
					m_aClients[i].m_PredPos[Tick % 200] = pChar->Core()->m_Pos;
					m_aClients[i].m_PredTick[Tick % 200] = Tick;
				}
		}
	}
	else
	{
		// skip to current gametick
		m_GameWorld.m_GameTick = Client()->GameTick();
		if(pLocalChar)
			if(CNetObj_PlayerInput *pInput = (CNetObj_PlayerInput*) Client()->GetInput(Client()->GameTick()))
				pLocalChar->SetInput(pInput);
	}

	for(int i = 0; i < MAX_CLIENTS; i++)
		if(CCharacter *pChar = m_GameWorld.GetCharacterByID(i))
		{
			m_aClients[i].m_PredPos[Client()->GameTick() % 200] = pChar->Core()->m_Pos;
			m_aClients[i].m_PredTick[Client()->GameTick() % 200] = Client()->GameTick();
		}

	// update the local gameworld with the new snapshot
	m_GameWorld.m_Teams = m_Teams;

	m_GameWorld.NetObjBegin();
	int Num = Client()->SnapNumItems(IClient::SNAP_CURRENT);
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(m_Snap.m_aCharacters[i].m_Active)
		{
			bool IsLocal = (i == m_Snap.m_LocalClientID);
			int GameTeam = (m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS) ? m_aClients[i].m_Team : i;
			m_GameWorld.NetCharAdd(i, &m_Snap.m_aCharacters[i].m_Cur,
					m_Snap.m_aCharacters[i].m_HasExtendedData ? &m_Snap.m_aCharacters[i].m_ExtendedData : 0,
					GameTeam, IsLocal);
		}
	for(int Index = 0; Index < Num; Index++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, Index, &Item);
		m_GameWorld.NetObjAdd(Item.m_ID, Item.m_Type, pData);
	}
	m_GameWorld.NetObjEnd(m_Snap.m_LocalClientID);

	// save the characters that are currently active
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(CCharacter *pChar = m_GameWorld.GetCharacterByID(i))
		{
			m_aLastWorldCharacters[i] = *pChar;
			m_aLastWorldCharacters[i].DetachFromGameWorld();
		}
}

void CGameClient::UpdateRenderedCharacters()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!m_Snap.m_aCharacters[i].m_Active)
			continue;
		m_aClients[i].m_RenderCur = m_Snap.m_aCharacters[i].m_Cur;
		m_aClients[i].m_RenderPrev = m_Snap.m_aCharacters[i].m_Prev;
		m_aClients[i].m_IsPredicted = false;
		m_aClients[i].m_IsPredictedLocal = false;
		vec2 UnpredPos = mix(
				vec2(m_Snap.m_aCharacters[i].m_Prev.m_X, m_Snap.m_aCharacters[i].m_Prev.m_Y),
				vec2(m_Snap.m_aCharacters[i].m_Cur.m_X, m_Snap.m_aCharacters[i].m_Cur.m_Y),
				Client()->IntraGameTick());
		vec2 Pos = UnpredPos;

		if(Predict() && (i == m_Snap.m_LocalClientID || AntiPingPlayers()))
		{
			m_aClients[i].m_Predicted.Write(&m_aClients[i].m_RenderCur);
			m_aClients[i].m_PrevPredicted.Write(&m_aClients[i].m_RenderPrev);

			m_aClients[i].m_IsPredicted = true;

			Pos = mix(
					vec2(m_aClients[i].m_RenderPrev.m_X, m_aClients[i].m_RenderPrev.m_Y),
					vec2(m_aClients[i].m_RenderCur.m_X, m_aClients[i].m_RenderCur.m_Y),
					m_aClients[i].m_IsPredicted ? Client()->PredIntraGameTick() : Client()->IntraGameTick());

			if(i == m_Snap.m_LocalClientID)
			{
				m_aClients[i].m_IsPredictedLocal = true;
				CCharacter *pChar = m_PredictedWorld.GetCharacterByID(i);
				if(pChar && AntiPingGunfire() && ((pChar->m_NinjaJetpack && pChar->m_FreezeTime == 0) || m_Snap.m_aCharacters[i].m_Cur.m_Weapon != WEAPON_NINJA || m_Snap.m_aCharacters[i].m_Cur.m_Weapon == m_aClients[i].m_Predicted.m_ActiveWeapon))
				{
					m_aClients[i].m_RenderCur.m_AttackTick = pChar->GetAttackTick();
					if(m_Snap.m_aCharacters[i].m_Cur.m_Weapon != WEAPON_NINJA && !(pChar->m_NinjaJetpack && pChar->Core()->m_ActiveWeapon == WEAPON_GUN))
						m_aClients[i].m_RenderCur.m_Weapon = m_aClients[i].m_Predicted.m_ActiveWeapon;
				}
			}
			else
			{
				// use unpredicted values for other players
				m_aClients[i].m_RenderPrev.m_Angle = m_Snap.m_aCharacters[i].m_Prev.m_Angle;
				m_aClients[i].m_RenderCur.m_Angle = m_Snap.m_aCharacters[i].m_Cur.m_Angle;

				if(g_Config.m_ClAntiPingSmooth)
					Pos = GetSmoothPos(i);
			}

		}
		m_Snap.m_aCharacters[i].m_Position = Pos;
		m_aClients[i].m_RenderPos = Pos;
		if(Predict() && i == m_Snap.m_LocalClientID)
			m_LocalCharacterPos = Pos;
	}
}

void CGameClient::DetectStrongHook()
{
	static int s_LastUpdateTick[MAX_CLIENTS] = {0};
	// attempt to detect strong/weak between players
	for(int FromPlayer = 0; FromPlayer < MAX_CLIENTS; FromPlayer++)
	{
		if(!m_Snap.m_aCharacters[FromPlayer].m_Active)
			continue;
		int ToPlayer = m_Snap.m_aCharacters[FromPlayer].m_Prev.m_HookedPlayer;
		if(ToPlayer < 0 || ToPlayer >= MAX_CLIENTS || !m_Snap.m_aCharacters[ToPlayer].m_Active || ToPlayer != m_Snap.m_aCharacters[FromPlayer].m_Cur.m_HookedPlayer)
			continue;
		if(abs(minimum(s_LastUpdateTick[ToPlayer], s_LastUpdateTick[FromPlayer]) - Client()->GameTick()) < SERVER_TICK_SPEED/4)
			continue;
		if(m_Snap.m_aCharacters[FromPlayer].m_Prev.m_Direction != m_Snap.m_aCharacters[FromPlayer].m_Cur.m_Direction
				|| m_Snap.m_aCharacters[ToPlayer].m_Prev.m_Direction != m_Snap.m_aCharacters[ToPlayer].m_Cur.m_Direction)
			continue;

		CCharacter *pFromCharWorld = m_GameWorld.GetCharacterByID(FromPlayer);
		CCharacter *pToCharWorld = m_GameWorld.GetCharacterByID(ToPlayer);
		if(!pFromCharWorld || !pToCharWorld)
			continue;

		s_LastUpdateTick[ToPlayer] = s_LastUpdateTick[FromPlayer] = Client()->GameTick();

		float PredictErr[2];
		CCharacterCore ToCharCur;
		ToCharCur.Read(&m_Snap.m_aCharacters[ToPlayer].m_Cur);

		CWorldCore World;
		World.m_Tuning[g_Config.m_ClDummy] = m_Tuning[g_Config.m_ClDummy];

		for(int dir = 0; dir < 2; dir++)
		{
			CCharacterCore ToChar = pFromCharWorld->GetCore();
			ToChar.Init(&World, Collision(), &m_Teams);
			World.m_apCharacters[ToPlayer] = &ToChar;
			ToChar.Read(&m_Snap.m_aCharacters[ToPlayer].m_Prev);

			CCharacterCore FromChar = pFromCharWorld->GetCore();
			FromChar.Init(&World, Collision(), &m_Teams);
			World.m_apCharacters[FromPlayer] = &FromChar;
			FromChar.Read(&m_Snap.m_aCharacters[FromPlayer].m_Prev);

			for(int Tick = Client()->PrevGameTick(); Tick < Client()->GameTick(); Tick++)
			{
				if(dir == 0)
				{
					FromChar.Tick(false);
					ToChar.Tick(false);
				}
				else
				{
					ToChar.Tick(false);
					FromChar.Tick(false);
				}
				FromChar.Move();
				FromChar.Quantize();
				ToChar.Move();
				ToChar.Quantize();
			}
			PredictErr[dir] = distance(ToChar.m_Vel, ToCharCur.m_Vel);
		}
		const float LOW = 0.0001f;
		const float HIGH = 0.07f;
		if(PredictErr[1] < LOW && PredictErr[0] > HIGH)
		{
			if(m_CharOrder.HasStrongAgainst(ToPlayer, FromPlayer))
			{
				if(ToPlayer != m_Snap.m_LocalClientID)
					m_CharOrder.GiveWeak(ToPlayer);
				else
					m_CharOrder.GiveStrong(FromPlayer);
			}
		}
		else if(PredictErr[0] < LOW && PredictErr[1] > HIGH)
		{
			if(m_CharOrder.HasStrongAgainst(FromPlayer, ToPlayer))
			{
				if(ToPlayer != m_Snap.m_LocalClientID)
					m_CharOrder.GiveStrong(ToPlayer);
				else
					m_CharOrder.GiveWeak(FromPlayer);
			}
		}
	}
}

vec2 CGameClient::GetSmoothPos(int ClientID)
{
	vec2 Pos = mix(m_aClients[ClientID].m_PrevPredicted.m_Pos, m_aClients[ClientID].m_Predicted.m_Pos, Client()->PredIntraGameTick());
	int64 Now = time_get();
	for(int i = 0; i < 2; i++)
	{
		int64 Len = clamp(m_aClients[ClientID].m_SmoothLen[i], (int64) 1, time_freq());
		int64 TimePassed = Now - m_aClients[ClientID].m_SmoothStart[i];
		if(in_range(TimePassed, (int64)0, Len-1))
		{
			float MixAmount = 1.f - powf(1.f - TimePassed/(float)Len, 1.2f);
			int SmoothTick;
			float SmoothIntra;
			Client()->GetSmoothTick(&SmoothTick, &SmoothIntra, MixAmount);
			if(SmoothTick > 0 && m_aClients[ClientID].m_PredTick[(SmoothTick-1) % 200] >= Client()->PrevGameTick() && m_aClients[ClientID].m_PredTick[SmoothTick % 200] <= Client()->PredGameTick())
				Pos[i] = mix(m_aClients[ClientID].m_PredPos[(SmoothTick-1) % 200][i], m_aClients[ClientID].m_PredPos[SmoothTick % 200][i], SmoothIntra);
		}
	}
	return Pos;
}

void CGameClient::Echo(const char *pString)
{
	m_pChat->Echo(pString);
}

bool CGameClient::IsOtherTeam(int ClientID)
{
	bool Local = m_Snap.m_LocalClientID == ClientID;

	if((m_aClients[m_Snap.m_LocalClientID].m_Team == TEAM_SPECTATORS && m_Snap.m_SpecInfo.m_SpectatorID == SPEC_FREEVIEW) || ClientID < 0)
		return false;
	else if(m_Snap.m_SpecInfo.m_Active && m_Snap.m_SpecInfo.m_SpectatorID != SPEC_FREEVIEW)
		return m_Teams.Team(ClientID) != m_Teams.Team(m_Snap.m_SpecInfo.m_SpectatorID);
	else if((m_aClients[m_Snap.m_LocalClientID].m_Solo || m_aClients[ClientID].m_Solo) && !Local)
		return true;

	return m_Teams.Team(ClientID) != m_Teams.Team(m_Snap.m_LocalClientID);
}
