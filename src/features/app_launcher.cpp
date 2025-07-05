#include "app_launcher.h"
#include <QTextStream>
#include <QCoreApplication>
#include <algorithm>

AppLauncher::AppLauncher() {
    loadApplications();
}

QList<FeatureItem> AppLauncher::search(const QString& query) {
    QList<FeatureItem> results;
    if (query.isEmpty()) {
        return m_applications.mid(0, 8); // top 8 apps
    }
    
    QList<QPair<FeatureItem, int>> scored;
    for (const auto& app : m_applications) {
        if (int score = fuzzyMatch(query.toLower(), app.title.toLower()); score > 0) {
            scored.append({app, score});
        }
    }
    
    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (const auto&[fst, snd] : scored) {
        results.append(fst);
        if (results.size() >= 8) break;
    }
    
    return results;
}

void AppLauncher::execute(const FeatureItem& item) {
    QStringList args = QProcess::splitCommand(item.data);
    if (args.isEmpty()) return;

    const QString program = args.takeFirst();
    QProcess::startDetached(program, args);
}

void AppLauncher::loadApplications() {
    m_applications.clear();
    m_seenApps.clear();
    
    QStringList appDirs = {
        "/usr/share/applications",
        "/usr/local/share/applications",
        "/var/lib/flatpak/exports/share/applications",
        "/var/lib/snapd/desktop/applications",
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation),
        QDir::homePath() + "/.local/share/applications"
    };

    if (const QString userFlatpak = QDir::homePath() + "/.local/share/flatpak/exports/share/applications"; QDir(userFlatpak).exists()) {
        appDirs.append(userFlatpak);
    }
    
    for (const QString& dirPath : appDirs) {
        QDir dir(dirPath);
        if (!dir.exists()) continue;
        
        QFileInfoList files = dir.entryInfoList({"*.desktop"}, QDir::Files);
        for (const QFileInfo& fileInfo : files) {
            parseDesktopFile(fileInfo.absoluteFilePath());
        }
    }
    
    std::sort(m_applications.begin(), m_applications.end(),
              [](const FeatureItem& a, const FeatureItem& b) {
                  return a.title.toLower() < b.title.toLower();
              });
}

void AppLauncher::parseDesktopFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    
    QTextStream stream(&file);
    QString name, exec, icon, comment;
    bool isApplication = false;
    bool noDisplay = false;
    
    while (!stream.atEnd()) {
        if (QString line = stream.readLine().trimmed(); line.startsWith("Type=")) {
            isApplication = line.split("=").value(1) == "Application";
        } else if (line.startsWith("Name=") && !line.contains("[")) {
            name = line.split("=", Qt::SkipEmptyParts).value(1);
        } else if (line.startsWith("Exec=")) {
            exec = line.split("=", Qt::SkipEmptyParts).value(1);
        } else if (line.startsWith("Icon=")) {
            icon = line.split("=", Qt::SkipEmptyParts).value(1);
        } else if (line.startsWith("Comment=") && !line.contains("[")) {
            comment = line.split("=", Qt::SkipEmptyParts).value(1);
        } else if (line.startsWith("NoDisplay=")) {
            noDisplay = line.split("=").value(1).toLower() == "true";
        }
    }
    
    if (isApplication && !name.isEmpty() && !exec.isEmpty() && !noDisplay) {
        if (m_seenApps.contains(name)) return;
        m_seenApps.insert(name);

        // fuckass regexp
        exec = exec.replace(QRegularExpression("%[fFuUdDnNickvm]"), "").trimmed();
        m_applications.append(FeatureItem(name, comment, icon, exec, "app"));
    }
}

int AppLauncher::fuzzyMatch(const QString& query, const QString& text) {
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