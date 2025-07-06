#include "windowui.h"
#include <QPainter>
#include <QApplication>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QItemSelectionModel>
#include <QScrollBar>
#include <QEasingCurve>
#include <QGraphicsPixmapItem>
#include <QShowEvent>
#include <QIcon>
#include <QPixmap>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <random>

ModernItemDelegate::ModernItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
    , m_accentColor(QColor("#A22633"))
    , m_backgroundColor(QColor("#000000"))
    , m_hoverColor(QColor("#FFFFFF"))
    , m_selectedColor(QColor("#A22633"))
    , m_textColor(QColor("#FFFFFF"))
    , m_subtitleColor(QColor("#9CA3AF"))
{
    m_titleFont = QFont("SF Pro Display", 15, QFont::Medium);
    m_subtitleFont = QFont("SF Pro Display", 13, QFont::Normal);

    m_titleFont.setFamilies({"SF Pro Display", "Segoe UI Variable", "Segoe UI", "Helvetica Neue", "Arial"});
    m_subtitleFont.setFamilies({"SF Pro Display", "Segoe UI Variable", "Segoe UI", "Helvetica Neue", "Arial"});
}

QIcon ModernItemDelegate::loadIcon(const QString& iconName) {
    if (iconName.isEmpty()) {
        return {};
    }

    if (QFileInfo(iconName).isAbsolute() && QFileInfo::exists(iconName)) {
        return QIcon(iconName);
    }

    if (QIcon::hasThemeIcon(iconName)) {
        return QIcon::fromTheme(iconName);
    }

    QStringList iconDirs = {
        "/usr/share/icons",
        "/usr/share/pixmaps",
        "/usr/local/share/icons",
        "/usr/local/share/pixmaps",
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/icons"
    };

    QStringList iconExtensions = {"png", "svg", "xpm", "ico"};
    QStringList iconSizes = {"24", "32", "48", "64", "128", "256"};

    for (const QString& dir : iconDirs) {
        for (const QString& ext : iconExtensions) {
            if (QString fullPath = QString("%1/%2.%3").arg(dir, iconName, ext); QFileInfo::exists(fullPath)) {
                return QIcon(fullPath);
            }
        }

        for (const QString& size : iconSizes) {
            for (const QString& ext : iconExtensions) {
                if (QString fullPath = QString("%1/hicolor/%2x%2/apps/%3.%4").arg(dir, size, iconName, ext); QFileInfo::exists(fullPath)) {
                    return QIcon(fullPath);
                }
            }
        }

        if (QDir iconDir(dir); iconDir.exists()) {
            QStringList themeDirs = iconDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString& theme : themeDirs) {
                for (const QString& size : iconSizes) {
                    for (const QString& ext : iconExtensions) {
                        if (QString fullPath = QString("%1/%2/%3x%3/apps/%4.%5").arg(dir, theme, size, iconName, ext); QFileInfo::exists(fullPath)) {
                            return QIcon(fullPath);
                        }
                    }
                }
            }
        }
    }

    return {};
}

void ModernItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    QRect rect = option.rect;
    bool isSelected = option.state & QStyle::State_Selected;

    if (bool isHovered = option.state & QStyle::State_MouseOver; isSelected || isHovered) {
        QPainterPath path;
        path.addRoundedRect(rect.adjusted(8, 4, -8, -4), 8, 8);

        if (isSelected) {
            QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
            gradient.setColorAt(0, m_selectedColor.lighter(120));
            gradient.setColorAt(0.5, m_selectedColor);
            gradient.setColorAt(1, m_selectedColor.darker(110));

            painter->fillPath(path, gradient);

            painter->setPen(QPen(QColor(255, 255, 255, 60), 1));
            painter->drawPath(path);
        } else if (isHovered) {
            auto hoverBg = QColor(255, 255, 255, 15);
            painter->fillPath(path, hoverBg);
            painter->setPen(QPen(QColor(255, 255, 255, 30), 1));
            painter->drawPath(path);
        }
    }

    QString title = index.data(Qt::DisplayRole).toString();
    QString subtitle = index.data(Qt::UserRole).toString();
    QString iconName = index.data(Qt::UserRole + 1).toString();

    // icon
    QRect iconRect(rect.left() + 20, rect.center().y() - 12, 24, 24);
    if (!iconName.isEmpty()) {
        if (QIcon icon = loadIcon(iconName); !icon.isNull()) {
            if (QPixmap pixmap = icon.pixmap(24, 24); !pixmap.isNull()) {
                painter->drawPixmap(iconRect, pixmap);
            } else {
                painter->setPen(QPen(isSelected ? QColor("#FFFFFF") : QColor("#6B7280"), 2));
                painter->setBrush(Qt::NoBrush);
                painter->drawRoundedRect(iconRect, 6, 6);
            }
        } else {
            painter->setPen(QPen(isSelected ? QColor("#FFFFFF") : QColor("#6B7280"), 2));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(iconRect, 6, 6);
        }
    } else {
        painter->setPen(QPen(isSelected ? QColor("#FFFFFF") : QColor("#6B7280"), 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(iconRect, 6, 6);
    }

    // app name/title
    painter->setPen(isSelected ? QColor("#FFFFFF") : m_textColor);
    painter->setFont(m_titleFont);

    QRect titleRect = rect.adjusted(56, 12, -60, -28);
    QFontMetrics titleMetrics(m_titleFont);
    QString elidedTitle = titleMetrics.elidedText(title, Qt::ElideRight, titleRect.width());
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, elidedTitle);

    // sub app desc/subtitle
    if (!subtitle.isEmpty()) {
        painter->setPen(isSelected ? QColor("#E5E7EB") : m_subtitleColor);
        painter->setFont(m_subtitleFont);

        QRect subtitleRect = rect.adjusted(56, 32, -60, -8);
        QFontMetrics subtitleMetrics(m_subtitleFont);
        QString elidedSubtitle = subtitleMetrics.elidedText(subtitle, Qt::ElideRight, subtitleRect.width());
        painter->drawText(subtitleRect, Qt::AlignLeft | Qt::AlignVCenter, elidedSubtitle);
    }

    if (isSelected) {
        painter->setPen(QColor("#FFFFFF"));
        painter->setFont(QFont("SF Pro Display", 11, QFont::Medium));

        QRect shortcutRect(rect.right() - 50, rect.center().y() - 8, 40, 16);
        painter->drawText(shortcutRect, Qt::AlignCenter, "⏎");
    }

    painter->restore();
}

QSize ModernItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_UNUSED(option)
    Q_UNUSED(index)
    return QSize(0, WindowUI::ITEM_HEIGHT);
}

WindowUI::WindowUI(QWidget* parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_searchFrame(nullptr)
    , m_searchLayout(nullptr)
    , m_searchEdit(nullptr)
    , m_searchIcon(nullptr)
    , m_listView(nullptr)
    , m_emptyLabel(nullptr)
    , m_model(nullptr)
    , m_delegate(nullptr)
    , m_separator(nullptr)
    , m_heightAnimation(nullptr)
    , m_showAnimation(nullptr)
    , m_opacityEffect(nullptr)
    , m_blurAnimation(nullptr)
    , m_currentHeight(SEARCH_HEIGHT)
    , m_hasResults(false)
    , m_backgroundOpacity(0.0)
    , m_blurRadius(0.0)
{
    setupUI();
    setupStyles();
    updateHeight();
}

WindowUI::~WindowUI() {
    // qt is clingy as shit and doesnt like it when you delete qt objects manually
}

void WindowUI::setupUI() {
    setFixedWidth(WINDOW_WIDTH);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // search
    m_searchFrame = new QFrame(this);
    m_searchFrame->setFixedHeight(SEARCH_HEIGHT);
    m_searchLayout = new QHBoxLayout(m_searchFrame);
    m_searchLayout->setContentsMargins(24, 0, 24, 0);
    m_searchLayout->setSpacing(16);
    m_searchIcon = new QLabel(this);
    m_searchIcon->setText("⌕");
    m_searchIcon->setFixedSize(24, 24);
    m_searchIcon->setAlignment(Qt::AlignCenter);
    // search input
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search applications, calculate, run commands...");
    m_searchEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_searchLayout->addWidget(m_searchIcon);
    m_searchLayout->addWidget(m_searchEdit);

    m_separator = new QFrame(this);
    m_separator->setFrameShape(QFrame::HLine);
    m_separator->setFixedHeight(1);

    // results
    m_listView = new QListView(this);
    m_model = new QStandardItemModel(this);
    m_delegate = new ModernItemDelegate(this);
    m_listView->setModel(m_model);
    m_listView->setItemDelegate(m_delegate);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setFocusPolicy(Qt::NoFocus);
    m_listView->setMouseTracking(true);

    // womp womp
    m_emptyLabel = new QLabel(this);
    m_emptyLabel->setText("No results found");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setVisible(false);

    m_mainLayout->addWidget(m_searchFrame);
    m_mainLayout->addWidget(m_separator);
    m_mainLayout->addWidget(m_listView);
    m_mainLayout->addWidget(m_emptyLabel);

    m_heightAnimation = new QPropertyAnimation(this, "minimumHeight", this);
    m_heightAnimation->setDuration(300);
    m_heightAnimation->setEasingCurve(QEasingCurve::OutExpo);

    m_showAnimation = new QParallelAnimationGroup(this);

    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(0.0);
    setGraphicsEffect(m_opacityEffect);

    m_blurAnimation = new QVariantAnimation(this);
    m_blurAnimation->setDuration(500);
    m_blurAnimation->setStartValue(0.0);
    m_blurAnimation->setEndValue(1.0);
    m_blurAnimation->setEasingCurve(QEasingCurve::OutQuart);
    connect(m_blurAnimation, &QVariantAnimation::valueChanged, [this](const QVariant& value) {
        m_backgroundOpacity = value.toReal();
        update();
    });

    connect(m_searchEdit, &QLineEdit::textChanged, this, &WindowUI::onTextChanged);
    connect(m_listView, &QListView::activated, this, &WindowUI::onItemActivated);
    connect(m_listView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &WindowUI::onSelectionChanged);
}

void WindowUI::setupStyles() const {
    m_searchFrame->setStyleSheet(
        "QFrame {"
        "    background-color: transparent;"
        "    border: none;"
        "}"
    );

    m_searchEdit->setStyleSheet(
        "QLineEdit {"
        "    border: none;"
        "    background-color: transparent;"
        "    color: #FFFFFF;"
        "    font-family: 'SF Pro Display', 'Segoe UI Variable', 'Segoe UI', 'Helvetica Neue', Arial;"
        "    font-size: 18px;"
        "    font-weight: 500;"
        "    padding: 0;"
        "    selection-background-color: rgba(162, 38, 51, 0.3);"
        "}"
        "QLineEdit::placeholder {"
        "    color: #6B7280;"
        "    font-weight: 400;"
        "}"
    );

    m_searchIcon->setStyleSheet(
        "QLabel {"
        "    color: #9CA3AF;"
        "    font-size: 18px;"
        "    font-weight: 300;"
        "    background-color: transparent;"
        "}"
    );

    m_separator->setStyleSheet(
        "QFrame {"
        "    border: none;"
        "    background-color: rgba(255, 255, 255, 0.08);"
        "    margin: 0px 16px;"
        "}"
    );

    m_listView->setStyleSheet(
        "QListView {"
        "    border: none;"
        "    background-color: transparent;"
        "    selection-background-color: transparent;"
        "    outline: none;"
        "    padding: 8px 0px;"
        "}"
    );

    m_emptyLabel->setStyleSheet(
        "QLabel {"
        "    color: #6B7280;"
        "    font-family: 'SF Pro Display', 'Segoe UI Variable', 'Segoe UI', 'Helvetica Neue', Arial;"
        "    font-size: 15px;"
        "    font-weight: 400;"
        "    background-color: transparent;"
        "    padding: 48px 24px;"
        "}"
    );
}

void WindowUI::setResults(const QList<FeatureItem>& results) {
    m_currentResults = results;
    m_hasResults = !results.isEmpty();

    m_model->clear();
    for (const auto& item : results) {
        const auto modelItem = new QStandardItem(item.title);
        modelItem->setData(item.subtitle, Qt::UserRole);
        modelItem->setData(item.icon, Qt::UserRole + 1);
        modelItem->setFlags(modelItem->flags() & ~Qt::ItemIsEditable);
        m_model->appendRow(modelItem);
    }

    if (m_model->rowCount() > 0) {
        m_listView->setCurrentIndex(m_model->index(0, 0));
    }

    updateHeight();
    updateEmptyState();
}

void WindowUI::setQuery(const QString& query) {
    m_currentQuery = query;
    if (m_searchEdit->text() != query) {
        m_searchEdit->setText(query);
    }
}

void WindowUI::selectNextItem() const {
    if (m_model->rowCount() > 0) {
        const int currentRow = m_listView->currentIndex().row();
        const int nextRow = (currentRow + 1) % m_model->rowCount();
        m_listView->setCurrentIndex(m_model->index(nextRow, 0));
    }
}

void WindowUI::selectPreviousItem() const {
    if (m_model->rowCount() > 0) {
        const int currentRow = m_listView->currentIndex().row();
        const int prevRow = (currentRow - 1 + m_model->rowCount()) % m_model->rowCount();
        m_listView->setCurrentIndex(m_model->index(prevRow, 0));
    }
}

void WindowUI::activateCurrentItem() {
    if (m_listView->currentIndex().isValid()) {
        onItemActivated(m_listView->currentIndex());
    }
}

int WindowUI::getCurrentIndex() const {
    return m_listView->currentIndex().row();
}

bool WindowUI::hasResults() const {
    return m_hasResults;
}

void WindowUI::updateHeight() {
    const int itemCount = std::min(m_model->rowCount(), MAX_VISIBLE_ITEMS);
    int newHeight = SEARCH_HEIGHT + 1;

    if (itemCount > 0) {
        newHeight += itemCount * ITEM_HEIGHT + 16;
    } else if (!m_currentQuery.isEmpty()) {
        newHeight += 96;
    }

    if (newHeight != m_currentHeight) {
        animateHeight(newHeight);
        m_currentHeight = newHeight;
    }
}

void WindowUI::animateHeight(const int newHeight) {
    m_heightAnimation->setStartValue(height());
    m_heightAnimation->setEndValue(newHeight);
    m_heightAnimation->start();

    setFixedHeight(newHeight);
}

void WindowUI::updateEmptyState() const {
    const bool showEmpty = !m_currentQuery.isEmpty() && m_model->rowCount() == 0;
    m_emptyLabel->setVisible(showEmpty);
    m_listView->setVisible(!showEmpty);
    m_separator->setVisible(!showEmpty || m_model->rowCount() > 0);
}

void WindowUI::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    animateIn();
}

void WindowUI::animateIn() const {
    m_showAnimation->start();
    m_blurAnimation->start();
}

void WindowUI::drawBlurredBackground(QPainter* painter, const QRect& rect) const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 1.0);
    painter->save();
    const auto baseColor = QColor(15, 15, 15, static_cast<int>(235 * m_backgroundOpacity));
    painter->fillRect(rect, baseColor);

    for (int i = 0; i < rect.width(); i += 4) {
        for (int j = 0; j < rect.height(); j += 4) {
            if (dis(gen) > 0.98) {
                painter->setPen(QColor(255, 255, 255, static_cast<int>(8 * m_backgroundOpacity)));
                painter->drawPoint(i, j);
            }
        }
    }

    painter->restore();
}

void WindowUI::drawGlassEffect(QPainter* painter, const QRect& rect) const {
    painter->save();

    QLinearGradient highlight(rect.topLeft(), QPoint(rect.left(), rect.top() + 100));
    highlight.setColorAt(0, QColor(255, 255, 255, static_cast<int>(25 * m_backgroundOpacity)));
    highlight.setColorAt(0.5, QColor(255, 255, 255, static_cast<int>(10 * m_backgroundOpacity)));
    highlight.setColorAt(1, QColor(255, 255, 255, 0));

    painter->fillRect(rect, highlight);
    painter->restore();
}

void WindowUI::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QPainterPath path;
    path.addRoundedRect(rect(), BORDER_RADIUS, BORDER_RADIUS);
    painter.setClipPath(path);

    drawBlurredBackground(&painter, rect());
    drawGlassEffect(&painter, rect());

    painter.setPen(QPen(QColor(255, 255, 255, static_cast<int>(40 * m_backgroundOpacity)), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    QWidget::paintEvent(event);
}

void WindowUI::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(SHADOW_BLUR);
    shadow->setColor(QColor(0, 0, 0, 120));
    shadow->setOffset(0, 12);
    setGraphicsEffect(shadow);
}

void WindowUI::onTextChanged(const QString& text) {
    m_currentQuery = text;
    emit queryChanged(text);
}

void WindowUI::onItemActivated(const QModelIndex& index) {
    if (index.isValid()) {
        emit itemActivated(index.row());
    }
}

void WindowUI::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
    Q_UNUSED(selected)
    Q_UNUSED(deselected)
    update();
}