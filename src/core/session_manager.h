#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <QObject>
#include <QStringList>
#include <QByteArray>

namespace Core {

struct SessionState {
    QStringList openFiles;
    int activeTabIndex = 0;
    QString projectRoot;
    QByteArray windowGeometry;
    QByteArray windowState;
    QByteArray splitterState;
};

class SessionManager : public QObject {
    Q_OBJECT

public:
    static SessionManager& instance();

    void saveSession(const SessionState &state);
    SessionState loadSession() const;

private:
    SessionManager();
    QString sessionFilePath() const;

    
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;
};

} 

#endif 
