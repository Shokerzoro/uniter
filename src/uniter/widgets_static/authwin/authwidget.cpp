
#include "authwidget.h"
//#include <qtkeychain/keychain.h> TODO: добавить безопасное хранение логина и пароля
#include <QSettings>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QEvent>
#include <QFocusEvent>
#include <QDebug>
#include <map>

namespace uniter::staticwdg {

AuthWdg::AuthWdg(QWidget* parent)
    : QWidget(parent)
{
    // Основной layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Заголовок
    QLabel* titleLabel = new QLabel("Login", this);
    mainLayout->addWidget(titleLabel);

    // Поле логина
    m_loginInput = new QLineEdit(this);
    m_loginInput->setPlaceholderText("my@email.com");  // <- вместо setText
    m_loginInput->installEventFilter(this);
    mainLayout->addWidget(m_loginInput);

    // Поле пароля
    m_passwordInput = new QLineEdit(this);
    m_passwordInput->setPlaceholderText("qwerty");  // <- вместо setText
    m_passwordInput->setEchoMode(QLineEdit::Password);
    m_passwordInput->installEventFilter(this);
    mainLayout->addWidget(m_passwordInput);

    // Чекбокс "Запомнить"
    m_rememberCheckBox = new QCheckBox("Remember me", this);
    m_rememberCheckBox->setChecked(true);
    mainLayout->addWidget(m_rememberCheckBox);

    // Кнопка входа
    m_loginButton = new QPushButton("Login", this);
    mainLayout->addWidget(m_loginButton);

    // Скрытое поле ошибки
    m_errorMessageLabel = new QLabel(this);
    m_errorMessageLabel->setStyleSheet("color: red;");
    m_errorMessageLabel->setVisible(false);
    mainLayout->addWidget(m_errorMessageLabel);

    // Растягиваем пространство
    mainLayout->addStretch();

    this->setLayout(mainLayout);

    // Подключаем сигналы
    connect(m_rememberCheckBox, &QCheckBox::checkStateChanged,
            this, &AuthWdg::onRememberCheckChanged);


    connect(m_loginButton, &QPushButton::clicked,
            this, [this]() { this->sendSignUpData(); });

}

void AuthWdg::onFindAuthData() {
    qDebug() << "AuthWdg::onFindAuthData()";

    QSettings settings("Uniter", "Uniter");
    QString login = settings.value("last_login", "").toString();
    QString password = settings.value("last_password", "").toString();

    qDebug() << "login:" << login << ", password:" << password;

    if (!login.isEmpty() && !password.isEmpty()) {

        m_loginInput->setText(login);
        m_passwordInput->setText(password);
        sendSignUpData();
    }


}


void AuthWdg::sendSignUpData() {

    qDebug() << "AuthWdg::sendSignUpData()";

    QString login = m_loginInput->text();
    QString password = m_passwordInput->text();

    auto message = std::make_shared<uniter::contract::UniterMessage>();
    message->subsystem = contract::Subsystem::PROTOCOL;
    message->protact = contract::ProtocolAction::AUTH;
    message->add_data.emplace("login", login);
    message->add_data.emplace("password", password);

    emit signalSendUniterMessage(message);
}


void AuthWdg::onAuthed(bool result)
{
    if (result) {
        if (RCheck == RememberCheck::REMEMBER) {
            saveCredentials(m_loginInput->text(), m_passwordInput->text());
        }
        m_errorMessageLabel->setVisible(false);
    } else {
        m_errorMessageLabel->setText("Authentication failed. Please check your credentials.");
        m_errorMessageLabel->setVisible(true);
        m_passwordInput->clear();
        m_passwordCleared = false;
    }
}


bool AuthWdg::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_loginInput && event->type() == QEvent::FocusIn) {
        if (!m_loginCleared) {
            m_loginInput->clear();
            m_loginCleared = true;
        }
        return false;
    }

    if (obj == m_passwordInput && event->type() == QEvent::FocusIn) {
        if (!m_passwordCleared) {
            m_passwordInput->clear();
            m_passwordCleared = true;
        }
        return false;
    }

    return QWidget::eventFilter(obj, event);
}

void AuthWdg::onRememberCheckChanged(int state)
{
    if (state == Qt::Checked) {
        RCheck = RememberCheck::REMEMBER;
    } else {
        RCheck = RememberCheck::SKIPPED;
    }
}

void AuthWdg::saveCredentials(const QString& login, const QString& password)
{
    QSettings settings("Uniter", "Uniter");
    settings.setValue("last_login", login);
    settings.setValue("last_password", password);
    settings.sync();
}

void AuthWdg::loadCredentials()
{
    QSettings settings("Uniter", "Uniter");
    QString login = settings.value("last_login", "").toString();
    QString password = settings.value("last_password", "").toString();

    if (!login.isEmpty()) {
        m_loginInput->setText(login);
    }
    if (!password.isEmpty()) {
        m_passwordInput->setText(password);
    }
}



} // namespace uniter::staticwdg


