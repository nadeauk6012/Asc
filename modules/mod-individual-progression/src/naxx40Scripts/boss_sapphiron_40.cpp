#include "Cell.h"
#include "CellImpl.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "naxxramas.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "bot_ai.h"

enum Yells
{
    EMOTE_AIR_PHASE         = 0,
    EMOTE_GROUND_PHASE      = 1,
    EMOTE_BREATH            = 2,
    EMOTE_ENRAGE            = 3
};

enum Spells
{
    // Fight
    SPELL_FROST_AURA                = 828531,
    SPELL_CLEAVE                    = 19983,
    SPELL_TAIL_SWEEP                = 15847,
    SPELL_SUMMON_BLIZZARD           = 28560,
    SPELL_LIFE_DRAIN                = 828542,
    SPELL_BERSERK                   = 26662,

    // Ice block
    SPELL_ICEBOLT_CAST              = 28526,
    SPELL_ICEBOLT_TRIGGER           = 828522,
    SPELL_FROST_MISSILE             = 30101,
    SPELL_FROST_EXPLOSION           = 28524,

    // Visuals
    SPELL_SAPPHIRON_DIES            = 29357,

    // 10 and 25 Man Spells
    SPELL_FROST_AURA_10             = 828531,
    SPELL_FROST_AURA_25             = 828531,
    SPELL_TAIL_SWEEP_10             = 55697,
    SPELL_TAIL_SWEEP_25             = 55696,
    SPELL_LIFE_DRAIN_10             = 828542,
    SPELL_LIFE_DRAIN_25             = 828542,

    // Healthstone for bots
    SPELL_MAJOR_HEALTHSTONE         = 23477,
};

enum Misc
{
    GO_ICE_BLOCK                    = 181247,
    NPC_BLIZZARD                    = 816474,

    POINT_CENTER                    = 1
};

enum Events
{
    EVENT_BERSERK                   = 1,
    EVENT_CLEAVE                    = 2,
    EVENT_TAIL_SWEEP                = 3,
    EVENT_LIFE_DRAIN                = 4,
    EVENT_BLIZZARD                  = 5,
    EVENT_FLIGHT_START              = 6,
    EVENT_FLIGHT_LIFTOFF            = 7,
    EVENT_FLIGHT_ICEBOLT            = 8,
    EVENT_FLIGHT_BREATH             = 9,
    EVENT_FLIGHT_SPELL_EXPLOSION    = 10,
    EVENT_FLIGHT_START_LAND         = 11,
    EVENT_LAND                      = 12,
    EVENT_GROUND                    = 13,
    EVENT_HUNDRED_CLUB              = 14,
    EVENT_MOVE_BEHIND_ICEBLOCKS     = 15

};

enum Positions
{
    POSITION_1,
    POSITION_2,
    POSITION_3,
    POSITION_4,
    POSITION_5,
    POSITION_6,
    POSITION_7,
    POSITION_8,
    POSITION_9,
    POSITION_10,
    POSITION_11,
    POSITION_12,
    POSITION_13,
    POSITION_14,
    POSITION_15,
    POSITION_16,
    POSITION_17,
    POSITION_18,
    POSITION_19,
    POSITION_20,
    POSITION_21,
    POSITION_22,
    POSITION_23,
    POSITION_24,
    MAX_POSITIONS
};

const Position predefinedPositions[MAX_POSITIONS] =
{
    {3501.8896484375f, -5285.8276367188f, 138.11660766602f, -0.97883719205856f},
    {3517.44921875f, -5287.248046875f, 138.11184692383f, 0.34032544493675f},
    {3531.1569824219f, -5286.6069335938f, 138.11717224121f, 2.4977161884308f},
    {3549.0439453125f, -5279.4028320312f, 137.80981445312f, 1.7680470943451f},
    {3553.9663085938f, -5290.6049804688f, 137.6363067627f, 0.43171513080597f},
    {3537.322265625f, -5296.2470703125f, 138.0382232666f, 0.32682889699936f},
    {3526.33203125f, -5300.8295898438f, 138.12173461914f, -2.0810854434967f},
    {3514.6770019531f, -5299.4975585938f, 138.1116027832f, 0.034198552370071f},
    {3498.83203125f, -5297.5244140625f, 138.11734008789f, 6.1592950820923f},
    {3486.9572753906f, -5289.1264648438f, 138.12460327148f, 2.6658415794373f},
    {3472.8562011719f, -5277.291015625f, 137.72737121582f, 5.9972171783447f},
    {3545.0686035156f, -5250.6962890625f, 137.59770202637f, 0.81440633535385f},
    {3532.7692871094f, -5253.73828125f, 137.30999755859f, 3.2648317813873f},
    {3521.8781738281f, -5257.1787109375f, 137.29754638672f, 1.3434364795685f},
    {3509.6098632812f, -5255.19140625f, 137.29754638672f, 4.0216503143311f},
    {3495.3903808594f, -5248.5595703125f, 137.53215026855f, 5.9685912132263f},
    {3543.9462890625f, -5269.5532226562f, 137.99829101562f, 5.1899771690369f},
    {3530.7158203125f, -5275.26953125f, 138.11015319824f, -0.16509318351746f},
    {3520.2028808594f, -5274.6108398438f, 138.10920715332f, 0.45487180352211f},
    {3507.2653808594f, -5272.9555664062f, 138.11065673828f, 0.31121501326561f},
    {3494.78125f, -5270.369140625f, 138.11225891113f, -0.57816123962402f},
    {3487.4572753906f, -5260.716796875f, 137.99456787109f, 0.65698927640915f},
    {3477.4174804688f, -5267.8798828125f, 137.78506469727f, 0.6028870344162f},
    {3490.2082519531f, -5278.6635742188f, 138.11825561523f, 2.3427762985229f}
};

// Unlike other Naxx 40 scripts, this overwrites all versions of the UI
// This is due to AI casting used in the spell script

class boss_sapphiron_40 : public CreatureScript
{
private:
    static bool isNaxx40Sapp(uint32 entry)
    {
        return (entry == NPC_SAPPHIRON_40);
    }
public:
    boss_sapphiron_40() : CreatureScript("boss_sapphiron_40") { }

    CreatureAI* GetAI(Creature* pCreature) const override
    {
        return GetNaxxramasAI<boss_sapphiron_40AI>(pCreature);
    }

    struct boss_sapphiron_40AI : public BossAI
    {
        explicit boss_sapphiron_40AI(Creature* c) : BossAI(c, BOSS_SAPPHIRON)
        {
            pInstance = me->GetInstanceScript();
        }
        bool inFlight = false;
        EventMap events;
        InstanceScript* pInstance;
        uint8 iceboltCount{};
        uint32 spawnTimer{};
        GuidList blockList;
        ObjectGuid currentTarget;

        void InitializeAI() override
        {
            me->SummonGameObject(GO_SAPPHIRON_BIRTH, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, 0, 0, 0, 0, 0);
            me->SetVisible(false);
            me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            ScriptedAI::InitializeAI();
        }

        bool IsInRoom()

        {
            if (me->GetExactDist(3523.5f, -5235.3f, 137.6f) > 100.0f)
            {
                EnterEvadeMode();
                return false;
            }
            return true;
        }

        void Reset() override
        {
            BossAI::Reset();
            if (me->IsVisible())
            {
                me->SetReactState(REACT_AGGRESSIVE);
            }
            events.Reset();
            iceboltCount = 0;
            spawnTimer = 0;
            currentTarget.Clear();
            blockList.clear();
            inFlight = false;
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC);
        }

        void EnterCombatSelfFunction()
        {
            Map::PlayerList const& PlList = me->GetMap()->GetPlayers();
            if (PlList.IsEmpty())
                return;

            for (const auto& i : PlList)
            {
                if (Player* player = i.GetSource())
                {
                    if (player->IsGameMaster())
                        continue;

                    if (player->IsAlive() && me->GetDistance(player) < 80.0f)
                    {
                        me->SetInCombatWith(player);
                        player->SetInCombatWith(me);
                        me->AddThreat(player, 0.0f);
                    }
                }
            }
        }

        void JustEngagedWith(Unit* who) override
        {
            BossAI::JustEngagedWith(who);
            EnterCombatSelfFunction();
            if (isNaxx40Sapp(me->GetEntry()))
            {
                me->CastSpell(me, SPELL_FROST_AURA, true);
            }
            else
            {
                me->CastSpell(me, RAID_MODE(SPELL_FROST_AURA_10, SPELL_FROST_AURA_25), true);
            }
            events.ScheduleEvent(EVENT_BERSERK, 360000);
            events.ScheduleEvent(EVENT_CLEAVE, 5000);
            events.ScheduleEvent(EVENT_TAIL_SWEEP, 10000);
            events.ScheduleEvent(EVENT_LIFE_DRAIN, 17000);
            events.ScheduleEvent(EVENT_BLIZZARD, 12000);
            events.ScheduleEvent(EVENT_FLIGHT_START, 45000);
            events.ScheduleEvent(EVENT_HUNDRED_CLUB, 5000);
        }

        void JustDied(Unit* killer) override
        {
            BossAI::JustDied(killer);
            me->CastSpell(me, SPELL_SAPPHIRON_DIES, true);
            DoCastSelf(875167, true);
            Map::PlayerList const& players = me->GetMap()->GetPlayers();
            for (auto const& playerPair : players)
            {
                Player* player = playerPair.GetSource();
                if (player)
                {
                    DistributeChallengeRewards(player, me, 1, false);
                }
            }
        }

        void DoAction(int32 param) override
        {
            if (param == ACTION_SAPPHIRON_BIRTH)
            {
                spawnTimer = 1;
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == POINT_MOTION_TYPE && id == POINT_CENTER)
            {
                events.ScheduleEvent(EVENT_FLIGHT_LIFTOFF, 500);
            }
        }

        void SpellHitTarget(Unit* target, SpellInfo const* spellInfo) override
        {
            if (spellInfo->Id == SPELL_ICEBOLT_CAST)
            {
                me->CastSpell(target, SPELL_ICEBOLT_TRIGGER, true);
            }
        }

        bool IsValidExplosionTarget(WorldObject* target)
        {
            for (ObjectGuid const& guid : blockList)
            {
                if (target->GetGUID() == guid)
                    return false;

                if (Unit* block = ObjectAccessor::GetUnit(*me, guid))
                {
                    if (block->IsInBetween(me, target, 2.0f) && block->IsWithinDist(target, 10.0f))
                        return false;
                }
            }
            return true;
        }

        void KilledUnit(Unit* who) override
        {
            if (who->GetTypeId() == TYPEID_PLAYER && pInstance)
            {
                pInstance->SetData(DATA_IMMORTAL_FAIL, 0);
            }
        }

        Unit* SelectRandomPlayerOrNPCBot(float range)
        {
            std::list<Unit*> targets;
            Acore::AnyUnitInObjectRangeCheck check(me, range);
            Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(me, targets, check);
            Cell::VisitAllObjects(me, searcher, range);

            targets.remove_if([this](Unit* unit) -> bool {
                // Skip if unit is not alive, is the current victim (tank), or is not a player or NPCBot
                return !unit->IsAlive() || unit == me->GetVictim() || !(unit->GetTypeId() == TYPEID_PLAYER || (unit->GetTypeId() == TYPEID_UNIT && static_cast<Creature*>(unit)->IsNPCBot()));
                });

            if (targets.empty())
                return nullptr;

            return Acore::Containers::SelectRandomContainerElement(targets);
        }

        void MoveBotsToPositionsAndCast()
        {
            std::list<Unit*> targetList;
            Acore::AnyUnitInObjectRangeCheck checker(me, 150.0f);
            Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(me, targetList, checker);
            Cell::VisitAllObjects(me, searcher, 150.0f);

            targetList.remove_if([](Unit* unit) {
                return !unit->IsNPCBot();
                });

            if (targetList.empty())
                return;

            uint8 posIndex = 0;
            for (Unit* unit : targetList)
            {
                if (posIndex >= MAX_POSITIONS)
                    break;
                
                if (unit->IsNonMeleeSpellCast(false, false, false, false, false))
                    unit->InterruptNonMeleeSpells(false, 0, true, false);
                unit->ToCreature()->GetBotAI()->MoveToSendPosition(predefinedPositions[posIndex]);
                unit->CastSpell(unit, SPELL_MAJOR_HEALTHSTONE, true);
                posIndex++;
            }
        }

        void MoveBotsBehindIceBlocks()
        {
            std::list<Unit*> targetList;
            Acore::AnyUnitInObjectRangeCheck checker(me, 150.0f);
            Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(me, targetList, checker);
            Cell::VisitAllObjects(me, searcher, 150.0f);

            targetList.remove_if([](Unit* unit) {
                return !unit->IsNPCBot();
                });

            if (targetList.empty())
                return;

            std::list<GameObject*> iceBlocks;
            Acore::GameObjectInRangeCheck goChecker(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 150.0f, GO_ICE_BLOCK);
            Acore::GameObjectListSearcher<Acore::GameObjectInRangeCheck> goSearcher(me, iceBlocks, goChecker);
            Cell::VisitAllObjects(me->GetPositionX(), me->GetPositionY(), me->GetMap(), goSearcher, 150.0f);

            if (iceBlocks.empty())
                return;

            for (Unit* unit : targetList)
            {
                GameObject* nearestIceBlock = nullptr;
                float minDist = std::numeric_limits<float>::max();

                for (GameObject* iceBlock : iceBlocks)
                {
                    float dist = unit->GetDistance(iceBlock);
                    if (dist < minDist)
                    {
                        minDist = dist;
                        nearestIceBlock = iceBlock;
                    }
                }

                if (nearestIceBlock)
                {
                    float ix, iy, iz;
                    nearestIceBlock->GetPosition(ix, iy, iz);

                    // Calculate the position directly behind the ice block relative to the boss
                    float bx, by, bz;
                    me->GetPosition(bx, by, bz);

                    float dx = ix - bx;
                    float dy = iy - by;
                    float distance = sqrt(dx * dx + dy * dy);

                    float behindX = ix + (dx / distance) * unit->GetObjectSize();
                    float behindY = iy + (dy / distance) * unit->GetObjectSize();

                    Position pos;
                    pos.m_positionX = behindX;
                    pos.m_positionY = behindY;
                    pos.m_positionZ = iz;
                    pos.m_orientation = nearestIceBlock->GetOrientation();

                    // Cancel current cast if any
                    if (unit->IsNonMeleeSpellCast(false, false, false, false, false))
                        unit->InterruptNonMeleeSpells(false, 0, true, false);

                    // Move to the calculated position using MoveToSendPosition
                    unit->ToCreature()->GetBotAI()->MoveToSendPosition(pos);
                }
            }
        }

        void SetBotsToFollow()
        {
            std::list<Unit*> targetList;
            Acore::AnyUnitInObjectRangeCheck checker(me, 150.0f);
            Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(me, targetList, checker);
            Cell::VisitAllObjects(me, searcher, 150.0f);

            targetList.remove_if([](Unit* unit) {
                return !unit->IsNPCBot();
                });

            if (targetList.empty())
                return;

            for (Unit* unit : targetList)
            {
                if (unit->ToCreature()->IsNPCBot())
                {
                    unit->ToCreature()->GetBotAI()->SetBotCommandState(BOT_COMMAND_FOLLOW, true);
                }
            }
        }
        
        void UpdateAI(uint32 diff) override
        {
            if (spawnTimer)
            {
                spawnTimer += diff;
                if (spawnTimer >= 21500)
                {
                    me->SetVisible(true);
                    me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
                    me->SetReactState(REACT_AGGRESSIVE);
                    spawnTimer = 0;
                }
                return;
            }

            if (!IsInRoom())
                return;

            if (!UpdateVictim())
                return;

            events.Update(diff);
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (events.ExecuteEvent())
            {
            case EVENT_BERSERK:
                Talk(EMOTE_ENRAGE);
                me->CastSpell(me, SPELL_BERSERK, true);
                return;
            case EVENT_CLEAVE:
                me->CastSpell(me->GetVictim(), SPELL_CLEAVE, false);
                events.RepeatEvent(10000);
                return;
            case EVENT_TAIL_SWEEP:
                if (isNaxx40Sapp(me->GetEntry()))
                {
                    me->CastSpell(me, SPELL_TAIL_SWEEP, false);
                }
                else
                {
                    me->CastSpell(me, RAID_MODE(SPELL_TAIL_SWEEP_10, SPELL_TAIL_SWEEP_25), false);
                }
                events.RepeatEvent(10000);
                return;
            case EVENT_LIFE_DRAIN:
                if (isNaxx40Sapp(me->GetEntry()))
                {
                    me->CastCustomSpell(SPELL_LIFE_DRAIN, SPELLVALUE_MAX_TARGETS, 5, me, false);
                }
                else
                {
                    me->CastCustomSpell(RAID_MODE(SPELL_LIFE_DRAIN_10, SPELL_LIFE_DRAIN_25), SPELLVALUE_MAX_TARGETS, RAID_MODE(2, 5), me, false);
                }
                events.RepeatEvent(24000);
                return;
            case EVENT_BLIZZARD:
            {
                Creature* cr;
                if (Unit* target = SelectRandomPlayerOrNPCBot(40.0f))
                {
                    cr = me->SummonCreature(NPC_BLIZZARD, *target, TEMPSUMMON_TIMED_DESPAWN, 16000);
                }
                else
                {
                    cr = me->SummonCreature(NPC_BLIZZARD, *me, TEMPSUMMON_TIMED_DESPAWN, 16000);
                }
                if (cr)
                {
                    cr->GetMotionMaster()->MoveRandom(40);
                }
                if (isNaxx40Sapp(me->GetEntry()))
                {
                    events.RepeatEvent(6500);
                }
                else
                {
                    events.RepeatEvent(RAID_MODE(8000, 6500));
                }
                return;
            }
            case EVENT_FLIGHT_START:
                if (me->HealthBelowPct(11))
                {
                    return;
                }
                inFlight = true;
                events.RepeatEvent(45000);
                events.DelayEvents(35000);
                me->SetReactState(REACT_PASSIVE);
                me->AttackStop();
                float x, y, z, o;
                me->GetHomePosition(x, y, z, o);
                me->GetMotionMaster()->MovePoint(POINT_CENTER, x, y, z);
                return;
            case EVENT_FLIGHT_LIFTOFF:
                Talk(EMOTE_AIR_PHASE);
                MoveBotsToPositionsAndCast();
                me->GetMotionMaster()->MoveIdle();
                me->SendMeleeAttackStop(me->GetVictim());
                me->HandleEmoteCommand(EMOTE_ONESHOT_LIFTOFF);
                me->SetDisableGravity(true);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC);
                currentTarget.Clear();
                events.ScheduleEvent(EVENT_FLIGHT_ICEBOLT, 8000);
                if (isNaxx40Sapp(me->GetEntry()))
                {
                    iceboltCount = 3;
                }
                else
                {
                    iceboltCount = RAID_MODE(2, 3);
                }
                return;
            case EVENT_FLIGHT_ICEBOLT:
            {
                if (currentTarget)
                {
                    if (Unit* target = ObjectAccessor::GetUnit(*me, currentTarget))
                    {
                        me->SummonGameObject(GO_ICE_BLOCK, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), target->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 0);
                    }
                }

                std::vector<Unit*> targets;
                auto i = me->GetThreatMgr().GetThreatList().begin();
                for (; i != me->GetThreatMgr().GetThreatList().end(); ++i)
                {
                    if ((*i)->getTarget()->GetTypeId() == TYPEID_PLAYER || ((*i)->getTarget()->GetTypeId() == TYPEID_UNIT && static_cast<Creature*>((*i)->getTarget())->IsNPCBot()))
                    {
                        bool inList = false;
                        if (!blockList.empty())
                        {
                            for (GuidList::const_iterator itr = blockList.begin(); itr != blockList.end(); ++itr)
                            {
                                if ((*i)->getTarget()->GetGUID() == *itr)
                                {
                                    inList = true;
                                    break;
                                }
                            }
                        }
                        if (!inList)
                        {
                            targets.push_back((*i)->getTarget());
                        }
                    }
                }

                if (!targets.empty() && iceboltCount)
                {
                    auto itr = targets.begin();
                    advance(itr, urand(0, targets.size() - 1));
                    me->CastSpell(*itr, SPELL_ICEBOLT_TRIGGER, true);  // stun the player or NPCBot immediately
                    me->CastSpell(*itr, SPELL_ICEBOLT_CAST, false);
                    blockList.push_back((*itr)->GetGUID());
                    currentTarget = (*itr)->GetGUID();
                    --iceboltCount;
                    events.ScheduleEvent(EVENT_FLIGHT_ICEBOLT, (me->GetExactDist(*itr) / 13.0f) * IN_MILLISECONDS);
                }
                else
                {
                    events.ScheduleEvent(EVENT_FLIGHT_BREATH, 1000);
                    events.ScheduleEvent(EVENT_MOVE_BEHIND_ICEBLOCKS, 500);
                }
                return;
            }
            case EVENT_MOVE_BEHIND_ICEBLOCKS:
                MoveBotsBehindIceBlocks();
                return;
            case EVENT_FLIGHT_BREATH:
                currentTarget.Clear();
                Talk(EMOTE_BREATH);
                me->CastSpell(me, SPELL_FROST_MISSILE, false);
                events.ScheduleEvent(EVENT_FLIGHT_SPELL_EXPLOSION, 8500);
                return;
            case EVENT_FLIGHT_SPELL_EXPLOSION:
                me->CastSpell(me, SPELL_FROST_EXPLOSION, true);
                events.ScheduleEvent(EVENT_FLIGHT_START_LAND, 3000);
                return;
            case EVENT_FLIGHT_START_LAND:
                if (!blockList.empty())
                {
                    for (GuidList::const_iterator itr = blockList.begin(); itr != blockList.end(); ++itr)
                    {
                        if (Unit* block = ObjectAccessor::GetUnit(*me, *itr))
                        {
                            block->RemoveAurasDueToSpell(SPELL_ICEBOLT_TRIGGER);
                        }
                    }
                }
                blockList.clear();
                me->RemoveAllGameObjects();
                SetBotsToFollow();
                events.ScheduleEvent(EVENT_LAND, 1000);
                return;
            case EVENT_LAND:
                me->HandleEmoteCommand(EMOTE_ONESHOT_LAND);
                me->SetDisableGravity(false);
                events.ScheduleEvent(EVENT_GROUND, 1500);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC);
                return;
            case EVENT_GROUND:
                Talk(EMOTE_GROUND_PHASE);
                me->SetReactState(REACT_AGGRESSIVE);
                me->SetInCombatWithZone();
                inFlight = false;
                return;
            case EVENT_HUNDRED_CLUB:
            {
                Map::PlayerList const& pList = me->GetMap()->GetPlayers();
                for (const auto& itr : pList)
                {
                    if (itr.GetSource()->GetResistance(SPELL_SCHOOL_FROST) > 100 && pInstance)
                    {
                        pInstance->SetData(DATA_HUNDRED_CLUB, 0);
                        return;
                    }
                }
                events.RepeatEvent(5000);
                return;
            }
            }
            if (!inFlight)
            {
                DoMeleeAttackIfReady();
            }
        }
    };
};

// This will overwrite the declared 10 and 25 man frost explosion to handle all versions of the spell script
class spell_sapphiron_frost_explosion_40 : public SpellScriptLoader
{
public:
    spell_sapphiron_frost_explosion_40() : SpellScriptLoader("spell_sapphiron_frost_explosion") { }

    class spell_sapphiron_frost_explosion_40_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_sapphiron_frost_explosion_40_SpellScript);

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->ToCreature())
                return;

            std::list<WorldObject*> tmplist;
            for (auto& target : targets)
            {
                if (CAST_AI(boss_sapphiron_40::boss_sapphiron_40AI, caster->ToCreature()->AI())->IsValidExplosionTarget(target))
                {
                    if (target->IsPlayer())
                    {
                        Player* playerTarget = target->ToPlayer();
                        if (playerTarget && !playerTarget->IsNPCBot())
                        {
                            tmplist.push_back(target);
                        }
                    }
                    else if (target->GetTypeId() == TYPEID_UNIT && target->ToCreature()->IsNPCBot() && target->ToCreature()->GetLevel() <= 63)
                    {
                        tmplist.push_back(target);
                    }
                }
            }
            targets.clear();
            for (auto& itr : tmplist)
            {
                targets.push_back(itr);
            }
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sapphiron_frost_explosion_40_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_sapphiron_frost_explosion_40_SpellScript();
    }
};

void AddSC_boss_sapphiron_40()
{
    new boss_sapphiron_40();
    new spell_sapphiron_frost_explosion_40();
}
