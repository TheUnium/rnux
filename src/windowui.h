#pragma once

#include <QLineEdit>
#include <QHBoxLayout>
#include <QListView>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QFrame>
#include <QParallelAnimationGroup>
#include <QVariantAnimation>
#include <QIcon>

#include "features/feature_base.h"

class ModernItemDelegate final : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit ModernItemDelegate(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    void paintDefaultItem(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void paintTimeItem(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    static QIcon loadIcon(const QString& iconName);

    QFont m_titleFont;
    QFont m_subtitleFont;
    QFont m_timeCityFont;
    QFont m_timeTimeFont;
    QFont m_timeTZFont;
    QFont m_timeDiffFont;
    QColor m_accentColor;
    QColor m_backgroundColor;
    QColor m_hoverColor;
    QColor m_selectedColor;
    QColor m_textColor;
    QColor m_subtitleColor;

    mutable QHash<QModelIndex, qreal> m_hoverOpacity;
    mutable QHash<QModelIndex, qreal> m_selectionOpacity;
};

class WindowUI final : public QWidget {
    Q_OBJECT

public:
    explicit WindowUI(QWidget* parent = nullptr);
    ~WindowUI() override;

    void setResults(const QList<FeatureItem>& results);
    void setQuery(const QString& query);
    void selectNextItem() const;
    void selectPreviousItem() const;
    void activateCurrentItem();
    [[nodiscard]] int getCurrentIndex() const;
    [[nodiscard]] bool hasResults() const;

    [[nodiscard]] QLineEdit* getSearchEdit() const { return m_searchEdit; }

    static constexpr int ITEM_HEIGHT = 40;
    static constexpr int TIME_ITEM_HEIGHT = 130;
    static constexpr int SEARCH_HEIGHT = 50;
    static constexpr int MAX_VISIBLE_ITEMS = 8;
    static constexpr int WINDOW_WIDTH = 680;
    static constexpr int BORDER_RADIUS = 16;
    static constexpr int SHADOW_BLUR = 32;

signals:
    void itemActivated(int index);
    void queryChanged(const QString& query);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onTextChanged(const QString& text);
    void onItemActivated(const QModelIndex& index);
    void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private:
    void setupUI();
    void setupStyles() const;
    void updateHeight();
    void animateHeight(int newHeight);
    void updateEmptyState() const;
    void animateIn() const;
    void drawBlurredBackground(QPainter* painter, const QRect& rect) const;
    void drawGlassEffect(QPainter* painter, const QRect& rect) const;

    QVBoxLayout* m_mainLayout;
    QFrame* m_searchFrame;
    QHBoxLayout* m_searchLayout;
    QLineEdit* m_searchEdit;
    QLabel* m_resultsLabel;
    QListView* m_listView;
    QLabel* m_emptyLabel;
    QStandardItemModel* m_model;
    ModernItemDelegate* m_delegate;
    QFrame* m_separator;

    QPropertyAnimation* m_heightAnimation;
    QParallelAnimationGroup* m_showAnimation;
    QGraphicsOpacityEffect* m_opacityEffect;
    QVariantAnimation* m_blurAnimation;

    QList<FeatureItem> m_currentResults;
    QString m_currentQuery;
    int m_currentHeight;
    bool m_hasResults;
    qreal m_backgroundOpacity;
    qreal m_blurRadius;
};