#ifndef OFFLINEWDG_H
#define OFFLINEWDG_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

namespace staticwdg {

class OfflineWdg : public QWidget
{
    Q_OBJECT
private:
    QLabel* m_label;
    QPushButton* m_btnConnect;

public:
    explicit OfflineWdg(QWidget *parent = nullptr);

signals:
    void signalMakeConnect();
};

} // namespace staticwdg

#endif // OFFLINEWDG_H
