#include "countdown_dialog.hpp"
#include "dashboard_appearance.hpp"
#include <QFont>
#include <QEvent>
#include <QHideEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShowEvent>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace hamclock {
namespace {
static_assert(std::is_nothrow_move_constructible_v<CountdownServiceSnapshot>);
static_assert(std::is_nothrow_move_assignable_v<CountdownServiceSnapshot>);
QPushButton* button(QWidget* parent, const QString& text, const QString& name) {
    auto* output = new QPushButton(text, parent);
    output->setObjectName(name);
    output->setAccessibleName(text);
    output->setMinimumSize(64, 44);
    output->setFocusPolicy(Qt::StrongFocus);
    output->setAutoDefault(false);
    return output;
}
QString run_text(CountdownPhase phase) {
    if (phase == CountdownPhase::Idle) return QStringLiteral("Start");
    if (phase == CountdownPhase::Paused) return QStringLiteral("Resume");
    return QStringLiteral("Restart");
}
} // namespace

CountdownDialog::CountdownDialog(CountdownService& service, Clock clock,
                               const std::string& appearance, QWidget* parent)
    : QDialog(parent), service_(service), clock_(std::move(clock)) {
    if (!clock_) throw std::invalid_argument("Countdown clock is required");
    const auto initial = service_.snapshot();
    setObjectName("countdownDialog");
    setWindowTitle("Countdown · HamClock Pro");
    setAccessibleName("Countdown");
    resize(560, 360);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    auto* heading = new QLabel("Countdown", this);
    heading->setTextFormat(Qt::PlainText);
    layout->addWidget(heading);
    remaining_ = new QLabel(this);
    remaining_->setObjectName("countdownRemaining");
    remaining_->setTextFormat(Qt::PlainText);
    remaining_->setAlignment(Qt::AlignCenter);
    QFont font = remaining_->font();
    font.setPointSize(24);
    font.setStyleHint(QFont::Monospace);
    remaining_->setFont(font);
    layout->addWidget(remaining_, 1);
    configured_ = new QLabel(this);
    configured_->setObjectName("countdownConfigured");
    configured_->setTextFormat(Qt::PlainText);
    layout->addWidget(configured_);
    status_ = new QLabel(this);
    status_->setObjectName("countdownStatus");
    status_->setTextFormat(Qt::PlainText);
    status_->setWordWrap(true);
    layout->addWidget(status_);
    error_ = new QLabel(this);
    error_->setObjectName("countdownError");
    error_->setTextFormat(Qt::PlainText);
    error_->setWordWrap(true);
    error_->setVisible(false);
    layout->addWidget(error_);
    auto* draft_row = new QHBoxLayout;
    auto* draft_label = new QLabel("Minutes", this);
    draft_label->setTextFormat(Qt::PlainText);
    minutes_ = new QSpinBox(this);
    minutes_->setObjectName("countdownMinutes");
    minutes_->setAccessibleName("Draft countdown duration in minutes");
    minutes_->setRange(1, 10080);
    minutes_->setMinimumSize(100, 44);
    minutes_->setKeyboardTracking(false);
    minutes_->setFocusPolicy(Qt::StrongFocus);
    minutes_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    decrease_ = button(this, "−", "countdownDecrease");
    increase_ = button(this, "+", "countdownIncrease");
    decrease_->setAccessibleName("Decrease draft minutes");
    increase_->setAccessibleName("Increase draft minutes");
    auto step = [this](bool up) {
        if (up) minutes_->stepUp();
        else minutes_->stepDown();
        invalid_draft_ = !minutes_->hasAcceptableInput();
        if (!invalid_draft_) {
            action_error_.clear();
            renderErrors();
        }
    };
    connect(decrease_, &QPushButton::clicked, this, [step] { step(false); });
    connect(increase_, &QPushButton::clicked, this, [step] { step(true); });
    connect(minutes_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        decrease_->setEnabled(value > minutes_->minimum());
        increase_->setEnabled(value < minutes_->maximum());
    });
    draft_label->setBuddy(minutes_);
    auto* line = minutes_->findChild<QLineEdit*>();
    connect(line, &QLineEdit::textEdited, this, [this] {
        invalid_draft_ = !minutes_->hasAcceptableInput();
        if (!invalid_draft_) {
            action_error_.clear();
            renderErrors();
        }
    });
    auto* configure = button(this, "Set && reset", "countdownSet");
    configure->setAccessibleName("Set and reset countdown duration");
    draft_row->addWidget(draft_label);
    draft_row->addWidget(decrease_);
    draft_row->addWidget(minutes_);
    draft_row->addWidget(increase_);
    draft_row->addWidget(configure);
    layout->addLayout(draft_row);
    auto* controls = new QHBoxLayout;
    run_ = button(this, "Start", "countdownRun");
    pause_ = button(this, "Pause", "countdownPause");
    auto* reset = button(this, "Reset", "countdownReset");
    dismiss_ = button(this, "Dismiss alert", "countdownDismiss");
    auto* exit = button(this, "Exit", "countdownExit");
    for (auto* control : {run_, pause_, reset, dismiss_, exit}) controls->addWidget(control);
    layout->addLayout(controls);
    setTabOrder(decrease_, minutes_);
    setTabOrder(minutes_, increase_);
    setTabOrder(increase_, configure);
    setTabOrder(configure, run_);
    setTabOrder(run_, pause_);
    setTabOrder(pause_, reset);
    setTabOrder(reset, dismiss_);
    setTabOrder(dismiss_, exit);
    connect(run_, &QPushButton::clicked, this, [this] { perform(CountdownControl::Run); });
    connect(pause_, &QPushButton::clicked, this, [this] { perform(CountdownControl::Pause); });
    connect(reset, &QPushButton::clicked, this, [this] { perform(CountdownControl::Reset); });
    connect(dismiss_, &QPushButton::clicked, this, [this] { perform(CountdownControl::Dismiss); });
    connect(configure, &QPushButton::clicked, this, [this] { perform(CountdownControl::SetAndReset); });
    connect(exit, &QPushButton::clicked, this, &QDialog::reject);
    auto* session = new QLabel("Session only · timers are not restored after app restart", this);
    session->setTextFormat(Qt::PlainText);
    session->setWordWrap(true);
    layout->addWidget(session);
    timer_ = new QTimer(this);
    timer_->setInterval(250);
    connect(timer_, &QTimer::timeout, this, [this] { refreshNow(); });
    apply_dashboard_appearance(*this, appearance);
    styleMinuteEditor();
    render(initial);
}
void CountdownDialog::styleMinuteEditor() {
    if (!minutes_) return;
    const auto colors = palette();
    minutes_->setPalette(colors);
    if (auto* line = minutes_->findChild<QLineEdit*>()) line->setPalette(colors);
    minutes_->setStyleSheet(QStringLiteral(
        "QSpinBox#countdownMinutes { background-color: %1; color: %2; border: 1px solid %2; border-radius: 7px; padding: 5px 9px; }"
        "QSpinBox#countdownMinutes:focus { border: 2px solid %3; }")
        .arg(colors.color(QPalette::Base).name(), colors.color(QPalette::Text).name(),
             colors.color(QPalette::Highlight).name()));
}
bool CountdownDialog::event(QEvent* event) {
    const bool handled = QDialog::event(event);
    if (event->type() == QEvent::PaletteChange) styleMinuteEditor();
    return handled;
}

void CountdownDialog::render(const CountdownServiceSnapshot& snapshot) {
    const auto remaining = QString::fromStdString(snapshot.view.remaining_text);
    const auto configured = QStringLiteral("Configured: ") + QString::fromStdString(snapshot.view.configured_text);
    const auto status = QString::fromStdString(snapshot.view.status);
    const auto remaining_name = QStringLiteral("Countdown remaining: ") + remaining;
    const auto status_name = QStringLiteral("Countdown status: ") + status;
    remaining_->setText(remaining);
    remaining_->setAccessibleName(remaining_name);
    configured_->setText(configured);
    configured_->setAccessibleName(configured);
    status_->setText(status);
    status_->setAccessibleName(status_name);
    run_->setText(run_text(snapshot.model.phase));
    run_->setAccessibleName(run_->text());
    pause_->setEnabled(snapshot.model.phase == CountdownPhase::Running);
    dismiss_->setEnabled(snapshot.dismiss_token.has_value());
    displayed_token_ = snapshot.dismiss_token;
    if (displayed_epoch_ != snapshot.configuration_epoch) {
        const auto duration = snapshot.model.configured.value;
        const auto minutes = duration / kCountdownMinimumNanoseconds +
                             (duration % kCountdownMinimumNanoseconds != 0 ? 1 : 0);
        minutes_->setValue(static_cast<int>(minutes));
        decrease_->setEnabled(minutes_->value() > minutes_->minimum());
        increase_->setEnabled(minutes_->value() < minutes_->maximum());
        invalid_draft_ = false;
        displayed_epoch_ = snapshot.configuration_epoch;
    }
}
void CountdownDialog::renderErrors() {
    const auto message = action_error_.isEmpty() ? clock_error_ :
        (clock_error_.isEmpty() ? action_error_ : action_error_ + '\n' + clock_error_);
    error_->setText(message);
    error_->setAccessibleName(message.isEmpty() ? QString() : QStringLiteral("Countdown error: ") + message);
    error_->setVisible(!message.isEmpty());
}
void CountdownDialog::reportFailure(const QString& error, bool action) {
    const auto message = QStringLiteral("Unable to update countdown: ") + error.left(240);
    if (action) action_error_ = message;
    else clock_error_ = message;
    renderErrors();
}
bool CountdownDialog::refreshNow() {
    std::optional<CountdownServiceSnapshot> output;
    try {
        output = service_.advance(clock_());
    } catch (const std::exception& error) {
        reportFailure(QString::fromUtf8(error.what()), false);
        return false;
    } catch (...) {
        reportFailure(QStringLiteral("Clock failed"), false);
        return false;
    }
    // Service committed. Qt rendering failures are not service rollback errors.
    render(*output);
    clock_error_.clear();
    renderErrors();
    return true;
}
bool CountdownDialog::perform(CountdownControl control) {
    std::optional<CountdownServiceSnapshot> output;
    try {
        if (control != CountdownControl::Run && control != CountdownControl::Pause &&
            control != CountdownControl::Reset && control != CountdownControl::Dismiss &&
            control != CountdownControl::SetAndReset) {
            throw std::invalid_argument("Unknown countdown control");
        }
        if (control == CountdownControl::Dismiss && !displayed_token_) {
            throw std::invalid_argument("No displayed pending countdown alert");
        }
        int draft = 0;
        if (control == CountdownControl::SetAndReset) {
            bool okay = false;
            draft = minutes_->cleanText().toInt(&okay);
            if (invalid_draft_ || !minutes_->hasAcceptableInput() || !okay || draft < 1 || draft > 10080) {
                throw std::invalid_argument("Enter a whole duration from 1 through 10080 minutes");
            }
        }
        const auto now = clock_();
        switch (control) {
        case CountdownControl::Run:
            output = service_.apply(service_.snapshot().model.phase == CountdownPhase::Paused ?
                                    CountdownCommand::Resume : CountdownCommand::Start, now).snapshot;
            break;
        case CountdownControl::Pause: output = service_.apply(CountdownCommand::Stop, now).snapshot; break;
        case CountdownControl::Reset: output = service_.apply(CountdownCommand::Reset, now).snapshot; break;
        case CountdownControl::Dismiss: output = service_.acknowledge(*displayed_token_, now).snapshot; break;
        case CountdownControl::SetAndReset:
            output = service_.reconfigure({static_cast<std::int64_t>(draft) * kCountdownMinimumNanoseconds}, now);
            break;
        }
    } catch (const std::exception& error) {
        reportFailure(QString::fromUtf8(error.what()), true);
        return false;
    } catch (...) {
        reportFailure(QStringLiteral("Clock or control failed"), true);
        return false;
    }
    render(*output);
    action_error_.clear();
    clock_error_.clear();
    renderErrors();
    return true;
}
void CountdownDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    refreshNow();
    timer_->start();
}
void CountdownDialog::hideEvent(QHideEvent* event) {
    timer_->stop();
    QDialog::hideEvent(event);
}
} // namespace hamclock
