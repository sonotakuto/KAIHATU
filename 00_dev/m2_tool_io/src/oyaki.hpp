#pragma once
#include <SimpleGtk>

#ifdef DECL
    map<int, unique_ptr<Window>> win;
    auto& loginWindow=win[1];
    auto& settingWindow=win[2];
    auto& mainWindow=win[3];
    auto& dbWindow=win[4];
#else
    extern unique_ptr<LoginWindow>& loginWindow;
    extern unique_ptr<SettingWindow>& settingWindow;
    extern unique_ptr<MainWindow>& mainWindow;
    extern unique_ptr<DbWindow>& dbWindow;
#endif
