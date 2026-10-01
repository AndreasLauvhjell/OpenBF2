// Client award catalogue and HUD presentation. [INFERRED] Names and source placement.
#include "HudAwardCatalogue.h"
#if !DEDICATED
#include "BF2HudManager.h"
#include "../Core/StringUtil.h"

namespace dice { namespace hfe { extern hud::BF2HudManager* hudManager; } }
namespace dice { namespace hfe { namespace hud {

HudAwardCatalogue::HudAwardCatalogue()
{
    populateFirstGroup();
    populateSecondGroup();
    populateThirdGroup();
}

HudAwardDescription* HudAwardCatalogue::findAward(unsigned int awardId, unsigned int level)
{
    const auto found = m_awards.find((std::uint64_t(level) << 32) | awardId);
    return found == m_awards.end() ? nullptr : &found->second;
}

void HudAwardCatalogue::addAward(unsigned int awardId, unsigned int level)
{
    // MSVC x86 evaluates these by-value arguments right to left: award lookup,
    // token construction, heading lookup, then token replacement. All three
    // calls use the live HUD singleton, without an added null guard. Preserve
    // temporary destruction at the full-expression boundary.
    const std::wstring message = hudManager->replaceWideToken(
        hudManager->getLocalizedString("HUD_HUD_AWARDED"), "#AWARD#",
        hudManager->getLocalizedString(level
            ? formatString("AWARD_Name_%i_%i", awardId, level)
            : formatString("AWARD_Name_%i", awardId)));
    const std::string iconPath = std::string("Ingame/GeneralIcons/Medals/")
        + (level ? formatString("AWARD_NAME_%i_%i.tga", awardId, level)
                 : formatString("AWARD_NAME_%i.tga", awardId));
    const std::string notification("AUTO_RULES_youbeenrecommendedforaward");
    HudAwardDescription& value = m_awards[(std::uint64_t(level) << 32) | awardId];
    value.message = message;
    value.iconPath = iconPath;
    value.notification = notification;
}

void HudAwardCatalogue::populateFirstGroup()
{
    addAward(1220118, 1);
    addAward(1220118, 2);
    addAward(1220118, 3);
    addAward(1222016, 1);
    addAward(1222016, 2);
    addAward(1222016, 3);
    addAward(1220803, 1);
    addAward(1220803, 2);
    addAward(1220803, 3);
    addAward(1220122, 1);
    addAward(1220122, 2);
    addAward(1220122, 3);
    addAward(1220104, 1);
    addAward(1220104, 2);
    addAward(1220104, 3);
    addAward(1031923, 1);
    addAward(1031923, 2);
    addAward(1031923, 3);
    addAward(1031406, 1);
    addAward(1031406, 2);
    addAward(1031406, 3);
    addAward(1031619, 1);
    addAward(1031619, 2);
    addAward(1031619, 3);
    addAward(1032415, 1);
    addAward(1032415, 2);
    addAward(1032415, 3);
    addAward(1190601, 1);
    addAward(1190601, 2);
    addAward(1190601, 3);
    addAward(1190507, 1);
    addAward(1190507, 2);
    addAward(1190507, 3);
    addAward(1191819, 1);
    addAward(1191819, 2);
    addAward(1191819, 3);
    addAward(1190304, 1);
    addAward(1190304, 2);
    addAward(1190304, 3);
    addAward(1031119, 1);
    addAward(1031119, 2);
    addAward(1031119, 3);
    addAward(1031120, 1);
    addAward(1031120, 2);
    addAward(1031120, 3);
    addAward(1031109, 1);
    addAward(1031109, 2);
    addAward(1031109, 3);
    addAward(1031115, 1);
    addAward(1031115, 2);
    addAward(1031115, 3);
    addAward(1031113, 1);
    addAward(1031113, 2);
    addAward(1031113, 3);
    addAward(1031121, 1);
    addAward(1031121, 2);
    addAward(1031121, 3);
    addAward(1031105, 1);
    addAward(1031105, 2);
    addAward(1031105, 3);
    addAward(2191319, 0);
    addAward(2190303, 0);
    addAward(2190309, 0);
    addAward(2190318, 0);
    addAward(2190308, 0);
    addAward(2020903, 0);
    addAward(2020913, 0);
    addAward(2020919, 0);
    addAward(2021322, 0);
    addAward(2020419, 0);
    addAward(2021403, 0);
    addAward(2020719, 0);
    addAward(2021613, 0);
    addAward(2191608, 0);
    addAward(2190703, 0);
    addAward(3240301, 0);
    addAward(3211305, 0);
    addAward(3150914, 0);
    addAward(3151920, 0);
    addAward(3190409, 0);
    addAward(3212201, 0);
    addAward(3190118, 0);
    addAward(3190105, 0);
    addAward(3190803, 0);
    addAward(3040109, 0);
    addAward(3040718, 0);
    addAward(3240102, 0);
    addAward(3240703, 0);
    addAward(3190318, 0);
    addAward(3241213, 0);
    addAward(3242303, 0);
}

void HudAwardCatalogue::populateSecondGroup()
{
    addAward(1261119, 1);
    addAward(1261119, 2);
    addAward(1261119, 3);
    addAward(1261120, 1);
    addAward(1261120, 2);
    addAward(1261120, 3);
    addAward(1261109, 1);
    addAward(1261109, 2);
    addAward(1261109, 3);
    addAward(1261115, 1);
    addAward(1261115, 2);
    addAward(1261115, 3);
    addAward(1261121, 1);
    addAward(1261121, 2);
    addAward(1261121, 3);
    addAward(1261105, 1);
    addAward(1261105, 2);
    addAward(1261105, 3);
    addAward(1261113, 1);
    addAward(1261113, 2);
    addAward(1261113, 3);
    addAward(1260602, 1);
    addAward(1260602, 2);
    addAward(1260602, 3);
    addAward(1260708, 1);
    addAward(1260708, 2);
    addAward(1260708, 3);
    addAward(1262612, 1);
    addAward(1262612, 2);
    addAward(1262612, 3);
    addAward(2261913, 0);
    addAward(2261919, 0);
    addAward(2261613, 0);
    addAward(2261303, 0);
    addAward(2261802, 0);
    addAward(2260914, 0);
    addAward(3261919, 0);
    addAward(3261901, 0);
    addAward(3261819, 0);
    addAward(3261319, 0);
    addAward(3261805, 0);
    addAward(3260914, 0);
    addAward(3260318, 0);
    addAward(3260118, 0);
    addAward(3260803, 0);
}

void HudAwardCatalogue::populateThirdGroup()
{
    addAward(2270521, 0);
    addAward(3271401, 0);
}

} } }
#endif
