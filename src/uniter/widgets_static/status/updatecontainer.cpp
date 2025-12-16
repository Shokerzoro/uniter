#include "updatecontainer.h"
#include "update_indicator.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDebug>

UpdateContainer::UpdateContainer(QWidget *parent)
    : QWidget{parent}
{
    //Настраиваем визуалку
    setAttribute(Qt::WA_TranslucentBackground);  // делает фон прозрачным
    setAutoFillBackground(false);                 // не заливать фон самостоятельно
    setStyleSheet("border: none;");
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 5, 10, 5);
    layout->setSpacing(10);
    layout->addStretch();  // Если нужно выравнивание

    //Добавляем виджеты
    update_indicator = new UpdateIndicator();
    update_label = new QLabel();
    agree_button = new QToolButton();
    connect(agree_button, &QToolButton::clicked, this, &UpdateContainer::signalMakeUpdates);
    refuse_button = new QToolButton();
    connect(refuse_button, &QToolButton::clicked, this, &UpdateContainer::signalRefuseUpdates);

    layout->addWidget(update_indicator);
    layout->addWidget(update_label);
    layout->addWidget(agree_button);
    layout->addWidget(refuse_button);
    setLayout(layout);

    //Устанавливаем первоначальное состояние
    update_indicator->set_state(UpdateIndicator::State::OFFLINE);
    update_label->setText("Нет соединения");

    //Установить на клик отправку emit signalMaleUpdates();
    agree_button->setText("обновить");
    //Установить на клик отправку imot signalRefuseUpdates();
    refuse_button->setText("отложить");

    agree_button->hide();
    refuse_button->hide();
}

void UpdateContainer::UpWorkerNoUpdaterExe()
{
    qDebug() << "UpdateContainer: не найден updater.exe";
    update_indicator->set_state(UpdateIndicator::State::WARNING);
    update_label->setText("Обновление невозможно: нет updater.exe");
    agree_button->hide();
    refuse_button->hide();
}

void UpdateContainer::UpWorkerNoRecoverExe()
{
    qDebug() << "UpdateContainer: не найден recover.exe";
    update_indicator->set_state(UpdateIndicator::State::WARNING);
    update_label->setText("Обновление невозможно: нет recover.exe");
    agree_button->hide();
    refuse_button->hide();
}

void UpdateContainer::UpWorkerNoServerData()
{
    qDebug() << "UpdateContainer: данные от сервера не получены";
    update_indicator->set_state(UpdateIndicator::State::WARNING);
    update_label->setText("Нет данных о сервере");
    agree_button->hide();
    refuse_button->hide();
}

void UpdateContainer::UpWorkerOnline()
{
    qDebug() << "UpdateContainer: соединение с сервером установлено";
    update_indicator->set_state(UpdateIndicator::State::ONLINE);
    update_label->setText("Соединение установлено");
    agree_button->hide();
    refuse_button->hide();
}

void UpdateContainer::UpWorkerOffline()
{
    qDebug() << "UpdateContainer: сервер недоступен";
    update_indicator->set_state(UpdateIndicator::State::OFFLINE);
    update_label->setText("Нет соединения");
    agree_button->hide();
    refuse_button->hide();
}

void UpdateContainer::UpdateReady()
{
    qDebug() << "UpdateContainer: доступно обновление";
    update_indicator->set_state(UpdateIndicator::State::WARNING);
    update_label->setText("Доступно обновление");
    agree_button->show();
    refuse_button->show();
}

