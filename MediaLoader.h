#ifndef MEDIALOADER_H
#define MEDIALOADER_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QTabWidget>
#include <QLabel>

class MediaLoader : public QWidget
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
    QWidget *tab1Content;
    QWidget *tab2Content;
    QVBoxLayout *loaderLayout;
    QPushButton *addButton;
    QPushButton *removeButton;
    QPushButton *playButton;
};

#endif // MEDIALOADER_H