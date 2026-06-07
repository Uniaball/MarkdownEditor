#pragma once
#include <QString>
#include <QStringList>
#include <QSet>

namespace FoldBlock {
    void resetCounter();
    QString convertDetailsBlock(const QStringList &lines, const QSet<QString> &expandedIds);
}