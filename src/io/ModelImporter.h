#pragma once

#include <QString>

struct ImportResult {
    bool success {false};
    QString message;
    QString objectName;
};

class ModelImporter {
public:
    static ImportResult importModel(const QString& path);
};
