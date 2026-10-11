#include "big_clock_dialog.hpp"
#include "dashboard_appearance.hpp"
#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QFont>
#include <QGridLayout>
#include <QHideEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPalette>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionButton>
#include <QPushButton>
#include <QScrollArea>
#include <QScopedValueRollback>
#include <QStyledItemDelegate>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace hamclock {
namespace {
static_assert(std::is_nothrow_copy_assignable_v<BigClockDigitalView>);
static_assert(std::is_nothrow_copy_assignable_v<BigClockPreferences>);
QString status_text(const BigClockDigitalView& view) {
    QString status = view.basis == BigClockDisplayBasis::Preview ? "Preview" : "Actual";
    if (view.synchronization == SyncStatus::Unknown) status += " · Synchronization unknown";
    else if (view.synchronization == SyncStatus::Unsynchronized) status += " · Unsynchronized";
    else status += " · Synchronized evidence";
    if (view.status == BigClockDigitalStatus::MissingSelectedTime) status += " · Selected zone time unavailable";
    if (view.status == BigClockDigitalStatus::UnsupportedFace) status += " · Analog view unavailable";
    if (view.basis == BigClockDisplayBasis::Preview && !view.actual_selected_time_available)
        status += " · Actual selected zone time unavailable";
    return status;
}
QString time_text(const BigClockDigitalView& view) {
    if (view.status == BigClockDigitalStatus::UnsupportedFace) return "Analog unavailable";
    if (view.status == BigClockDigitalStatus::MissingSelectedTime) return "Time unavailable";
    return QString::fromLatin1(view.time_text.data());
}
QLabel* label(QWidget* parent, const char* name) {
    auto* item = new QLabel(parent);
    item->setObjectName(name); item->setTextFormat(Qt::PlainText); item->setWordWrap(true);
    return item;
}
QPushButton* button(QWidget* parent, const char* text, const char* name) {
    auto* item = new QPushButton(text, parent); item->setObjectName(name);
    item->setAccessibleName(text); item->setMinimumSize(88, 44);
    item->setFocusPolicy(Qt::StrongFocus); item->setAutoDefault(false); return item;
}
class TouchCheckBox final : public QCheckBox {
public:
    using QCheckBox::QCheckBox;
protected:
    bool hitButton(const QPoint& point) const override { return rect().contains(point); }
    void paintEvent(QPaintEvent* event) override {
        QCheckBox::paintEvent(event);
        QStyleOptionButton option; initStyleOption(&option);
        const auto area = style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, this);
        QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
        const auto ink_value = property("bigClockIndicatorInk");
        const auto background_value = property("bigClockIndicatorBackground");
        const auto ink = ink_value.isValid() ? ink_value.value<QColor>() : palette().color(QPalette::Text);
        const auto background = background_value.isValid() ? background_value.value<QColor>() : palette().color(QPalette::Window);
        painter.fillRect(area, background);
        painter.setPen(QPen(ink, 1.5)); painter.setBrush(background);
        painter.drawRoundedRect(QRectF(area).adjusted(1, 1, -1, -1), 2, 2);
        if (isChecked()) {
            QPainterPath mark;
            mark.moveTo(area.left() + area.width() * .23, area.top() + area.height() * .52);
            mark.lineTo(area.left() + area.width() * .43, area.top() + area.height() * .72);
            mark.lineTo(area.left() + area.width() * .78, area.top() + area.height() * .28);
            painter.setPen(QPen(ink, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.setBrush(Qt::NoBrush); painter.drawPath(mark);
        }
    }
};
class TouchComboBox final : public QComboBox {
public:
    using QComboBox::QComboBox;
protected:
    void paintEvent(QPaintEvent* event) override {
        QComboBox::paintEvent(event);
        QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
        const auto area = QRect(width() - 23, 5, 18, height() - 10);
        painter.fillRect(area, palette().color(QPalette::Base));
        const auto center = area.center();
        painter.setPen(Qt::NoPen); painter.setBrush(palette().color(QPalette::Text));
        painter.drawPolygon(QPolygonF{QPointF(center.x() - 5, center.y() - 3),
            QPointF(center.x() + 5, center.y() - 3), QPointF(center.x(), center.y() + 3)});
    }
};
class TouchOptionDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        auto size = QStyledItemDelegate::sizeHint(option, index);
        if (size.height() < 44) size.setHeight(44);
        return size;
    }
};
// Local observer keeps semantic popup palettes consistent on platforms whose
// combo popup does not inherit the dialog stylesheet. No global palette writes.
class OptionAppearance final : public QObject {
    QDialog& dialog_;
    void paint() {
        const auto colors = dialog_.palette();
        const auto base = colors.color(QPalette::Base).name();
        const auto ink = colors.color(QPalette::Text).name();
        const auto accent = colors.color(QPalette::Highlight).name();
        const auto selected = colors.color(QPalette::HighlightedText).name();
        const auto canvas = colors.color(QPalette::Window).name();
        if (auto* scroll = dialog_.findChild<QScrollArea*>("bigClockScroll")) {
            auto canvas_palette = colors;
            canvas_palette.setColor(QPalette::Base, colors.color(QPalette::Window));
            for (auto* area : {static_cast<QWidget*>(scroll), scroll->viewport(), scroll->widget()}) {
                area->setPalette(canvas_palette); area->setBackgroundRole(QPalette::Window);
                area->setAutoFillBackground(true);
            }
            scroll->setStyleSheet(QString("QScrollArea#bigClockScroll {background-color:%1;border:none;}").arg(canvas));
            scroll->viewport()->setStyleSheet(QString("QWidget#bigClockViewport {background-color:%1;}").arg(canvas));
            scroll->widget()->setStyleSheet(QString("QWidget#bigClockContent {background-color:%1;}").arg(canvas));
        }
        for (auto* combo : dialog_.findChildren<QComboBox*>()) {
            combo->setPalette(colors); combo->view()->setPalette(colors);
            combo->setStyleSheet(QString(
                "QComboBox {background:%1;color:%2;border:1px solid %2;border-radius:7px;padding:5px 24px 5px 9px;}"
                "QComboBox:focus {border:2px solid %3;}"
                "QComboBox::drop-down {width:22px;border:none;}"
                "QComboBox QAbstractItemView {background:%1;color:%2;selection-background-color:%3;selection-color:%4;}")
                .arg(base, ink, accent, selected));
        }
        for (auto* checkbox : dialog_.findChildren<QCheckBox*>()) {
            checkbox->setPalette(colors);
            // Native macOS style can replace a checkbox's Base role. Carry the
            // already resolved dialog canvas/ink explicitly for original paint.
            checkbox->setProperty("bigClockIndicatorBackground", colors.color(QPalette::Window));
            checkbox->setProperty("bigClockIndicatorInk", colors.color(QPalette::Text));
            checkbox->setStyleSheet(QString("QCheckBox {color:%1;padding:4px;} QCheckBox:focus {border:2px solid %2;border-radius:4px;}").arg(ink, accent));
        }
    }
public:
    explicit OptionAppearance(QDialog& dialog) : QObject(&dialog), dialog_(dialog) {
        dialog.installEventFilter(this); paint();
    }
protected:
    bool eventFilter(QObject*, QEvent* event) override {
        if (event->type() == QEvent::PaletteChange) paint();
        return false;
    }
};
} // namespace

BigClockDialog::BigClockDialog(const BigClockPreferences& preferences, Snapshot snapshot,
                             const std::string& appearance, QWidget* parent)
    : QDialog(parent), preferences_(preferences), snapshot_(std::move(snapshot)) {
    if (!snapshot_) throw std::invalid_argument("Big Clock snapshot is required");
    validate_big_clock_preferences(preferences_);
    if (appearance != "system" && appearance != "light" && appearance != "dark")
        throw std::invalid_argument("Invalid Big Clock appearance");
    const auto initial = present_big_clock_digital(preferences_, snapshot_());
    const auto initial_time = time_text(initial);
    const auto initial_date = QString::fromLatin1(initial.date_text.data());
    const auto initial_zone = QString::fromLatin1(initial.zone_text.data());
    const auto initial_status = status_text(initial);
    setObjectName("bigClockDialog"); setWindowTitle("Big Clock · HamClock Pro");
    setAccessibleName("Big Clock"); resize(800, 480);
    auto* outer = new QVBoxLayout(this); outer->setContentsMargins(16, 12, 16, 12);
    auto* scroll = new QScrollArea(this); scroll->setObjectName("bigClockScroll");
    scroll->viewport()->setObjectName("bigClockViewport");
    scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
    auto* body = new QWidget(scroll); auto* layout = new QVBoxLayout(body);
    body->setObjectName("bigClockContent");
    layout->setContentsMargins(4, 4, 4, 4); layout->setSpacing(10);
    zone_ = label(body, "bigClockZone"); layout->addWidget(zone_);
    time_ = label(body, "bigClockTime"); time_->setAlignment(Qt::AlignCenter);
    auto font = time_->font(); font.setPointSize(44); time_->setFont(font);
    layout->addWidget(time_, 1);
    date_ = label(body, "bigClockDate"); date_->setAlignment(Qt::AlignCenter); layout->addWidget(date_);
    status_ = label(body, "bigClockStatus"); layout->addWidget(status_);
    source_error_ = label(body, "bigClockSourceError"); layout->addWidget(source_error_); source_error_->hide();
    action_error_ = label(body, "bigClockActionError"); layout->addWidget(action_error_); action_error_->hide();
    features_ = label(body, "bigClockUnavailableFeatures");
    features_->setText("Weather, alarms and countdown are unavailable in this view.");
    features_->setAccessibleName(features_->text()); layout->addWidget(features_);
    options_panel_ = new QWidget(body); options_panel_->setObjectName("bigClockOptionsPanel");
    auto* options = new QGridLayout(options_panel_); options->setContentsMargins(0, 4, 0, 4);
    const auto combo = [&](const char* text, const char* name, int row) {
        auto* prompt = label(options_panel_, name); prompt->setText(text);
        prompt->setObjectName(QString::fromLatin1(name) + "Label");
        auto* item = new TouchComboBox(options_panel_); item->setObjectName(name);
        item->setAccessibleName(text); item->setMinimumHeight(44); item->setFocusPolicy(Qt::StrongFocus);
        item->setItemDelegate(new TouchOptionDelegate(item));
        prompt->setBuddy(item); options->addWidget(prompt, row, 0); options->addWidget(item, row, 1);
        return item;
    };
    face_ = combo("Clock face", "bigClockFace", 0); face_->addItems({"Analog (unavailable)", "Digital"});
    zone_option_ = combo("Time zone", "bigClockZoneOption", 1); zone_option_->addItems({"UTC", "DE"});
    hour_format_ = combo("Hour format", "bigClockHourFormat", 2); hour_format_->addItems({"12-hour", "24-hour"});
    show_date_ = new TouchCheckBox("Show date", options_panel_); show_date_->setObjectName("bigClockShowDate");
    show_seconds_ = new TouchCheckBox("Show seconds", options_panel_); show_seconds_->setObjectName("bigClockShowSeconds");
    for (auto* item : {show_date_, show_seconds_}) {
        item->setAccessibleName(item->text()); item->setMinimumHeight(44); item->setFocusPolicy(Qt::StrongFocus);
    }
    options->addWidget(show_date_, 3, 0); options->addWidget(show_seconds_, 3, 1);
    layout->addWidget(options_panel_); options_panel_->hide();
    scroll->setWidget(body); outer->addWidget(scroll, 1);
    auto* footer = new QHBoxLayout;
    options_button_ = button(this, "Options", "bigClockOptions");
    exit_button_ = button(this, "Exit", "bigClockExit");
    footer->addWidget(options_button_); footer->addStretch(); footer->addWidget(exit_button_); outer->addLayout(footer);
    setTabOrder(options_button_, face_); setTabOrder(face_, zone_option_); setTabOrder(zone_option_, hour_format_);
    setTabOrder(hour_format_, show_date_); setTabOrder(show_date_, show_seconds_); setTabOrder(show_seconds_, exit_button_);
    connect(options_button_, &QPushButton::clicked, this, &BigClockDialog::showOptions);
    connect(exit_button_, &QPushButton::clicked, this, &QDialog::reject);
    connect(face_, &QComboBox::currentIndexChanged, this, [this](int) { applyOptionControls(); });
    connect(zone_option_, &QComboBox::currentIndexChanged, this, [this](int) { applyOptionControls(); });
    connect(hour_format_, &QComboBox::currentIndexChanged, this, [this](int) { applyOptionControls(); });
    connect(show_date_, &QCheckBox::toggled, this, [this](bool) { applyOptionControls(); });
    connect(show_seconds_, &QCheckBox::toggled, this, [this](bool) { applyOptionControls(); });
    timer_ = new QTimer(this); timer_->setObjectName("bigClockRefreshTimer"); timer_->setInterval(1000);
    connect(timer_, &QTimer::timeout, this, [this] { if (isVisible()) refreshNow(); });
    apply_dashboard_appearance(*this, appearance); new OptionAppearance(*this);
    last_view_ = initial;
    time_->setText(initial_time); time_->setAccessibleName("Displayed clock time: " + initial_time);
    date_->setText(initial_date); date_->setAccessibleName("Clock date: " + initial_date);
    zone_->setText(initial_zone); zone_->setAccessibleName("Clock zone: " + initial_zone);
    status_->setText(initial_status); status_->setAccessibleName("Clock status: " + initial_status);
    const QSignalBlocker a(face_), b(zone_option_), c(hour_format_), d(show_date_), e(show_seconds_);
    face_->setCurrentIndex(static_cast<int>(preferences_.face)); zone_option_->setCurrentIndex(static_cast<int>(preferences_.zone));
    hour_format_->setCurrentIndex(static_cast<int>(preferences_.hour_format)); show_date_->setChecked(preferences_.show_date); show_seconds_->setChecked(preferences_.show_seconds);
}

const BigClockPreferences& BigClockDialog::preferences() const noexcept { return preferences_; }
const BigClockDigitalView& BigClockDialog::lastView() const noexcept { return last_view_; }
bool BigClockDialog::displayStale() const noexcept { return stale_; }

void BigClockDialog::reportFailure(const QString& message, bool action, bool source_failed) {
    if (source_failed) {
        stale_ = true;
        source_error_->setText("Display could not refresh: " + message.left(200));
        source_error_->setAccessibleName(source_error_->text()); source_error_->show();
        status_->setText("Display stale · Last successful value; source not refreshed");
        status_->setAccessibleName("Clock status: " + status_->text());
        time_->setAccessibleName("Displayed clock time (stale): " + time_->text());
        date_->setAccessibleName("Clock date (stale): " + date_->text());
    }
    if (action) {
        action_error_->setText("Options were not applied: " + message.left(200));
        action_error_->setAccessibleName(action_error_->text()); action_error_->show();
    }
}
bool BigClockDialog::update(const BigClockPreferences& candidate, bool action) {
    if (updating_) return false;
    const QScopedValueRollback<bool> in_flight(updating_, true);
    const auto prepared_preferences = candidate;
    BigClockDigitalView view{};
    QString time, date, zone, status, time_name, date_name, zone_name, status_name;
    bool source_failed = false, prepared = false;
    try {
        validate_big_clock_preferences(prepared_preferences);
        source_failed = true;
        view = present_big_clock_digital(prepared_preferences, snapshot_());
        time = time_text(view); date = QString::fromLatin1(view.date_text.data());
        zone = QString::fromLatin1(view.zone_text.data()); status = status_text(view);
        time_name = "Displayed clock time: " + time; date_name = "Clock date: " + date;
        zone_name = "Clock zone: " + zone; status_name = "Clock status: " + status;
        prepared = true;
    } catch (const std::exception& error) { reportFailure(QString::fromUtf8(error.what()), action, source_failed); }
      catch (...) { reportFailure("Snapshot or presentation failed", action, source_failed); }
    if (!prepared) {
        const QSignalBlocker a(face_), b(zone_option_), c(hour_format_), d(show_date_), e(show_seconds_);
        face_->setCurrentIndex(static_cast<int>(preferences_.face)); zone_option_->setCurrentIndex(static_cast<int>(preferences_.zone));
        hour_format_->setCurrentIndex(static_cast<int>(preferences_.hour_format)); show_date_->setChecked(preferences_.show_date); show_seconds_->setChecked(preferences_.show_seconds);
        return false;
    }
    // Nonthrowing value commit. All subsequent Qt rendering deliberately sits
    // OUTSIDE the recoverable preparation catch: a postcommit exception must
    // not claim that an applied option was rejected or rolled back.
    preferences_ = prepared_preferences; last_view_ = view; stale_ = false;
    time_->setText(time); time_->setAccessibleName(time_name); date_->setText(date); date_->setAccessibleName(date_name);
    zone_->setText(zone); zone_->setAccessibleName(zone_name); status_->setText(status); status_->setAccessibleName(status_name);
    source_error_->clear(); source_error_->setAccessibleName({}); source_error_->hide();
    if (action) { action_error_->clear(); action_error_->setAccessibleName({}); action_error_->hide(); }
    const QSignalBlocker a(face_), b(zone_option_), c(hour_format_), d(show_date_), e(show_seconds_);
    face_->setCurrentIndex(static_cast<int>(preferences_.face)); zone_option_->setCurrentIndex(static_cast<int>(preferences_.zone));
    hour_format_->setCurrentIndex(static_cast<int>(preferences_.hour_format)); show_date_->setChecked(preferences_.show_date); show_seconds_->setChecked(preferences_.show_seconds);
    return true;
}
bool BigClockDialog::refreshNow() { return update(preferences_, false); }
bool BigClockDialog::setPreferences(const BigClockPreferences& candidate) { return update(candidate, true); }
void BigClockDialog::applyOptionControls() {
    auto candidate = preferences_;
    candidate.face = static_cast<BigClockFace>(face_->currentIndex()); candidate.zone = static_cast<BigClockZone>(zone_option_->currentIndex());
    candidate.hour_format = static_cast<BigClockHourFormat>(hour_format_->currentIndex());
    candidate.show_date = show_date_->isChecked(); candidate.show_seconds = show_seconds_->isChecked();
    setPreferences(candidate);
}
void BigClockDialog::showOptions() {
    const bool show = !options_panel_->isVisible(); options_panel_->setVisible(show);
    if (show) face_->setFocus(); else options_button_->setFocus();
}
void BigClockDialog::showEvent(QShowEvent* event) { QDialog::showEvent(event); refreshNow(); timer_->start(); }
void BigClockDialog::hideEvent(QHideEvent* event) { timer_->stop(); QDialog::hideEvent(event); }
void BigClockDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape && options_panel_->isVisible()) {
        options_panel_->hide(); options_button_->setFocus(); event->accept(); return;
    }
    QDialog::keyPressEvent(event);
}
} // namespace hamclock
