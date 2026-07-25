#ifndef GODOT_ANALYZER_H
#define GODOT_ANALYZER_H

#include "core/gameanalyzer.h"

namespace core {
namespace engines {
namespace godot {

class GodotAnalyzer : public core::IGameAnalyzer {
public:
    core::AnalyzerOutput analyze(const QString &inputPath) override;
    bool save(const QString &outputPath, const QJsonArray &texts) override;
};

} // namespace godot
} // namespace engines
} // namespace core

#endif // GODOT_ANALYZER_H
