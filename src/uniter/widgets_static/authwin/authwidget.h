#ifndef AUTHWDG_H
#define AUTHWDG_H

#include "../../contract/unitermessage.h"
//#include <qtkeychain/keychain.h> TODO: добавить безопасное хранение логина и пароля
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <memory>


namespace uniter::staticwdg {

class AuthWdg : public QWidget {
    Q_OBJECT

private:
    // Состояние
    enum class RememberCheck {SKIPPED, REMEMBER};
    RememberCheck RCheck = RememberCheck::REMEMBER;

    // Виджеты
    QLineEdit* m_loginInput = nullptr;
    QLineEdit* m_passwordInput = nullptr;
    QCheckBox* m_rememberCheckBox = nullptr;
    QPushButton* m_loginButton = nullptr;
    QLabel* m_errorMessageLabel = nullptr;

    // Флаги для удаления placeholder текста при первом клике
    bool m_loginCleared = false;
    bool m_passwordCleared = false;

    // Приватные методы
    void sendSignUpData();
    void saveCredentials(const QString& login, const QString& password);
    void loadCredentials();
public:
    explicit AuthWdg(QWidget* parent = nullptr);
    ~AuthWdg() = default;

    AuthWdg(const AuthWdg&) = delete;
    AuthWdg(AuthWdg&&) = delete;
    AuthWdg& operator=(const AuthWdg&) = delete;
    AuthWdg& operator=(AuthWdg&&) = delete;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

public slots:
    // От главного окна
    void onAuthed(bool result);
    void onFindAuthData();

    // Внутренние
    void onRememberCheckChanged(int state);

signals:
    void signalSendUniterMessage(std::shared_ptr<uniter::contract::UniterMessage> Message);
};

} // namespace uniter::staticwdg


#endif // AUTHWDG_H
