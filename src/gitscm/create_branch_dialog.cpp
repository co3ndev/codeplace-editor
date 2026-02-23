#include "create_branch_dialog.h"

namespace GitScm {

CreateBranchDialog::CreateBranchDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Create Branch");
    setMinimumWidth(300);

    auto *layout = new QVBoxLayout(this);

    auto *label = new QLabel("New branch name:", this);
    layout->addWidget(label);

    nameEdit = new QLineEdit(this);
    nameEdit->setPlaceholderText("e.g. feature/my-new-feature");
    layout->addWidget(nameEdit);

    auto *buttonsLayout = new QHBoxLayout();
    createButton = new QPushButton("Create", this);
    createButton->setDefault(true);
    cancelButton = new QPushButton("Cancel", this);

    buttonsLayout->addStretch();
    buttonsLayout->addWidget(createButton);
    buttonsLayout->addWidget(cancelButton);

    layout->addLayout(buttonsLayout);

    connect(createButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    connect(nameEdit, &QLineEdit::textChanged, [this](const QString &text) {
        createButton->setEnabled(!text.trimmed().isEmpty());
    });
    createButton->setEnabled(false);
}

QString CreateBranchDialog::branchName() const {
    return nameEdit->text().trimmed();
}

}
