/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#ifndef GAME_SERVER_ENTITIES_PLASMA_H
#define GAME_SERVER_ENTITIES_PLASMA_H

#include <game/server/entity.h>

class CGun;

class CPlasma: public CEntity
{
	vec2 m_Core;
	int m_EvalTick;
	int m_LifeTime;

	int m_ResponsibleTeam;
	int m_Freeze;
	int m_Owner;
	float m_Accel;

	bool m_Explosive;
	bool HitCharacter();
	void Move();
public:

	CPlasma(CGameWorld *pGameWorld, vec2 Pos, vec2 Dir, bool Freeze,
			bool Explosive, int ResponsibleTeam, int Owner = -1, float Accel = 1.1f, float Lifetime = 1.5f);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
};

#endif // GAME_SERVER_ENTITIES_PLASMA_H
