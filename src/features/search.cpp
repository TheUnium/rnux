#include "search.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QMutexLocker>
#include <QDebug>

Search::Search(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_searchTimer(new QTimer(this))
{
    setupProviders();
    initializeCache();
    downloadProviderIcons();

    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(200);
    connect(m_searchTimer, &QTimer::timeout, this, &Search::onSearchTimeout);
}

Search::~Search() {
    if (m_currentReply) {
        m_currentReply->abort();
    }
    saveCache();
}

void Search::setupProviders() {
    m_providers = {
        // general search engines
        SearchProvider("Google", "g", "https://www.google.com/favicon.ico", // y
                      "https://www.google.com/search?q=%1",
                      "Search Google"),

        SearchProvider("DuckDuckGo", "ddg", "https://duckduckgo.com/favicon.ico", // y
                      "https://duckduckgo.com/?q=%1",
                      "Search DuckDuckGo"),

        SearchProvider("Bing", "bing", "https://www.bing.com/favicon.ico", // y
                      "https://www.bing.com/search?q=%1",
                      "Search Bing"),

        SearchProvider("Yandex", "yandex", "https://yandex.com/favicon.ico", // y
                      "https://yandex.com/search/?text=%1",
                      "Search Yandex"),

        // thingies
        SearchProvider("NPM", "npm", "https://theunium.github.io/extra/npm.ico", // n y
                      "https://www.npmjs.com/search?q=%1",
                      "Search NPM packages",
                      "https://registry.npmjs.org/-/v1/search?text=%1&size=10", true, "npm"),

        SearchProvider("Cargo", "cargo", "https://crates.io/favicon.ico", // y
                      "https://crates.io/search?q=%1",
                      "Search Rust crates",
                      "https://crates.io/api/v1/crates?q=%1&per_page=10", true, "cargo"),

        SearchProvider("GitHub", "gh", "https://github.com/favicon.ico", // y
                      "https://github.com/search?q=%1",
                      "Search GitHub repositories",
                      "https://api.github.com/search/repositories?q=%1&sort=stars&order=desc&per_page=10", true, "github"),

        SearchProvider("PyPI", "pypi", "https://pypi.org/favicon.ico", // y
                      "https://pypi.org/search/?q=%1",
                      "Search Python packages"),

        SearchProvider("Docker Hub", "docker", "https://hub.docker.com/favicon.ico", // y
                      "https://hub.docker.com/search?q=%1",
                      "Search Docker images"),

        // docs
        SearchProvider("MDN", "mdn", "https://developer.mozilla.org/favicon.ico", // y
                      "https://developer.mozilla.org/en-US/search?q=%1",
                      "Search MDN Web Docs"),

        SearchProvider("Stack Overflow", "so", "https://stackoverflow.com/favicon.ico", // y
                      "https://stackoverflow.com/search?q=%1",
                      "Search Stack Overflow"),

        SearchProvider("Reddit", "reddit", "https://www.reddit.com/favicon.ico", // y
                      "https://www.reddit.com/search/?q=%1",
                      "Search Reddit"),

        SearchProvider("YouTube", "yt", "https://www.youtube.com/favicon.ico", // y
                      "https://www.youtube.com/results?search_query=%1",
                      "Search YouTube"),

        SearchProvider("Wikipedia", "wiki", "https://en.wikipedia.org/favicon.ico", // y
                      "https://en.wikipedia.org/wiki/Special:Search/%1",
                      "Search Wikipedia"),

        SearchProvider("Arch AUR", "aur", "https://aur.archlinux.org/favicon.ico", // y
                      "https://aur.archlinux.org/packages/?K=%1",
                      "Search Arch User Repository"),

        SearchProvider("Debian Packages", "deb", "https://packages.debian.org/favicon.ico", // y
                      "https://packages.debian.org/search?keywords=%1",
                      "Search Debian packages"),

        SearchProvider("Homebrew", "brew", "https://brew.sh/assets/img/favicon.ico", // n y
                      "https://formulae.brew.sh/formula/%1",
                      "Search Homebrew packages"),

        // social media
        SearchProvider("Twitter", "tw", "https://twitter.com/favicon.ico", // y
                      "https://twitter.com/search?q=%1",
                      "Search Twitter"),
    };
}

void Search::downloadProviderIcons() {
    const QString iconDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.rnux/icons";
    if (!QDir().exists(iconDir)) {
        QDir().mkpath(iconDir);
    }

    for (const auto& provider : m_providers) {
        const QString iconPath = iconDir + "/" + provider.shortcut + ".ico";
        m_iconPaths[provider.shortcut] = iconPath;

        if (QFile::exists(iconPath)) {
            QPixmap icon(iconPath);
            if (!icon.isNull()) {
                m_iconCache[provider.shortcut] = icon;
                continue;
            }
        }

        QNetworkRequest request(QUrl(provider.iconUrl));
        request.setRawHeader("User-Agent", "rnux-app-launcher/1.0");
        QNetworkReply* reply = m_networkManager->get(request);
        reply->setProperty("shortcut", provider.shortcut);
        connect(reply, &QNetworkReply::finished, this, &Search::onIconDownloaded);
    }
}

void Search::onIconDownloaded() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    const QString shortcut = reply->property("shortcut").toString();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QPixmap icon;
        if (icon.loadFromData(data)) {
            m_iconCache[shortcut] = icon;

            if (m_iconPaths.contains(shortcut)) {
                QFile file(m_iconPaths[shortcut]);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(data);
                }
            }
        }
    }

    reply->deleteLater();
}

QList<FeatureItem> Search::search(const QString& query) {
    QList<FeatureItem> results;
    if (query.isEmpty()) return results;

    for (const auto& provider : m_providers) {
        if (QString pattern = provider.shortcut + " "; query.startsWith(pattern, Qt::CaseInsensitive)) {
            if (QString searchQuery = extractSearchQuery(query, provider.shortcut); searchQuery.isEmpty()) {
                const QString iconPath = m_iconPaths.value(provider.shortcut, "system-search");
                results.append({ provider.name, "", iconPath,
                                 provider.searchUrl.arg(""), "search" });
            } else {
                const QString iconPath = m_iconPaths.value(provider.shortcut, "system-search");
                results.append({ QString("Search %1: %2").arg(provider.name, searchQuery),
                                 "", iconPath,
                                 provider.searchUrl.arg(QString(QUrl::toPercentEncoding(searchQuery))),
                                 "search" });

                // check cache for the thingies that fetch the things from the thingies api
                if (provider.hasApi && !provider.cacheType.isEmpty()) {
                    if (const QList<FeatureItem> cachedResults = getCachedResults(provider.cacheType, searchQuery); !cachedResults.isEmpty()) {
                        results.append(cachedResults);
                        return results;
                    }

                    // trigger search through api
                    if (m_currentQuery != query) {
                        m_currentQuery = query;
                        m_activeSearchType = provider.cacheType;
                        m_activeSearchQuery = searchQuery;
                        m_searchTimer->start();
                    }
                }
            }
            break;
        }
    }
    return results;
}

void Search::execute(const FeatureItem& item) {
    if (item.type == "search") {
        QDesktopServices::openUrl(QUrl(item.data));
    }
}

QString Search::extractSearchQuery(const QString& fullQuery, const QString& shortcut) {
    if (const QString pattern = shortcut + " "; fullQuery.startsWith(pattern, Qt::CaseInsensitive)) {
        return fullQuery.mid(pattern.length()).trimmed();
    }
    return {};
}

void Search::onSearchTimeout() {
    for (const auto& provider : m_providers) {
        if (QString pattern = provider.shortcut + " "; m_currentQuery.startsWith(pattern, Qt::CaseInsensitive)) {
            if (const QString q = extractSearchQuery(m_currentQuery, provider.shortcut); !q.isEmpty() && provider.hasApi) {
                performApiSearch(provider, q);
            }
            break;
        }
    }
}

void Search::performApiSearch(const SearchProvider& provider, const QString& query) {
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply = nullptr;
    }

    const QString apiUrl = provider.apiUrl.arg(QString(QUrl::toPercentEncoding(query)));
    QNetworkRequest request{ QUrl(apiUrl) };
    request.setRawHeader("User-Agent", "rnux-app-launcher/1.0");

    if (provider.shortcut == "gh")
        request.setRawHeader("Accept", "application/vnd.github.v3+json");

    m_currentReply = m_networkManager->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &Search::onApiResponse);
}

void Search::onApiResponse() {
    if (!m_currentReply) return;
    QNetworkReply* reply = m_currentReply;
    m_currentReply = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QList<FeatureItem> results;

        if (const QString urlStr = reply->url().toString(); urlStr.contains("registry.npmjs.org")) {
            results = parseNpmResults(doc, m_activeSearchQuery);
            if (!results.isEmpty()) {
                setCachedResults("npm", m_activeSearchQuery, results);
            }
        } else if (urlStr.contains("crates.io/api")) {
            results = parseCargoResults(doc, m_activeSearchQuery);
            if (!results.isEmpty()) {
                setCachedResults("cargo", m_activeSearchQuery, results);
            }
        } else if (urlStr.contains("api.github.com")) {
            results = parseGitHubResults(doc, m_activeSearchQuery);
            if (!results.isEmpty()) {
                setCachedResults("github", m_activeSearchQuery, results);
            }
        }

        m_currentResults = results;
        emit resultsUpdated();
    }
    reply->deleteLater();
}

QList<FeatureItem> Search::parseNpmResults(const QJsonDocument& doc, const QString&) {
    QList<FeatureItem> results;
    if (!doc.isObject()) return results;

    auto packages = doc.object()["objects"].toArray();
    for (auto item : packages) {
        auto itemObj = item.toObject();
        // i hate the fact that npm cant just provide the total downloads
        // or hell, even if you ONLY want to show monthly/weekly, why not move it under the package obj?
        // why keep it in the root obj?
        const int downloads = itemObj["downloads"].toObject()["monthly"].toInt();

        auto pkg = itemObj["package"].toObject();
        QString name = pkg["name"].toString();
        QString version = pkg["version"].toString();

        QString title = QString("%1 v%2 • %3 downloads").arg(name, version, QString::number(downloads));

        results.append(createFeatureItem(
            title,
            QString("https://www.npmjs.com/package/%1").arg(name)
        ));
    }
    return results;
}

QList<FeatureItem> Search::parseCargoResults(const QJsonDocument& doc, const QString&) {
    QList<FeatureItem> results;
    if (!doc.isObject()) return results;

    auto crates = doc.object()["crates"].toArray();
    for (auto item : crates) {
        auto crate = item.toObject();
        QString name = crate["name"].toString();
        QString version = crate["max_version"].toString();
        const int downloads = crate["downloads"].toInt();

        QString title = QString("%1 v%2 • %3 downloads").arg(name, version, QString::number(downloads));

        results.append(createFeatureItem(
            title,
            QString("https://crates.io/crates/%1").arg(name)
        ));
    }
    return results;
}

QList<FeatureItem> Search::parseGitHubResults(const QJsonDocument& doc, const QString&) {
    QList<FeatureItem> results;
    if (!doc.isObject()) return results;

    auto repos = doc.object()["items"].toArray();
    for (auto item : repos) {
        auto repo = item.toObject();
        QString name = repo["full_name"].toString();
        QString url = repo["html_url"].toString();
        const int stars = repo["stargazers_count"].toInt();

        QString title = QString("%1 ⭐ %2").arg(name, QString::number(stars));

        results.append(createFeatureItem(
            title,
            url
        ));
    }
    return results;
}

// cache impl
void Search::initializeCache() {
    loadCache();
}

void Search::saveCache() {
    QMutexLocker locker(&m_cacheMutex);

    if (const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.rnux/cache"; !QDir().exists(cacheDir)) {
        QDir().mkpath(cacheDir);
    }

    const QString cacheFile = getCacheFilePath();
    QFile file(cacheFile);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open cache file for writing:" << cacheFile;
        return;
    }

    QJsonObject cacheObj;
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        const CacheEntry& entry = it.value();

        QJsonArray compact;
        compact.append(entry.type);
        compact.append(entry.query);
        compact.append(entry.lastModified.toSecsSinceEpoch());

        QJsonArray items;
        for (const auto& result : entry.results) {
            QJsonArray item;
            item.append(result.title);
            item.append("");
            item.append(result.data);
            items.append(item);
        }

        compact.append(items);
        cacheObj[it.key()] = compact;
    }

    const QJsonDocument doc(cacheObj);
    file.write(doc.toJson(QJsonDocument::Compact));
}

void Search::loadCache() {
    QMutexLocker locker(&m_cacheMutex);
    const QString cacheFile = getCacheFilePath();
    QFile file(cacheFile);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return;
    }

    QJsonObject cacheObj = doc.object();
    for (auto it = cacheObj.begin(); it != cacheObj.end(); ++it) {
        QString cacheKey = it.key();

        if (QJsonArray compact = it.value().toArray(); compact.size() >= 4) {
            CacheEntry entry;
            entry.type = compact[0].toString();
            entry.query = compact[1].toString();
            entry.lastModified = QDateTime::fromSecsSinceEpoch(compact[2].toInt());

            QJsonArray items = compact[3].toArray();
            for (const auto& itemValue : items) {
                QJsonArray item = itemValue.toArray();
                if (item.size() >= 3) {
                    entry.results.append(createFeatureItem(
                        item[0].toString(),
                        item[2].toString()
                    ));
                }
            }
            m_cache[cacheKey] = entry;
        }
    }
    cleanupCache();
}

QList<FeatureItem> Search::getCachedResults(const QString& type, const QString& query) {
    QMutexLocker locker(&m_cacheMutex);
    if (const QString cacheKey = QString("%1:%2").arg(type, query); m_cache.contains(cacheKey)) {
        if (const CacheEntry& entry = m_cache[cacheKey]; entry.lastModified.secsTo(QDateTime::currentDateTime()) < CACHE_EXPIRE_HOURS * 3600) {
            return entry.results;
        } else {
            m_cache.remove(cacheKey);
        }
    }

    return {};
}

void Search::setCachedResults(const QString& type, const QString& query, const QList<FeatureItem>& results) {
    QMutexLocker locker(&m_cacheMutex);
    const QString cacheKey = QString("%1:%2").arg(type, query);
    const CacheEntry entry(type, query, QDateTime::currentDateTime(), results);
    m_cache[cacheKey] = entry;
    if (m_cache.size() > MAX_CACHE_ENTRIES) {
        cleanupCache();
    }

    QTimer::singleShot(0, this, &Search::saveCache);
}

void Search::cleanupCache() {
    if (m_cache.size() <= MAX_CACHE_ENTRIES) {
        return;
    }

    QDateTime oldest = QDateTime::currentDateTime();
    QString oldestKey;

    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it.value().lastModified < oldest) {
            oldest = it.value().lastModified;
            oldestKey = it.key();
        }
    }

    if (!oldestKey.isEmpty()) {
        m_cache.remove(oldestKey);
    }
}

QString Search::getCacheFilePath() {
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.rnux/cache";
    return cacheDir + "/search.json";
}

FeatureItem Search::createFeatureItem(const QString& name, const QString& url) {
    return FeatureItem{ name, "", "applications-development", url, "search" };
}