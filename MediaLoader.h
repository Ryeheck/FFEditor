#ifndef MEDIALOADER_H
#define MEDIALOADER_H

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QTabWidget>
#include <QLabel>
#include <QDialog>

class MediaLoader : public QDialog
{
    Q_OBJECT

public:
    MediaLoader(QWidget *parent = nullptr);
    // ~MediaLoader();

private slots:
    void onAddButtonClicked();

signals:
    void startPlayRequested(const QString &path);

private:
    QListWidget *listWidget;
    QTabWidget  *tabWidget;
    QWidget *fileTab;
    QWidget *networkTab;
    QVBoxLayout *loaderLayout;
    QPushButton *addButton;
    QPushButton *removeButton;
    QPushButton *playButton;
    QPushButton *closeButton;
};

#endif // MEDIALOADER_H