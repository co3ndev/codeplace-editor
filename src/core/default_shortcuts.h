#ifndef DEFAULT_SHORTCUTS_H
#define DEFAULT_SHORTCUTS_H

#include <QMap>
#include <QString>

namespace Core {

inline const QMap<QString, QString>& defaultShortcuts() {
    static const QMap<QString, QString> shortcuts = {
        {"New",              "Ctrl+N"},
        {"Open",             "Ctrl+O"},
        {"Open Folder",      "Ctrl+K"},
        {"Save",             "Ctrl+S"},
        {"Save As",          "Ctrl+Shift+S"},
        {"Save All",         "Ctrl+Alt+S"},
        {"Exit",             "Ctrl+Q"},
        {"Undo",             "Ctrl+Z"},
        {"Redo",             "Ctrl+Y"},
        {"Cut",              "Ctrl+X"},
        {"Copy",             "Ctrl+C"},
        {"Paste",            "Ctrl+V"},
        {"Select All",       "Ctrl+A"},
        {"Find",             "Ctrl+F"},
        {"Find Next",        "Ctrl+G"},
        {"Find Previous",    "Ctrl+Shift+G"},
        {"Replace",          "Ctrl+H"},
        {"Go to Line",       "Ctrl+L"},
        {"Close Tab",        "Ctrl+W"},
        {"Next Tab",         "Ctrl+PgDown"},
        {"Previous Tab",     "Ctrl+PgUp"},
        {"Zoom In",          "Ctrl++"},
        {"Zoom Out",         "Ctrl+-"},
        {"Settings",         "Ctrl+,"},
        {"Toggle File Browser", "Ctrl+B"},
        {"Toggle Terminal",  "Ctrl+Shift+`"},
        {"Toggle Search",    "Ctrl+Shift+B"},
        {"Toggle Outline",   "Ctrl+Shift+J"},
        {"Focus File Browser", "Ctrl+Shift+E"},
        {"Focus Terminal",   "Ctrl+`"},
        {"Focus Search",     "Ctrl+Shift+F"},
        {"Focus Outline",    "Ctrl+Shift+O"},
        {"Comment/Uncomment", "Ctrl+/"},
        {"Format Document",  "Alt+Shift+F"},
        {"Go to Definition", "F12"},
        {"Rename Symbol",    "F2"},
        {"Quick Fix",        "Ctrl+."},
        {"Trigger Suggestion", "Ctrl+Space"},
        {"New File (Browser)", "Ctrl+Alt+N"},
        {"New Folder (Browser)", "Ctrl+Alt+Shift+N"},
        {"Rename",           "F2"},
        {"Delete",           "Delete"},
        {"Copy Path",        "Ctrl+Shift+C"},
        {"Reveal in Explorer", "Ctrl+Alt+R"},
    };
    return shortcuts;
}

} 

#endif 
