#ifndef PROBLEMS_WIDGET_H
#define PROBLEMS_WIDGET_H

#include <QTreeWidget>
#include <QJsonArray>
#include <QMap>
#include <QSet>
#include <QStringList>

namespace LspClient {

class ProblemsWidget : public QTreeWidget {
    Q_OBJECT

public:
    explicit ProblemsWidget(QWidget *parent = nullptr);
    ~ProblemsWidget() override = default;

public slots:
    void onDiagnosticsReady(const QString &filePath, const QJsonArray &diagnostics);
    void updateTheme();
    void onOpenFilesChanged(const QStringList &openFiles);

signals:
    void problemSelected(const QString &filePath, int line, int column);

private slots:
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onContextMenuRequested(const QPoint &pos);

private:
    void updateFileItem(const QString &filePath);
    
    QMap<QString, QJsonArray> m_diagnostics;
    QMap<QString, QTreeWidgetItem*> m_fileItems;
    QSet<QString> m_openFiles;
};

} 

#endif 
