#include "clipboard.h"
#include <QGuiApplication>
#include <QMimeData>
#include <QImage>
#include <QBuffer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QMessageAuthenticationCode>

Clipboard::Clipboard(QObject* parent)
    : QObject(parent),
      FeatureBase(),
      m_clipboard(QGuiApplication::clipboard()),
      m_storageDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.rnux/clipboard"),
      m_encryptionEnabled(true) {
    setup();
    initializeEncryption();
    loadHistory();
}

Clipboard::~Clipboard() {
    saveHistory();
}

void Clipboard::setup() {
    if (!m_storageDir.exists()) {
        m_storageDir.mkpath(".");
    }
    connect(m_clipboard, &QClipboard::dataChanged, this, &Clipboard::onClipboardChanged);
}

void Clipboard::initializeEncryption() {
    const QString keyFilePath = m_storageDir.absoluteFilePath("key.bin");
    if (QFile keyFile(keyFilePath); keyFile.exists() && keyFile.open(QIODevice::ReadOnly)) {
        m_encryptionKey = keyFile.readAll();
        keyFile.close();
    } else {
        const QByteArray salt = QCryptographicHash::hash(
            QByteArray::number(QDateTime::currentMSecsSinceEpoch()) +
            QByteArray::number(QRandomGenerator::global()->generate64()),
            QCryptographicHash::Sha256
        );

        const QByteArray baseData = QGuiApplication::applicationDirPath().toUtf8() +
                             QDir::homePath().toUtf8() +
                             QByteArray::number(QRandomGenerator::global()->generate64());

        m_encryptionKey = deriveKey(baseData, salt);

        if (keyFile.open(QIODevice::WriteOnly)) {
            keyFile.write(m_encryptionKey);
            keyFile.close();
            keyFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        }
    }
}

QByteArray Clipboard::deriveKey(const QByteArray& password, const QByteArray& salt) {
    QByteArray key = password + salt;
    for (int i = 0; i < 10000; ++i) {
        key = QCryptographicHash::hash(key, QCryptographicHash::Sha256);
    }
    return key;
}

QByteArray Clipboard::encrypt(const QString& plainText) const {
    if (!m_encryptionEnabled || plainText.isEmpty()) {
        return plainText.toUtf8();
    }

    QByteArray data = plainText.toUtf8();
    QByteArray result;

    QByteArray iv;
    for (int i = 0; i < 16; ++i) {
        iv.append(static_cast<char>(QRandomGenerator::global()->bounded(256U)));
    }

    QByteArray key = m_encryptionKey;
    while (key.size() < data.size()) {
        key += QCryptographicHash::hash(key, QCryptographicHash::Sha256);
    }

    for (int i = 0; i < data.size(); ++i) {
        const char keyByte = key[i] ^ iv[i % iv.size()];
        result.append(static_cast<char>(static_cast<unsigned char>(data[i]) ^ static_cast<unsigned char>(keyByte)));
    }

    const QByteArray encrypted = iv + result;
    const QByteArray mac = QMessageAuthenticationCode::hash(encrypted, m_encryptionKey, QCryptographicHash::Sha256);

    return mac + encrypted;
}

QString Clipboard::decrypt(const QByteArray& encryptedData) const {
    if (!m_encryptionEnabled || encryptedData.isEmpty()) {
        return QString::fromUtf8(encryptedData);
    }

    if (encryptedData.size() < 48) {
        return QString::fromUtf8(encryptedData);
    }

    const QByteArray mac = encryptedData.left(32);
    const QByteArray encrypted = encryptedData.mid(32);
    if (const QByteArray expectedMac = QMessageAuthenticationCode::hash(encrypted, m_encryptionKey, QCryptographicHash::Sha256); mac != expectedMac) {
        return QString::fromUtf8(encryptedData);
    }

    QByteArray iv = encrypted.left(16);
    QByteArray data = encrypted.mid(16);

    QByteArray key = m_encryptionKey;
    while (key.size() < data.size()) {
        key += QCryptographicHash::hash(key, QCryptographicHash::Sha256);
    }

    QByteArray result;
    for (int i = 0; i < data.size(); ++i) {
        const char keyByte = key[i] ^ iv[i % iv.size()];
        result.append(static_cast<char>(static_cast<unsigned char>(data[i]) ^ static_cast<unsigned char>(keyByte)));
    }

    return QString::fromUtf8(result);
}

QString Clipboard::createPreview(const QString& text) {
    if (text.isEmpty()) {
        return text;
    }
    QString firstLine = text.split('\n', Qt::SkipEmptyParts).first();
    if (firstLine.length() > 50) {
        return firstLine.left(50) + "...";
    }
    return firstLine;
}

void Clipboard::onClipboardChanged() {
    if (const QMimeData* mimeData = m_clipboard->mimeData(); mimeData->hasImage()) {
        const auto image = qvariant_cast<QImage>(mimeData->imageData());
        addImageItem(image);
    } else if (mimeData->hasText()) {
        addTextItem(mimeData->text());
    }
}

QImage Clipboard::createThumbnail(const QString& imagePath, int size) {
    const QImage image(imagePath);
    if (image.isNull()) {
        return {};
    }

    return image.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void Clipboard::addTextItem(const QString& text) {
    if (text.trimmed().isEmpty()) {
        return;
    }

    for(const auto& item : m_history) {
        if(item.type == "text" && item.data == text) {
            return;
        }
    }

    ClipboardItem newItem;
    newItem.type = "text";
    newItem.data = text;
    newItem.preview = createPreview(text);
    newItem.timestamp = QDateTime::currentDateTime();

    m_history.prepend(newItem);

    int textCount = 0;
    for (qsizetype i = m_history.size() - 1; i >= 0; --i) {
        if (m_history[static_cast<int>(i)].type == "text") {
            textCount++;
            if (textCount > 500) {
                m_history.removeAt(static_cast<int>(i));
            }
        }
    }

    saveHistory();
}

void Clipboard::addImageItem(const QImage& image) {
    for(qsizetype i = 0; i < qMin(static_cast<qsizetype>(5), m_history.size()); ++i) {
        if(m_history[static_cast<int>(i)].type == "image") {
            if(QImage recentImage = loadImage(m_history[static_cast<int>(i)].filePath); recentImage == image) {
                return;
            }
        }
    }

    const QString filePath = storeImage(image);
    if (filePath.isEmpty()) {
        return;
    }

    ClipboardItem newItem;
    newItem.type = "image";
    newItem.data = "Image";
    const QString imagePreview = "Image (copied at " + QDateTime::currentDateTime().toString("hh:mm:ss") + ")";
    newItem.preview = imagePreview;
    newItem.timestamp = QDateTime::currentDateTime();
    newItem.filePath = filePath;

    m_history.prepend(newItem);

    int imageCount = 0;
    for (qsizetype i = m_history.size() - 1; i >= 0; --i) {
        if (m_history[static_cast<int>(i)].type == "image") {
            imageCount++;
            // images have a lower history size, mainly to save disk space
            // ill probably add some kind of settings menu later so things like these can be configured
            if (imageCount > 100) {
                QFile::remove(m_history[static_cast<int>(i)].filePath);
                m_history.removeAt(static_cast<int>(i));
            }
        }
    }

    saveHistory();
}

QString Clipboard::storeImage(const QImage& image) const {
    if (image.isNull()) {
        return {};
    }

    const QString fileName = QString::number(QDateTime::currentMSecsSinceEpoch()) + ".png";
    if (const QString filePath = m_storageDir.absoluteFilePath(fileName); image.save(filePath, "PNG")) {
        return filePath;
    }

    return {};
}

QImage Clipboard::loadImage(const QString& filePath) {
    if (filePath.isEmpty()) {
        return {};
    }
    return QImage(filePath);
}

QList<FeatureItem> Clipboard::search(const QString& query) {
    QList<FeatureItem> results;
    const QString lowerQuery = query.toLower();
    const QStringList aliases = {"clipboard ", "clip "};
    QString searchQuery;
    bool aliasFound = false;

    for (const auto& alias : aliases) {
        if (lowerQuery.startsWith(alias)) {
            searchQuery = query.mid(alias.length());
            aliasFound = true;
            break;
        }
    }

    if (!aliasFound && (lowerQuery == "clipboard" || lowerQuery == "clip")) {
        aliasFound = true;
        searchQuery = "";
    }

    if (!aliasFound) {
        return results;
    }

    for (const auto&[data, preview, type, timestamp, filePath] : m_history) {
        if (searchQuery.isEmpty() || preview.toLower().contains(searchQuery)) {
            FeatureItem featureItem;
            featureItem.title = preview;
            featureItem.subtitle = timestamp.toString(Qt::ISODate);

            if (type == "text") {
                featureItem.icon = "text-plain";
            } else if (type == "image") {
                featureItem.icon = filePath;
            }

            featureItem.data = filePath.isEmpty() ? data : filePath;
            featureItem.type = "clipboard";
            results.append(featureItem);
        }
    }

    return results;
}

void Clipboard::execute(const FeatureItem& item) {
    for (const auto& clipboardItem : m_history) {
        if (QString data = clipboardItem.filePath.isEmpty() ? clipboardItem.data : clipboardItem.filePath; data == item.data) {
            if (clipboardItem.type == "text") {
                m_clipboard->setText(clipboardItem.data);
            } else if (clipboardItem.type == "image") {
                if(const QImage image = loadImage(clipboardItem.filePath); !image.isNull()){
                    m_clipboard->setImage(image);
                }
            }
            break;
        }
    }
}

void Clipboard::saveHistory() {
    QJsonArray historyArray;
    for (const auto&[data, preview, type, timestamp, filePath] : m_history) {
        QJsonObject itemObject;
        itemObject["t"] = type;
        itemObject["d"] = QString::fromLatin1(encrypt(data).toBase64());
        itemObject["p"] = QString::fromLatin1(encrypt(preview).toBase64());
        itemObject["ts"] = timestamp.toString(Qt::ISODate);
        if (!filePath.isEmpty()) {
            itemObject["f"] = filePath;
        }
        historyArray.append(itemObject);
    }

    const QJsonDocument doc(historyArray);
    if (QFile file(m_storageDir.absoluteFilePath("history.json")); file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Compact));
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }
}

void Clipboard::loadHistory() {
    QFile file(m_storageDir.absoluteFilePath("history.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    if (const QJsonDocument doc = QJsonDocument::fromJson(file.readAll()); doc.isArray()) {
        QJsonArray historyArray = doc.array();
        for (const auto& itemValue : historyArray) {
            QJsonObject itemObject = itemValue.toObject();
            ClipboardItem item;
            item.type = itemObject["t"].toString();
            item.data = decrypt(QByteArray::fromBase64(itemObject["d"].toString().toLatin1()));
            item.preview = decrypt(QByteArray::fromBase64(itemObject["p"].toString().toLatin1()));
            item.timestamp = QDateTime::fromString(itemObject["ts"].toString(), Qt::ISODate);
            item.filePath = itemObject["f"].toString();
            m_history.append(item);
        }
    }
}