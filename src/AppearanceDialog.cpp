#include "AppearanceDialog.h"
#include "LinkPolicyWebPage.h"

#include <QColorDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFontDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWebEngineView>

namespace
{
QString previewMarkdown()
{
    return QStringLiteral(R"(
# Preview Heading

This is a sample paragraph with **bold text**, *italics*, inline `code`, and a [link](https://example.com).

> A blockquote shows muted text and border styling.

## Lists and Code

- First item with ~~strikethrough~~
- Second item
- [x] A completed task
- [ ] An open task

```cpp
std::string_view title = "madqt";
```

| Name | Value |
| --- | ---: |
| Font size | 15 |
| Line height | 1.55 |
)");
}
}

AppearanceDialog::AppearanceDialog(const MarkdownAppearance &appearance, QWidget *parent)
    : QDialog(parent)
    , appearance(appearance)
{
    setWindowTitle(tr("Edit Theme"));
    resize(920, 680);

    createLayout();
    loadAppearanceIntoControls();
    updatePreview();
}

AppearanceDialog::AppearanceDialog(const Theme &theme, QWidget *parent)
    : QDialog(parent)
    , appearance(theme.appearance)
    , themeName(theme.name)
{
    setWindowTitle(tr("Edit Theme"));
    resize(920, 680);

    createLayout();
    loadAppearanceIntoControls();
    updatePreview();
}

Theme AppearanceDialog::editedTheme() const
{
    return Theme{nameEdit->text().trimmed(), false, appearance};
}

void AppearanceDialog::chooseBodyFont()
{
    bool accepted = false;
    const QFont chosenFont = QFontDialog::getFont(&accepted, appearance.bodyFont(), this, tr("Choose Body Font"));
    if (!accepted) {
        return;
    }

    appearance.bodyFontFamily = chosenFont.family();
    if (chosenFont.pointSize() > 0) {
        appearance.bodyFontSize = chosenFont.pointSize();
    }
    updateFontSummary(bodyFontLabel, appearance.bodyFontFamily, appearance.bodyFontSize, appearance.bodyFont());
    updatePreview();
}

void AppearanceDialog::chooseColor(QColor &targetColor, QPushButton *button)
{
    const QColor chosenColor = QColorDialog::getColor(targetColor, this, tr("Choose Color"));
    if (!chosenColor.isValid()) {
        return;
    }

    targetColor = chosenColor;
    updateButtonSwatch(button, targetColor);
    updatePreview();
}

void AppearanceDialog::chooseMonospaceFont()
{
    bool accepted = false;
    const QFont chosenFont = QFontDialog::getFont(&accepted, appearance.monospaceFont(), this, tr("Choose Code Font"));
    if (!accepted) {
        return;
    }

    appearance.monospaceFontFamily = chosenFont.family();
    if (chosenFont.pointSize() > 0) {
        appearance.monospaceFontSize = chosenFont.pointSize();
    }
    updateFontSummary(monospaceFontLabel, appearance.monospaceFontFamily, appearance.monospaceFontSize, appearance.monospaceFont());
    updatePreview();
}

void AppearanceDialog::createLayout()
{
    auto *mainLayout = new QVBoxLayout(this);
    auto *contentLayout = new QHBoxLayout();
    mainLayout->addLayout(contentLayout, 1);

    auto *controlsContainer = new QWidget(this);
    auto *controlsLayout = new QVBoxLayout(controlsContainer);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(16);

    auto *nameRow = new QWidget(controlsContainer);
    auto *nameLayout = new QHBoxLayout(nameRow);
    nameLayout->setContentsMargins(0, 0, 0, 0);
    auto *nameLabel = new QLabel(tr("Name"), nameRow);
    nameEdit = new QLineEdit(nameRow);
    nameLayout->addWidget(nameLabel, 0);
    nameLayout->addWidget(nameEdit, 1);
    controlsLayout->addWidget(nameRow);

    auto *typographyGroup = new QGroupBox(tr("Typography"), controlsContainer);
    auto *typographyLayout = new QFormLayout(typographyGroup);

    auto *bodyFontRow = new QWidget(typographyGroup);
    auto *bodyFontLayout = new QHBoxLayout(bodyFontRow);
    bodyFontLayout->setContentsMargins(0, 0, 0, 0);
    bodyFontLabel = new QLabel(bodyFontRow);
    bodyFontButton = new QPushButton(tr("Choose..."), bodyFontRow);
    bodyFontLayout->addWidget(bodyFontLabel, 1);
    bodyFontLayout->addWidget(bodyFontButton, 0);

    auto *monoFontRow = new QWidget(typographyGroup);
    auto *monoFontLayout = new QHBoxLayout(monoFontRow);
    monoFontLayout->setContentsMargins(0, 0, 0, 0);
    monospaceFontLabel = new QLabel(monoFontRow);
    monospaceFontButton = new QPushButton(tr("Choose..."), monoFontRow);
    monoFontLayout->addWidget(monospaceFontLabel, 1);
    monoFontLayout->addWidget(monospaceFontButton, 0);

    lineHeightSpinBox = new QDoubleSpinBox(typographyGroup);
    lineHeightSpinBox->setRange(1.0, 2.4);
    lineHeightSpinBox->setSingleStep(0.05);
    lineHeightSpinBox->setDecimals(2);

    contentPaddingSpinBox = new QSpinBox(typographyGroup);
    contentPaddingSpinBox->setRange(12, 96);
    contentPaddingSpinBox->setSuffix(tr(" px"));

    contentMaxWidthSpinBox = new QSpinBox(typographyGroup);
    contentMaxWidthSpinBox->setRange(480, 2000);
    contentMaxWidthSpinBox->setSingleStep(20);
    contentMaxWidthSpinBox->setSuffix(tr(" px"));

    typographyLayout->addRow(tr("Body font"), bodyFontRow);
    typographyLayout->addRow(tr("Code font"), monoFontRow);
    typographyLayout->addRow(tr("Line height"), lineHeightSpinBox);
    typographyLayout->addRow(tr("Max text width"), contentMaxWidthSpinBox);
    typographyLayout->addRow(tr("Document margins"), contentPaddingSpinBox);
    controlsLayout->addWidget(typographyGroup);

    auto *colorsGroup = new QGroupBox(tr("Colors"), controlsContainer);
    auto *colorsLayout = new QGridLayout(colorsGroup);

    bodyTextColorButton = new QPushButton(colorsGroup);
    pageBackgroundColorButton = new QPushButton(colorsGroup);
    headingColorButton = new QPushButton(colorsGroup);
    linkColorButton = new QPushButton(colorsGroup);
    mutedTextColorButton = new QPushButton(colorsGroup);
    codeTextColorButton = new QPushButton(colorsGroup);
    codeBackgroundColorButton = new QPushButton(colorsGroup);
    panelBackgroundColorButton = new QPushButton(colorsGroup);
    borderColorButton = new QPushButton(colorsGroup);
    blockquoteBorderColorButton = new QPushButton(colorsGroup);

    const struct ColorRow
    {
        const char *label;
        QPushButton **button;
    } rows[] {
        {"Body text", &bodyTextColorButton},
        {"Page background", &pageBackgroundColorButton},
        {"Headings", &headingColorButton},
        {"Links", &linkColorButton},
        {"Muted text", &mutedTextColorButton},
        {"Code text", &codeTextColorButton},
        {"Inline code background", &codeBackgroundColorButton},
        {"Panels / code blocks", &panelBackgroundColorButton},
        {"Borders", &borderColorButton},
        {"Blockquote border", &blockquoteBorderColorButton},
    };

    for (int row = 0; row < static_cast<int>(std::size(rows)); ++row) {
        colorsLayout->addWidget(new QLabel(tr(rows[row].label), colorsGroup), row, 0);
        colorsLayout->addWidget(*rows[row].button, row, 1);
    }

    controlsLayout->addWidget(colorsGroup);
    controlsLayout->addStretch(1);

    contentLayout->addWidget(controlsContainer, 0);

    auto *previewGroup = new QGroupBox(tr("Preview"), this);
    auto *previewLayout = new QVBoxLayout(previewGroup);
    preview = new QWebEngineView(previewGroup);
    preview->setPage(new LinkPolicyWebPage(LinkPolicy::IgnoreClicks, preview));
    preview->setContextMenuPolicy(Qt::NoContextMenu);
    previewLayout->addWidget(preview);
    contentLayout->addWidget(previewGroup, 1);

    auto *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults,
        this);
    mainLayout->addWidget(buttonBox);

    // Pin the default button so Enter in a spin box applies and closes instead
    // of hitting whichever button the style put first in the focus chain.
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &AppearanceDialog::resetDefaults);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        syncAppearanceFromControls();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    const auto updatePreviewConnection = [this]() {
        syncAppearanceFromControls();
        updatePreview();
    };

    connect(bodyFontButton, &QPushButton::clicked, this, &AppearanceDialog::chooseBodyFont);
    connect(monospaceFontButton, &QPushButton::clicked, this, &AppearanceDialog::chooseMonospaceFont);
    connect(lineHeightSpinBox, &QDoubleSpinBox::valueChanged, this, updatePreviewConnection);
    connect(contentPaddingSpinBox, &QSpinBox::valueChanged, this, updatePreviewConnection);
    connect(contentMaxWidthSpinBox, &QSpinBox::valueChanged, this, updatePreviewConnection);

    connect(bodyTextColorButton, &QPushButton::clicked, this, [this]() { chooseColor(appearance.bodyTextColor, bodyTextColorButton); });
    connect(pageBackgroundColorButton, &QPushButton::clicked, this, [this]() { chooseColor(appearance.pageBackgroundColor, pageBackgroundColorButton); });
    connect(headingColorButton, &QPushButton::clicked, this, [this]() { chooseColor(appearance.headingColor, headingColorButton); });
    connect(linkColorButton, &QPushButton::clicked, this, [this]() { chooseColor(appearance.linkColor, linkColorButton); });
    connect(mutedTextColorButton, &QPushButton::clicked, this, [this]() { chooseColor(appearance.mutedTextColor, mutedTextColorButton); });
    connect(codeTextColorButton, &QPushButton::clicked, this, [this]() { chooseColor(appearance.codeTextColor, codeTextColorButton); });
    connect(codeBackgroundColorButton, &QPushButton::clicked, this, [this]() { chooseColor(appearance.codeBackgroundColor, codeBackgroundColorButton); });
    connect(panelBackgroundColorButton, &QPushButton::clicked, this, [this]() { chooseColor(appearance.panelBackgroundColor, panelBackgroundColorButton); });
    connect(borderColorButton, &QPushButton::clicked, this, [this]() { chooseColor(appearance.borderColor, borderColorButton); });
    connect(blockquoteBorderColorButton, &QPushButton::clicked, this, [this]() { chooseColor(appearance.blockquoteBorderColor, blockquoteBorderColorButton); });
}

void AppearanceDialog::loadAppearanceIntoControls()
{
    const QSignalBlocker b1(lineHeightSpinBox);
    const QSignalBlocker b2(contentPaddingSpinBox);
    const QSignalBlocker b3(contentMaxWidthSpinBox);

    nameEdit->setText(themeName);
    lineHeightSpinBox->setValue(appearance.lineHeight);
    contentPaddingSpinBox->setValue(appearance.contentPadding);
    contentMaxWidthSpinBox->setValue(appearance.contentMaxWidth);
    updateFontSummary(bodyFontLabel, appearance.bodyFontFamily, appearance.bodyFontSize, appearance.bodyFont());
    updateFontSummary(monospaceFontLabel, appearance.monospaceFontFamily, appearance.monospaceFontSize, appearance.monospaceFont());

    updateButtonSwatch(bodyTextColorButton, appearance.bodyTextColor);
    updateButtonSwatch(pageBackgroundColorButton, appearance.pageBackgroundColor);
    updateButtonSwatch(headingColorButton, appearance.headingColor);
    updateButtonSwatch(linkColorButton, appearance.linkColor);
    updateButtonSwatch(mutedTextColorButton, appearance.mutedTextColor);
    updateButtonSwatch(codeTextColorButton, appearance.codeTextColor);
    updateButtonSwatch(codeBackgroundColorButton, appearance.codeBackgroundColor);
    updateButtonSwatch(panelBackgroundColorButton, appearance.panelBackgroundColor);
    updateButtonSwatch(borderColorButton, appearance.borderColor);
    updateButtonSwatch(blockquoteBorderColorButton, appearance.blockquoteBorderColor);
}

void AppearanceDialog::resetDefaults()
{
    appearance = MarkdownAppearance::defaults();
    loadAppearanceIntoControls();
    updatePreview();
}

void AppearanceDialog::syncAppearanceFromControls()
{
    appearance.lineHeight = lineHeightSpinBox->value();
    appearance.contentPadding = contentPaddingSpinBox->value();
    appearance.contentMaxWidth = contentMaxWidthSpinBox->value();
}

void AppearanceDialog::updateFontSummary(QLabel *label, const QString &family, int pointSize, const QFont &sampleFont)
{
    if (pointSize > 0) {
        label->setText(tr("%1, %2 pt").arg(family, QString::number(pointSize)));
    } else {
        label->setText(family);
    }
    QFont displayFont(sampleFont.family());
    label->setFont(displayFont);
}

void AppearanceDialog::updateButtonSwatch(QPushButton *button, const QColor &color)
{
    button->setText(color.name(QColor::HexRgb));
    button->setStyleSheet(QStringLiteral(R"(
        QPushButton {
            text-align: left;
            padding: 6px 10px;
            border: 1px solid #9aa7b7;
            border-radius: 6px;
            background: %1;
            color: %2;
        }
    )")
                              .arg(color.name(QColor::HexRgb),
                                   color.lightnessF() < 0.45 ? QStringLiteral("#ffffff") : QStringLiteral("#111827")));
}

void AppearanceDialog::updatePreview()
{
    updateFontSummary(bodyFontLabel, appearance.bodyFontFamily, appearance.bodyFontSize, appearance.bodyFont());
    updateFontSummary(monospaceFontLabel, appearance.monospaceFontFamily, appearance.monospaceFontSize, appearance.monospaceFont());
    preview->setContent(appearance.renderMarkdownToHtml(previewMarkdown()).toUtf8(), QStringLiteral("text/html;charset=UTF-8"));
}
