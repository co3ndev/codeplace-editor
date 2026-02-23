#ifndef QUICK_START_WIDGET_H
#define QUICK_START_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>

namespace Editor {

class QuickStartWidget final : public QWidget {
    Q_OBJECT

public:
    explicit QuickStartWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUi();
    void addShortcutRow(QGridLayout *layout, int row, const QString &label, const QString &shortcut);
};

} 

#endif 
