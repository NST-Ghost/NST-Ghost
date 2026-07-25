#include "core/analyzerfactory.h"
#include "core/engines/rpgm/rpganalyzer.h"
#include "core/engines/renpy/renpyanalyzer.h"
#include "core/engines/unity/unityanalyzer.h"
#include "core/engines/godot/godotanalyzer.h"

namespace core {

std::unique_ptr<IGameAnalyzer> createAnalyzer(const QString &engineName)
{
    QString name = engineName.toLower();
    if (name == "rpgm") {
        return std::make_unique<engines::rpgm::RpgmAnalyzer>();
    } else if (name == "renpy") {
        return std::make_unique<engines::renpy::RenpyAnalyzer>();
    } else if (name == "unity") {
        return std::make_unique<engines::unity::UnityAnalyzer>();
    } else if (name == "godot") {
        return std::make_unique<engines::godot::GodotAnalyzer>();
    }
    return nullptr;
}

QStringList availableAnalyzers()
{
    return { "rpgm", "renpy", "unity", "godot" };
}

} // namespace core

