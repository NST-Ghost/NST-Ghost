#include "core/engines/renpy/renpyanalyzer.h"
#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QFile>
#include <QtCore/QDir>

// FFI declarations for renpy_rs
extern "C" {
    char* renpy_analyze(const char* path);
    int renpy_save(const char* path, const char* texts_json);
    void renpy_free_string(char* ptr);
}

namespace core { namespace engines { namespace renpy {

core::AnalyzerOutput RenpyAnalyzer::analyze(const QString &inputPath)
{
    core::AnalyzerOutput output;
    output.format = QStringLiteral("application/json");

    QByteArray pathBytes = inputPath.toUtf8();
    char* result = renpy_analyze(pathBytes.constData());

    if (result == nullptr) {
        output.errorMessage = QStringLiteral("Ren'Py Rust analyzer returned null");
        return output;
    }

    output.payload = QByteArray(result);
    renpy_free_string(result);

    return output;
}

bool RenpyAnalyzer::save(const QString &outputPath, const QJsonArray &texts)
{
    Q_UNUSED(outputPath);
    
    QJsonDocument doc(texts);
    QByteArray jsonBytes = doc.toJson(QJsonDocument::Compact);
    QByteArray pathBytes = outputPath.toUtf8();

    int result = renpy_save(pathBytes.constData(), jsonBytes.constData());
    return result == 0;
}

} // namespace renpy
} // namespace engines
} // namespace core
