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
    QListWidget *m_listWidget;
    QTabWidget  *m_tabWidget;
    QWidget *m_fileTab;
    QWidget *m_networkTab;
    QVBoxLayout *m_loaderLayout;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QPushButton *m_playButton;
    QPushButton *m_closeButton;
};

#endif // MEDIALOADER_H