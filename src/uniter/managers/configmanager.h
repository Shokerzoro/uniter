


namespace uniter::managers {

// Управляет созданием подсистем
class ConfigManager : public QObject
{
    Q_OBJECT
public:
    ConfigManager() {}
    virtual ~ConfigManager() {}

    // Генерация конфигурации
public slots:
    void GenerateConfiguration();

};


} // managers
