#ifndef CREATE_BRANCH_DIALOG_H
#define CREATE_BRANCH_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

namespace GitScm {

class CreateBranchDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreateBranchDialog(QWidget *parent = nullptr);

    QString branchName() const;

private:
    QLineEdit *nameEdit;
    QPushButton *createButton;
    QPushButton *cancelButton;
};

}

#endif 
