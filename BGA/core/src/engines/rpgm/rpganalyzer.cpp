#include "core/engines/rpgm/rpganalyzer.h"
#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QFile>
#include <QtCore/QDir>

// FFI declarations for rpgm_rs
extern "C" {
    char* rpgm_analyze(const char* path);
    int rpgm_save(const char* path, const char* texts_json);
    void rpgm_free_string(char* ptr);
}

namespace core { namespace engines { namespace rpgm {

core::AnalyzerOutput RpgmAnalyzer::analyze(const QString &inputPath)
{
    core::AnalyzerOutput output;
    output.format = QStringLiteral("application/json");

    QByteArray pathBytes = inputPath.toUtf8();
    char* result = rpgm_analyze(pathBytes.constData());

    if (result == nullptr) {
        output.errorMessage = QStringLiteral("RPGM Rust analyzer returned null");
        return output;
    }

    output.payload = QByteArray(result);
    rpgm_free_string(result);

    return output;
}

bool RpgmAnalyzer::save(const QString &outputPath, const QJsonArray &texts)
{
    Q_UNUSED(outputPath);
    
    QJsonDocument doc(texts);
    QByteArray jsonBytes = doc.toJson(QJsonDocument::Compact);
    QByteArray pathBytes = outputPath.toUtf8();

    int result = rpgm_save(pathBytes.constData(), jsonBytes.constData());
    return result == 0;
}

QString RpgmAnalyzer::getScriptPath(const QString &projectPath) const
{
    QDir projectDir(projectPath);
    QString scriptPath = projectDir.absoluteFilePath("www/js/rpg_windows.js");
    if (!QFile::exists(scriptPath)) {
        scriptPath = projectDir.absoluteFilePath("js/rpg_windows.js");
    }
    return scriptPath;
}

QString RpgmAnalyzer::getScriptTarget() const
{
    return QStringLiteral("Window_Base.prototype.convertEscapeCharacters");
}

} // namespace rpgm
} // namespace engines
} // namespace core
