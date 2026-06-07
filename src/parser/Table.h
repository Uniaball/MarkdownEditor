#pragma once
#include <QString>
#include <QStringList>

namespace TableParser {
    QString convertTable(const QStringList &lines);
}