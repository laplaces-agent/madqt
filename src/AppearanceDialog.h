#pragma once

#include "MarkdownAppearance.h"
#include "Theme.h"

#include <QDialog>

class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QWebEngineView;

class AppearanceDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit AppearanceDialog(const MarkdownAppearance &appearance, QWidget *parent = nullptr);
    explicit AppearanceDialog(const Theme &theme, QWidget *parent = nullptr);

    [[nodiscard]] Theme editedTheme() const;

private:
    void chooseBodyFont();
    void chooseColor(QColor &targetColor, QPushButton *button);
    void chooseMonospaceFont();
    void createLayout();
    void loadAppearanceIntoControls();
    void resetDefaults();
    void syncAppearanceFromControls();
    void updateFontSummary(QLabel *label, const QString &family, int pointSize, const QFont &sampleFont);
    void updateButtonSwatch(QPushButton *button, const QColor &color);
    void updatePreview();

    MarkdownAppearance appearance;
    QString themeName;
    QLabel *bodyFontLabel {};
    QLabel *monospaceFontLabel {};
    QLineEdit *nameEdit {};
    QPushButton *bodyFontButton {};
    QPushButton *monospaceFontButton {};
    QDoubleSpinBox *lineHeightSpinBox {};
    QSpinBox *contentPaddingSpinBox {};
    QSpinBox *contentMaxWidthSpinBox {};
    QPushButton *bodyTextColorButton {};
    QPushButton *pageBackgroundColorButton {};
    QPushButton *headingColorButton {};
    QPushButton *linkColorButton {};
    QPushButton *mutedTextColorButton {};
    QPushButton *codeTextColorButton {};
    QPushButton *codeBackgroundColorButton {};
    QPushButton *panelBackgroundColorButton {};
    QPushButton *borderColorButton {};
    QPushButton *blockquoteBorderColorButton {};
    QWebEngineView *preview {};
};
