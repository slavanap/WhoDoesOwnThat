// Copyright Vyacheslav Napadovsky, 2015.

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <Windows.h>
#undef min
#undef max

UINT GetOwner(LPCWSTR szFileOrFolderPathName, LPWSTR pUserNameBuffer, int nSizeInBytes);

QString getOwner(const QString& path) {
    static const int size = 65536;
    static LPWSTR buffer = new WCHAR[size];
    std::wstring wstr = path.toStdWString();
    if (GetOwner(wstr.c_str(), buffer, size) == S_OK)
        return QString::fromWCharArray(buffer);
    return QString();
}

class MyFileModel: public QFileSystemModel {
public:
    MyFileModel(QObject* parent = 0): QFileSystemModel(parent) {}
    QVariant data(const QModelIndex& index, int role) const {
        if (index.column() != 4)
            return QFileSystemModel::data(index, role);
        if (role == Qt::DisplayRole)
            return getOwner(filePath(index));
        return QVariant();
    }
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const {
        if (section != 4)
            return QFileSystemModel::headerData(section, orientation, role);
        if (role == Qt::DisplayRole)
            return "Owner";
        return QVariant();
    }
    int columnCount(const QModelIndex& parent = QModelIndex()) const {
        Q_UNUSED(parent);
        return 5;
    }
};



void MainWindow::notify_expanded(const QModelIndex& index) {
    QTreeView *view = dynamic_cast<QTreeView*>(sender());
    if (!view)
        return;
    Q_UNUSED(index);
    int w, neww;
    w = view->columnWidth(0);
    view->resizeColumnToContents(0);
    neww = view->columnWidth(0);
    view->setColumnWidth(0, std::max(w, neww+20));
}

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    model(NULL)
{
    ui->setupUi(this);
    connect(ui->treeView, SIGNAL(expanded(QModelIndex)), this, SLOT(notify_expanded(QModelIndex)));

    model = new MyFileModel(this);
    model->setRootPath(model->myComputer().toString());
    ui->treeView->setModel(model);
    ui->treeView->setSortingEnabled(true);
    ui->treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_lineEdit_editingFinished() {
    QModelIndex index = model->setRootPath(ui->lineEdit->text());
    ui->treeView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    QModelIndex parent = index.parent();
    while (parent != QModelIndex()) {
        ui->treeView->expand(parent);
        parent = parent.parent();
    }
    ui->treeView->scrollTo(index, QAbstractItemView::PositionAtTop);
}
