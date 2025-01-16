#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
  public:
    Config()
    {
        reload();
    }

    // Функция reload() перезагружает настройки из файла settings.json
    // Это позволяет обновлять настройки игры без перезапуска программы
    void reload()
    {
        std::ifstream fin(project_path + "settings.json");
        fin >> config;
        fin.close();
    }

    // Оператор () позволяет удобно получать доступ к настройкам в формате:
    // config("Bot", "IsWhiteBot") вместо config.config["Bot"]["IsWhiteBot"]
    // Это делает код более читаемым и менее подверженным ошибкам
    auto operator()(const string &setting_dir, const string &setting_name) const
    {
        return config[setting_dir][setting_name];
    }

  private:
    json config;  // JSON-объект, хранящий все настройки игры
};
