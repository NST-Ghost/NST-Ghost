#include "core/engines/godot/godotanalyzer.h"
#include "core/bga_rust_bridge.h"
#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

namespace core { namespace engines { namespace godot {

core::AnalyzerOutput GodotAnalyzer::analyze(const QString &inputPath)
{
    RustAnalyzerBridge bridge(QStringLiteral("godot"));
    return bridge.analyze(inputPath);
}

bool GodotAnalyzer::save(const QString &outputPath, const QJsonArray &texts)
{
    RustAnalyzerBridge bridge(QStringLiteral("godot"));
    return bridge.save(outputPath, texts);
}

} // namespace godot
} // namespace engines
} // namespace core
