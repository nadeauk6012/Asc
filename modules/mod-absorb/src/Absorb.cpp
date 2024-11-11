#include "Absorb.h"

class Absorb_UnitScript : public UnitScript
{
private:
    static const std::unordered_set<uint32> absorbSpells;
    static constexpr float absorbAmount = 0.20f; // 20% absorb amount

public:
    Absorb_UnitScript() : UnitScript("Absorb_UnitScript") { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (!attacker || attacker->GetTypeId() != TYPEID_PLAYER)
            return;

        Player* player = attacker->GetOwner() && attacker->GetOwner()->GetTypeId() == TYPEID_PLAYER
            ? attacker->GetOwner()->ToPlayer() : attacker->ToPlayer();

        if (!player || !player->HasAura(SPELL_AURA_ABSORPTION) || player->GetAuraCount(SPELL_ABSORPTION) >= 3)
            return;

        Spell* currentSpell = player->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (!currentSpell)
            return;

        SpellInfo const* spellInfo = currentSpell->GetSpellInfo();
        if (!spellInfo || absorbSpells.find(spellInfo->Id) == absorbSpells.end())
            return;

        int32 bp1 = static_cast<int32>(absorbAmount * damage);
        player->CastCustomSpell(player, SPELL_ABSORPTION, &bp1, nullptr, nullptr, true); // Cast absorption spell on the player
    }
};

const std::unordered_set<uint32> Absorb_UnitScript::absorbSpells = {
    SPELL_BLOOD_STRIKE1, SPELL_BLOOD_STRIKE2, SPELL_BLOOD_STRIKE3,
    SPELL_BLOOD_STRIKE4, SPELL_BLOOD_STRIKE5, SPELL_BLOOD_STRIKE6,
    SPELL_HEART_STRIKE1, SPELL_HEART_STRIKE2, SPELL_HEART_STRIKE3,
    SPELL_HEART_STRIKE4, SPELL_HEART_STRIKE5, SPELL_HEART_STRIKE6,
    SPELL_OBLITERATE1, SPELL_OBLITERATE2, SPELL_OBLITERATE3, SPELL_OBLITERATE4,
    SPELL_SCOURGE_STRIKE1, SPELL_SCOURGE_STRIKE2, SPELL_SCOURGE_STRIKE3, SPELL_SCOURGE_STRIKE4
};

// Add all scripts in one
void AddSC_mod_absorb()
{
    new Absorb_UnitScript();
}
