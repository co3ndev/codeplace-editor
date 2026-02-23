#ifndef DIFF_VIEW_H
#define DIFF_VIEW_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QHBoxLayout>
#include <QScrollBar>
#include "core/simple_highlighter.h"
#include "diff_utils.h"

namespace GitScm {

class DiffView : public QWidget {
    Q_OBJECT

public:
    explicit DiffView(QWidget *parent = nullptr);
    ~DiffView() override = default;
    
    void setDiff(const QString &oldContent, const QString &newContent, const QString &fileName);

private slots:
    void syncScrollLeft(int value);
    void syncScrollRight(int value);
    void onThemeChanged();

private:
    void setupUI();
    void applyChangeHighlights(const QString &oldContent, const QString &newContent);

    QPlainTextEdit *leftEditor;
    QPlainTextEdit *rightEditor;
    Core::SimpleHighlighter *leftHighlighter = nullptr;
    Core::SimpleHighlighter *rightHighlighter = nullptr;
    QString m_fileName;
    QString m_oldContent;
    QString m_newContent;
};

}

#endif
