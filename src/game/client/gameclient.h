/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_GAMECLIENT_H
#define GAME_CLIENT_GAMECLIENT_H

#include <base/color.h>
#include <base/vmath.h>
#include <engine/client.h>
#include <engine/console.h>
#include <game/layers.h>
#include <game/localization.h>
#include <game/gamecore.h>
#include "render.h"

#include <game/teamscore.h>

#include <game/client/prediction/gameworld.h>
#include <game/client/prediction/entities/character.h>
#include <game/client/prediction/entities/laser.h>
#include <game/client/prediction/entities/pickup.h>

class CGameClient;

class CWeaponData
{
public:
	int m_Tick;
	vec2 m_Pos;
	vec2 m_Direction;
	vec2 StartPos() { return m_Pos + m_Direction * 28.0f * 0.75f; }
};

class CLocalProjectile
{
public:
	int m_Active;
	CGameClient *m_pGameClient;
	CWorldCore *m_pWorld;
	CCollision *m_pCollision;

	vec2 m_Direction;
	vec2 m_Pos;
	int m_StartTick;
	int m_Type;

	int m_Owner;
	int m_Weapon;
	bool m_Explosive;
	int m_Bouncing;
	bool m_Freeze;
	bool m_ExtraInfo;

	vec2 GetPos(float Time);
	void CreateExplosion(vec2 Pos, int LocalClientID);
	void Tick(int CurrentTick, int GameTickSpeed, int LocalClientID);
	void Init(CGameClient *pGameClient, CWorldCore *pWorld, CCollision *pCollision, const CNetObj_Projectile *pProj);
	void Init(CGameClient *pGameClient, CWorldCore *pWorld, CCollision *pCollision, vec2 Vel, vec2 Pos, int StartTick, int Type, int Owner, int Weapon, bool Explosive, int Bouncing, bool Freeze, bool ExtraInfo);
	bool GameLayerClipped(vec2 CheckPos);
	void Deactivate() { m_Active = 0; }
};

class CGameInfo
{
public:
	bool m_FlagStartsRace;
	bool m_TimeScore;
	bool m_UnlimitedAmmo;
	bool m_DDRaceRecordMessage;
	bool m_RaceRecordMessage;

	bool m_AllowEyeWheel;
	bool m_AllowHookColl;
	bool m_AllowZoom;

	bool m_BugDDRaceGhost;
	bool m_BugDDRaceInput;
	bool m_BugFNGLaserRange;
	bool m_BugVanillaBounce;

	bool m_PredictFNG;
	bool m_PredictDDRace;
	bool m_PredictDDRaceTiles;
	bool m_PredictVanilla;

	bool m_EntitiesDDNet;
	bool m_EntitiesDDRace;
	bool m_EntitiesRace;
	bool m_EntitiesFNG;
	bool m_EntitiesVanilla;

	bool m_Race;

	bool m_DontMaskEntities;
};

class CGameClient : public IGameClient
{
	class CStack
	{
	public:
		enum
		{
			MAX_COMPONENTS = 64,
		};

		CStack();
		void Add(class CComponent *pComponent);

		class CComponent *m_paComponents[MAX_COMPONENTS];
		int m_Num;
	};

	CStack m_All;
	CStack m_Input;
	CNetObjHandler m_NetObjHandler;

	class IEngine *m_pEngine;
	class IInput *m_pInput;
	class IGraphics *m_pGraphics;
	class ITextRender *m_pTextRender;
	class IClient *m_pClient;
	class ISound *m_pSound;
	class IConsole *m_pConsole;
	class IStorage *m_pStorage;
	class IDemoPlayer *m_pDemoPlayer;
	class IServerBrowser *m_pServerBrowser;
	class IEditor *m_pEditor;
	class IFriends *m_pFriends;
	class IFriends *m_pFoes;
	class IUpdater *m_pUpdater;

	CLayers m_Layers;
	class CCollision m_Collision;
	CUI m_UI;

	void ProcessEvents();
	void UpdatePositions();

	int m_PredictedTick;
	int m_LastNewPredictedTick[2];

	int m_LastRoundStartTick;

	int m_LastFlagCarrierRed;
	int m_LastFlagCarrierBlue;

	int m_CheckInfo[2];

	static void ConTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConKill(IConsole::IResult *pResult, void *pUserData);

	static void ConchainSpecialInfoupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainSpecialDummyInfoupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainSpecialDummy(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainClTextEntitiesSize(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

public:
	IKernel *Kernel() { return IInterface::Kernel(); }
	IEngine *Engine() const { return m_pEngine; }
	class IGraphics *Graphics() const { return m_pGraphics; }
	class IClient *Client() const { return m_pClient; }
	class CUI *UI() { return &m_UI; }
	class ISound *Sound() const { return m_pSound; }
	class IInput *Input() const { return m_pInput; }
	class IStorage *Storage() const { return m_pStorage; }
	class IConsole *Console() { return m_pConsole; }
	class ITextRender *TextRender() const { return m_pTextRender; }
	class IDemoPlayer *DemoPlayer() const { return m_pDemoPlayer; }
	class IDemoRecorder *DemoRecorder(int Recorder) const { return Client()->DemoRecorder(Recorder); }
	class IServerBrowser *ServerBrowser() const { return m_pServerBrowser; }
	class CRenderTools *RenderTools() { return &m_RenderTools; }
	class CLayers *Layers() { return &m_Layers; };
	class CCollision *Collision() { return &m_Collision; };
	class IEditor *Editor() { return m_pEditor; }
	class IFriends *Friends() { return m_pFriends; }
	class IFriends *Foes() { return m_pFoes; }
	class IUpdater *Updater() { return m_pUpdater; }

	int NetobjNumCorrections() { return m_NetObjHandler.NumObjCorrections(); }
	const char *NetobjCorrectedOn() { return m_NetObjHandler.CorrectedObjOn(); }

	bool m_SuppressEvents;
	bool m_NewTick;
	bool m_NewPredictedTick;
	int m_FlagDropTick[2];

	// TODO: move this
	CTuningParams m_Tuning[2];

	enum
	{
		SERVERMODE_PURE=0,
		SERVERMODE_MOD,
		SERVERMODE_PUREMOD,
	};
	int m_ServerMode;
	CGameInfo m_GameInfo;

	int m_DemoSpecID;

	vec2 m_LocalCharacterPos;

	// predicted players
	CCharacterCore m_PredictedPrevChar;
	CCharacterCore m_PredictedChar;

	// snap pointers
	struct CSnapState
	{
		const CNetObj_Character *m_pLocalCharacter;
		const CNetObj_Character *m_pLocalPrevCharacter;
		const CNetObj_PlayerInfo *m_pLocalInfo;
		const CNetObj_SpectatorInfo *m_pSpectatorInfo;
		const CNetObj_SpectatorInfo *m_pPrevSpectatorInfo;
		const CNetObj_Flag *m_paFlags[2];
		const CNetObj_GameInfo *m_pGameInfoObj;
		const CNetObj_GameData *m_pGameDataObj;
		int m_GameDataSnapID;

		const CNetObj_PlayerInfo *m_paPlayerInfos[MAX_CLIENTS];
		const CNetObj_PlayerInfo *m_paInfoByScore[MAX_CLIENTS];
		const CNetObj_PlayerInfo *m_paInfoByName[MAX_CLIENTS];
		//const CNetObj_PlayerInfo *m_paInfoByTeam[MAX_CLIENTS];
		const CNetObj_PlayerInfo *m_paInfoByDDTeam[MAX_CLIENTS];

		int m_LocalClientID;
		int m_NumPlayers;
		int m_aTeamSize[2];

		// spectate data
		struct CSpectateInfo
		{
			bool m_Active;
			int m_SpectatorID;
			bool m_UsePosition;
			vec2 m_Position;
		} m_SpecInfo;

		//
		struct CCharacterInfo
		{
			bool m_Active;

			// snapshots
			CNetObj_Character m_Prev;
			CNetObj_Character m_Cur;

			CNetObj_DDNetCharacter m_ExtendedData;
			bool m_HasExtendedData;

			// interpolated position
			vec2 m_Position;
		};

		CCharacterInfo m_aCharacters[MAX_CLIENTS];
	};

	CSnapState m_Snap;

	// client data
	struct CClientData
	{
		int m_UseCustomColor;
		int m_ColorBody;
		int m_ColorFeet;

		char m_aName[MAX_NAME_LENGTH];
		char m_aClan[MAX_CLAN_LENGTH];
		int m_Country;
		char m_aSkinName[64];
		int m_SkinID;
		int m_SkinColor;
		int m_Team;
		int m_Emoticon;
		int m_EmoticonStart;
		bool m_Solo;
		bool m_Jetpack;
		bool m_NoCollision;
		bool m_EndlessHook;
		bool m_EndlessJump;
		bool m_NoHammerHit;
		bool m_NoGrenadeHit;
		bool m_NoRifleHit;
		bool m_NoShotgunHit;
		bool m_NoHookHit;
		bool m_Super;
		bool m_HasTelegunGun;
		bool m_HasTelegunGrenade;
		bool m_HasTelegunLaser;
		int m_FreezeEnd;
		bool m_DeepFrozen;


		CCharacterCore m_Predicted;
		CCharacterCore m_PrevPredicted;

		CTeeRenderInfo m_SkinInfo; // this is what the server reports
		CTeeRenderInfo m_RenderInfo; // this is what we use

		float m_Angle;
		bool m_Active;
		bool m_ChatIgnore;
		bool m_Friend;
		bool m_Foe;

		int m_AuthLevel;
		bool m_Afk;

		void UpdateRenderInfo();
		void Reset();

		// DDRace

		int m_Score;

		// rendered characters
		CNetObj_Character m_RenderCur;
		CNetObj_Character m_RenderPrev;
		vec2 m_RenderPos;
		bool m_IsPredicted;
		bool m_IsPredictedLocal;
		int64 m_SmoothStart[2];
		int64 m_SmoothLen[2];
		vec2 m_PredPos[200];
		int m_PredTick[200];
	};

	CClientData m_aClients[MAX_CLIENTS];

	class CClientStats
	{
		int m_IngameTicks;
		int m_JoinTick;
		bool m_Active;

	public:
		CClientStats();

		int m_aFragsWith[NUM_WEAPONS];
		int m_aDeathsFrom[NUM_WEAPONS];
		int m_Frags;
		int m_Deaths;
		int m_Suicides;
		int m_BestSpree;
		int m_CurrentSpree;

		int m_FlagGrabs;
		int m_FlagCaptures;

		void Reset();

		bool IsActive() const { return m_Active; }
		void JoinGame(int Tick) { m_Active = true; m_JoinTick = Tick; };
		void JoinSpec(int Tick) { m_Active = false; m_IngameTicks += Tick - m_JoinTick; };
		int GetIngameTicks(int Tick) const { return m_IngameTicks + Tick - m_JoinTick; };
		float GetFPM(int Tick, int TickSpeed) const { return (float)(m_Frags * TickSpeed * 60) / GetIngameTicks(Tick); };
	};

	CClientStats m_aStats[MAX_CLIENTS];

	CRenderTools m_RenderTools;

	void OnReset();

	// hooks
	virtual void OnConnected();
	virtual void OnRender();
	virtual void OnUpdate();
	virtual void OnDummyDisconnect();
	virtual void OnRelease();
	virtual void OnInit();
	virtual void OnConsoleInit();
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnMessage(int MsgId, CUnpacker *pUnpacker, bool IsDummy = 0);
	virtual void OnNewSnapshot();
	virtual void OnPredict();
	virtual void OnActivateEditor();
	virtual void OnDummySwap();
	virtual int OnSnapInput(int *pData, bool Dummy, bool Force);
	virtual void OnShutdown();
	virtual void OnEnterGame();
	virtual void OnRconType(bool UsernameReq);
	virtual void OnRconLine(const char *pLine);
	virtual void OnGameOver();
	virtual void OnStartGame();
	virtual void OnFlagGrab(int TeamID);

	void OnWindowResize();
	static void OnWindowResizeCB(void *pUser);

	virtual const char *GetItemName(int Type);
	virtual const char *Version();
	virtual const char *NetVersion();

	// actions
	// TODO: move these
	void SendSwitchTeam(int Team);
	void SendInfo(bool Start);
	virtual void SendDummyInfo(bool Start);
	void SendKill(int ClientID);

	// pointers to all systems
	class CGameConsole *m_pGameConsole;
	class CBinds *m_pBinds;
	class CParticles *m_pParticles;
	class CMenus *m_pMenus;
	class CSkins *m_pSkins;
	class CCountryFlags *m_pCountryFlags;
	class CFlow *m_pFlow;
	class CChat *m_pChat;
	class CDamageInd *m_pDamageind;
	class CCamera *m_pCamera;
	class CControls *m_pControls;
	class CEffects *m_pEffects;
	class CSounds *m_pSounds;
	class CMotd *m_pMotd;
	class CMapImages *m_pMapimages;
	class CVoting *m_pVoting;
	class CScoreboard *m_pScoreboard;
	class CStatboard *m_pStatboard;
	class CItems *m_pItems;
	class CMapLayers *m_pMapLayersBackGround;
	class CMapLayers *m_pMapLayersForeGround;
	class CBackground *m_pBackGround;

	class CMapSounds *m_pMapSounds;
	class CPlayers *m_pPlayers;

	// DDRace

	int m_LocalIDs[2];
	CNetObj_PlayerInput m_DummyInput;
	CNetObj_PlayerInput m_HammerInput;
	int m_DummyFire;

	class CRaceDemo *m_pRaceDemo;
	class CGhost *m_pGhost;
	class CTeamsCore m_Teams;

	int IntersectCharacter(vec2 Pos0, vec2 Pos1, vec2& NewPos, int ownID);

	virtual int GetLastRaceTick();

	bool AntiPingPlayers() { return g_Config.m_ClAntiPing && g_Config.m_ClAntiPingPlayers && !m_Snap.m_SpecInfo.m_Active && Client()->State() != IClient::STATE_DEMOPLAYBACK && (m_Tuning[g_Config.m_ClDummy].m_PlayerCollision || m_Tuning[g_Config.m_ClDummy].m_PlayerHooking); }
	bool AntiPingGrenade() { return g_Config.m_ClAntiPing && g_Config.m_ClAntiPingGrenade && !m_Snap.m_SpecInfo.m_Active && Client()->State() != IClient::STATE_DEMOPLAYBACK; }
	bool AntiPingWeapons() { return g_Config.m_ClAntiPing && g_Config.m_ClAntiPingWeapons && !m_Snap.m_SpecInfo.m_Active && Client()->State() != IClient::STATE_DEMOPLAYBACK; }
	bool AntiPingGunfire() { return AntiPingGrenade() && AntiPingWeapons() && g_Config.m_ClAntiPingGunfire; }
	bool Predict() { return g_Config.m_ClPredict && !(m_Snap.m_pGameInfoObj && m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER) && !m_Snap.m_SpecInfo.m_Active && Client()->State() != IClient::STATE_DEMOPLAYBACK && m_Snap.m_pLocalCharacter; }

	CGameWorld m_GameWorld;
	CGameWorld m_PredictedWorld;
	CGameWorld m_PrevPredictedWorld;

	void Echo(const char *pString);
	bool IsOtherTeam(int ClientID);

private:
	bool m_DDRaceMsgSent[2];
	int m_ShowOthers[2];

	void UpdatePrediction();
	void UpdateRenderedCharacters();
	void DetectStrongHook();
	vec2 GetSmoothPos(int ClientID);

	CCharOrder m_CharOrder;
	class CCharacter m_aLastWorldCharacters[MAX_CLIENTS];
};

ColorRGBA CalculateNameColor(ColorHSLA TextColorHSL);

#endif
