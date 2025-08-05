#include "update_indicator.h"

UpdateIndicator::UpdateIndicator(QWidget *parent)
    : QWidget{parent}
{
    //Настраиваем визуалку
    setAttribute(Qt::WA_TranslucentBackground);  // делает фон прозрачным
    setAutoFillBackground(false);                 // не заливать фон самостоятельно
    setStyleSheet("border: none;");

    setFixedSize(10, 10);
    m_state = State::OFFLINE;
}

void UpdateIndicator::set_state(int new_state)
{
    if(new_state != m_state)
    {
        m_state = new_state;
        update();
    }
}


