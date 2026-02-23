#ifndef DIFF_UTILS_H
#define DIFF_UTILS_H

#include <QString>
#include <QStringList>
#include <QList>

namespace GitScm {

struct DiffEdit {
    enum Type {
        Keep,
        Insert,
        Delete
    };

    Type type;
    int oldLine; 
    int newLine; 
    QString text;
};

QList<DiffEdit> computeMyersDiff(const QStringList &oldLines, const QStringList &newLines);

}

#endif
