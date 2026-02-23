#ifndef OUTLINE_WIDGET_H
#define OUTLINE_WIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QTimer>
#include "symbol_parser.h"

namespace Editor {
    class EditorView;
}

namespace Outline {

class OutlineWidget : public QWidget {
    Q_OBJECT

public:
    explicit OutlineWidget(QWidget *parent = nullptr);

    void setEditor(Editor::EditorView *editor);

public slots:
    void refreshOutline();

signals:
    void symbolSelected(int line, int column);

private slots:
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onContextMenuRequested(const QPoint &pos);
    void onDocumentSymbolsReady(const QString &uri, const QJsonArray &symbols);

private:
    void setupUI();
    void parseSymbols();
    int getSymbolIndexForItem(QTreeWidgetItem *item) const;

    QTreeWidget *m_treeWidget;
    Editor::EditorView *m_currentEditor = nullptr;
    QTimer *m_refreshTimer;
    QVector<OutlineSymbol> m_currentSymbols;
};

} 

#endif 
