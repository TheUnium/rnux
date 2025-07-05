#include "system_commands.h"
#include <algorithm>

SystemCommands::SystemCommands() {
    m_commands = {
        FeatureItem("Shutdown", "Power off the system", "system-shutdown", "shutdown -h now", "system"),
        FeatureItem("Restart", "Restart the system", "system-reboot", "reboot", "system"),
        FeatureItem("Log Out", "Log out of current session", "system-log-out", "pkill -KILL -u $USER", "system"),
        FeatureItem("Lock Screen", "Lock the screen", "system-lock-screen", "loginctl lock-session", "system"),
        FeatureItem("Sleep", "Put system to sleep", "system-suspend", "systemctl suspend", "system"),
        FeatureItem("File Manager", "Open file manager", "folder", "xdg-open ~", "system"),
        FeatureItem("Terminal", "Open terminal", "utilities-terminal", "x-terminal-emulator", "system"),
        FeatureItem("Settings", "Open system settings", "preferences-system", "gnome-control-center", "system")
    };
}

QList<FeatureItem> SystemCommands::search(const QString& query) {
    QList<FeatureItem> results;
    if (query.isEmpty()) {
        return m_commands.mid(0, 6);
    }
    
    QList<QPair<FeatureItem, int>> scored;
    for (const auto& cmd : m_commands) {
        if (int score = fuzzyMatch(query.toLower(), cmd.title.toLower()); score > 0) {
            scored.append({cmd, score});
        }
    }
    
    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (const auto&[fst, snd] : scored) {
        results.append(fst);
        if (results.size() >= 6) break;
    }
    
    return results;
}

void SystemCommands::execute(const FeatureItem& item) {
    QProcess::startDetached("/bin/sh", {"-c", item.data});
}

int SystemCommands::fuzzyMatch(const QString& query, const QString& text) {
    if (query.isEmpty()) return 1;
    int queryPos = 0;
    int score = 0;
    
    for (int i = 0; i < text.length() && queryPos < query.length(); ++i) {
        if (text[i] == query[queryPos]) {
            queryPos++;
            score += (queryPos == 1) ? 10 : 1;
        }
    }
    
    return (queryPos == query.length()) ? score : 0;
}