#include "stopwatch_dialog.hpp"
#include "dashboard_appearance.hpp"
#include <QFont>
#include <QHideEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace hamclock {
namespace {
static_assert(std::is_nothrow_swappable_v<StopwatchState>);
static_assert(std::is_nothrow_swappable_v<StopwatchPresentation>);
QString state_text(const StopwatchPresentationView& view) {
    switch (view.label) {
    case StopwatchDisplayLabel::Idle: return QStringLiteral("Ready");
    case StopwatchDisplayLabel::Running: return QStringLiteral("Running");
    case StopwatchDisplayLabel::RunningHeld: return QStringLiteral("Display held — counting continues");
    case StopwatchDisplayLabel::Stopped: return QStringLiteral("Stopped");
    }
    throw std::invalid_argument("Unknown stopwatch display label");
}
QPushButton* button(QWidget* parent, const QString& text, const QString& name) {
    auto* output = new QPushButton(text, parent);
    output->setObjectName(name);
    output->setAccessibleName(text);
    output->setMinimumSize(64, 44);
    output->setFocusPolicy(Qt::StrongFocus);
    output->setAutoDefault(false);
    return output;
}
} // namespace

StopwatchDialog::StopwatchDialog(StopwatchState& model, StopwatchPresentation& presentation,
                               Clock clock, const std::string& appearance, QWidget* parent)
    : QDialog(parent), model_(model), presentation_(presentation), clock_(std::move(clock)) {
    if (!clock_) throw std::invalid_argument("Stopwatch clock is required");
    auto initial = presentation_;
    const auto initial_view = initial.refresh(model_.snapshot());
    setObjectName("stopwatchDialog");
    setWindowTitle("Stopwatch · HamClock Pro");
    setAccessibleName("Stopwatch");
    resize(480, 300);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    auto* heading = new QLabel("Stopwatch", this);
    heading->setTextFormat(Qt::PlainText);
    layout->addWidget(heading);
    elapsed_ = new QLabel(this);
    elapsed_->setObjectName("stopwatchElapsed");
    elapsed_->setAccessibleName("Displayed stopwatch elapsed");
    elapsed_->setTextFormat(Qt::PlainText);
    elapsed_->setAlignment(Qt::AlignCenter);
    QFont font = elapsed_->font();
    font.setPointSize(24);
    font.setStyleHint(QFont::Monospace);
    elapsed_->setFont(font);
    layout->addWidget(elapsed_, 1);
    status_ = new QLabel(this);
    status_->setObjectName("stopwatchStatus");
    status_->setAccessibleName("Stopwatch status");
    status_->setTextFormat(Qt::PlainText);
    status_->setWordWrap(true);
    layout->addWidget(status_);
    actual_ = new QLabel(this);
    actual_->setObjectName("stopwatchActual");
    actual_->setAccessibleName("Actual stopwatch elapsed");
    actual_->setTextFormat(Qt::PlainText);
    layout->addWidget(actual_);
    auto* controls = new QHBoxLayout;
    run_ = button(this, "Run", "stopwatchRun");
    stop_ = button(this, "Stop", "stopwatchStop");
    lap_ = button(this, "Lap", "stopwatchLap");
    auto* reset = button(this, "Reset", "stopwatchReset");
    auto* exit = button(this, "Exit", "stopwatchExit");
    for (auto* control : {run_, stop_, lap_, reset, exit}) controls->addWidget(control);
    layout->addLayout(controls);
    setTabOrder(run_, stop_);
    setTabOrder(stop_, lap_);
    setTabOrder(lap_, reset);
    setTabOrder(reset, exit);
    connect(run_, &QPushButton::clicked, this, [this] { perform(StopwatchControl::Run); });
    connect(stop_, &QPushButton::clicked, this, [this] { perform(StopwatchControl::Stop); });
    connect(lap_, &QPushButton::clicked, this, [this] { perform(StopwatchControl::Lap); });
    connect(reset, &QPushButton::clicked, this, [this] { perform(StopwatchControl::Reset); });
    connect(exit, &QPushButton::clicked, this, &QDialog::reject);
    timer_ = new QTimer(this);
    timer_->setInterval(50);
    connect(timer_, &QTimer::timeout, this, [this] { refreshNow(); });
    apply_dashboard_appearance(*this, appearance);
    render(initial_view, QString::fromStdString(initial_view.formatted), state_text(initial_view));
}

void StopwatchDialog::render(const StopwatchPresentationView& view,
                             const QString& elapsed, const QString& status) {
    elapsed_->setText(elapsed);
    elapsed_->setAccessibleName(QStringLiteral("Displayed stopwatch elapsed: ") + elapsed);
    status_->setText(status);
    status_->setAccessibleName(QStringLiteral("Stopwatch status: ") + status);
    actual_->setText(view.held ? QStringLiteral("Actual elapsed: ") +
                    QString::fromStdString(format_stopwatch_duration(view.actual_elapsed)) : QString());
    actual_->setAccessibleName(actual_->text());
    stop_->setEnabled(view.phase == StopwatchPhase::Running);
    lap_->setEnabled(view.phase == StopwatchPhase::Running);
}

void StopwatchDialog::reportFailure(const QString& message) {
    const auto text = QStringLiteral("Unable to update stopwatch: ") + message.left(240);
    status_->setText(text);
    status_->setAccessibleName(QStringLiteral("Stopwatch status: ") + text);
}

bool StopwatchDialog::update(bool action, StopwatchControl control) {
    try {
        if (action && control != StopwatchControl::Run && control != StopwatchControl::Stop &&
            control != StopwatchControl::Lap && control != StopwatchControl::Reset) {
            throw std::invalid_argument("Unknown stopwatch control");
        }
        auto model = model_;
        auto presentation = presentation_;
        const auto now = clock_();
        if (!action) {
            model.advance(now);
        } else {
            switch (control) {
            case StopwatchControl::Run:
                if (model.snapshot().phase == StopwatchPhase::Idle) model.apply(StopwatchCommand::Start, now);
                else if (model.snapshot().phase == StopwatchPhase::Stopped) model.apply(StopwatchCommand::Resume, now);
                else model.advance(now);
                break;
            case StopwatchControl::Stop: model.apply(StopwatchCommand::Stop, now); break;
            case StopwatchControl::Lap: model.advance(now); break;
            case StopwatchControl::Reset: model.apply(StopwatchCommand::Reset, now); break;
            }
        }
        StopwatchPresentationView view;
        if (action && control == StopwatchControl::Run) {
            view = presentation.apply(StopwatchDisplayCommand::Release, model.snapshot()).view;
        } else if (action && control == StopwatchControl::Lap) {
            view = presentation.apply(StopwatchDisplayCommand::Hold, model.snapshot()).view;
        } else view = presentation.refresh(model.snapshot());
        const auto elapsed = QString::fromStdString(view.formatted);
        const auto status = state_text(view);
        // Prepare the secondary duration conversion too, before shared commit.
        const auto actual = view.held ? QStringLiteral("Actual elapsed: ") +
            QString::fromStdString(format_stopwatch_duration(view.actual_elapsed)) : QString();
        const auto elapsed_name = QStringLiteral("Displayed stopwatch elapsed: ") + elapsed;
        const auto status_name = QStringLiteral("Stopwatch status: ") + status;
        using std::swap;
        swap(model_, model);
        swap(presentation_, presentation);
        elapsed_->setText(elapsed);
        elapsed_->setAccessibleName(elapsed_name);
        status_->setText(status);
        status_->setAccessibleName(status_name);
        actual_->setText(actual);
        actual_->setAccessibleName(actual);
        stop_->setEnabled(view.phase == StopwatchPhase::Running);
        lap_->setEnabled(view.phase == StopwatchPhase::Running);
        return true;
    } catch (const std::exception& error) {
        reportFailure(QString::fromUtf8(error.what()));
    } catch (...) {
        reportFailure(QStringLiteral("Clock or control failed"));
    }
    return false;
}

bool StopwatchDialog::refreshNow() { return update(false, StopwatchControl::Run); }
bool StopwatchDialog::perform(StopwatchControl control) { return update(true, control); }
void StopwatchDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    refreshNow();
    timer_->start();
}
void StopwatchDialog::hideEvent(QHideEvent* event) {
    timer_->stop();
    QDialog::hideEvent(event);
}
} // namespace hamclock
