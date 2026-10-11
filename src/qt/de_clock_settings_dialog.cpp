#include "de_clock_settings_dialog.hpp"
#include "dashboard_appearance.hpp"
#include <QAbstractItemView>
#include <QComboBox>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QScopedValueRollback>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStyledItemDelegate>
#include <QVariant>
#include <QVBoxLayout>
#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace hamclock {
namespace {
static_assert(std::is_nothrow_copy_assignable_v<DeClockConfigurationSnapshot>);
bool same_config(const DeClockConfiguration& a, const DeClockConfiguration& b) {
    return a.mode == b.mode && a.manual_offset.has_value() == b.manual_offset.has_value() &&
        (!a.manual_offset || a.manual_offset->value == b.manual_offset->value);
}
bool same_context(const DeClockConfigurationSnapshot& a, const DeClockConfigurationSnapshot& b) {
    return a.offset_policy.minimum.value == b.offset_policy.minimum.value &&
        a.offset_policy.maximum.value == b.offset_policy.maximum.value &&
        a.location.latitude == b.location.latitude && a.location.longitude == b.location.longitude &&
        a.location_revision == b.location_revision;
}
void validate_snapshot(const DeClockConfigurationSnapshot& snapshot) {
    validate_de_clock_configuration(snapshot.configuration, snapshot.offset_policy);
    if (snapshot.offset_policy.minimum.value != kDeClockEditorMinimumSeconds ||
        snapshot.offset_policy.maximum.value != kDeClockEditorMaximumSeconds ||
        !std::isfinite(snapshot.location.latitude) || !std::isfinite(snapshot.location.longitude) ||
        snapshot.location.latitude < -90 || snapshot.location.latitude > 90 ||
        snapshot.location.longitude < -180 || snapshot.location.longitude >= 180 ||
        snapshot.location_revision == 0 || snapshot.configuration_revision < snapshot.location_revision)
        throw std::invalid_argument("Invalid DE settings snapshot");
}
QString offset_text(std::int64_t offset) {
    const auto magnitude = offset < 0 ? -offset : offset; // Validated ±86400 only.
    auto result = QString("%1%2:%3").arg(offset < 0 ? '-' : '+')
        .arg(magnitude / 3600, 2, 10, QChar('0')).arg(magnitude / 60 % 60, 2, 10, QChar('0'));
    if (magnitude % 60) result += QString(":%1").arg(magnitude % 60, 2, 10, QChar('0'));
    return result;
}
QString current_text(const DeClockConfigurationSnapshot& snapshot) {
    return snapshot.configuration.mode == DeClockMode::Automatic ? QString("Current setting: Automatic (unresolved)") :
        QString("Current setting: Manual fixed offset %1").arg(offset_text(snapshot.configuration.manual_offset->value));
}
UtcOffsetSeconds parse_offset(const QString& text) {
    if ((text.size() != 6 && text.size() != 9) || (text[0] != '+' && text[0] != '-') ||
        text[3] != ':' || (text.size() == 9 && text[6] != ':'))
        throw std::invalid_argument("Invalid offset draft");
    const auto pair = [&](int start) {
        const auto a = text[start].unicode(), b = text[start + 1].unicode();
        if (a < '0' || a > '9' || b < '0' || b > '9') throw std::invalid_argument("Invalid offset digits");
        return static_cast<int>((a - '0') * 10 + b - '0');
    };
    const int hour = pair(1), minute = pair(4), second = text.size() == 9 ? pair(7) : 0;
    if (hour > 24 || minute > 59 || second > 59 || (hour == 24 && (minute || second)))
        throw std::invalid_argument("Offset outside editor limits");
    const std::int64_t magnitude = hour * 3600 + minute * 60 + second;
    return {text[0] == '-' ? -magnitude : magnitude};
}
QString outcome_message(const DeClockSettingsApplyOutcome& outcome) {
    bool ended = false; std::size_t length = 0;
    for (const char byte : outcome.message) {
        const auto value = static_cast<unsigned char>(byte);
        if (ended) { if (value != 0) throw std::invalid_argument("Invalid message padding"); }
        else if (value == 0) ended = true;
        else { if (value < 32 || value > 126) throw std::invalid_argument("Invalid message byte"); ++length; }
    }
    if (!ended) throw std::invalid_argument("Unterminated outcome message");
    const bool reason = outcome.status == DeClockSettingsApplyStatus::Rejected ||
                        outcome.status == DeClockSettingsApplyStatus::CommittedDisplayError;
    if (reason != (length != 0)) throw std::invalid_argument("Invalid outcome reason");
    return QString::fromLatin1(outcome.message.data(), static_cast<qsizetype>(length));
}
void validate_outcome(const DeClockSettingsApplyOutcome& outcome, const DeClockConfiguration& desired,
                      const DeClockConfigurationSnapshot& basis) {
    if (outcome.status != DeClockSettingsApplyStatus::Rejected && outcome.status != DeClockSettingsApplyStatus::Applied &&
        outcome.status != DeClockSettingsApplyStatus::Unchanged && outcome.status != DeClockSettingsApplyStatus::CommittedDisplayError)
        throw std::invalid_argument("Unknown apply outcome");
    validate_snapshot(outcome.confirmed);
    if (outcome.status == DeClockSettingsApplyStatus::Rejected) return;
    const bool changed = !same_config(desired, basis.configuration);
    if (!same_config(outcome.confirmed.configuration, desired) || !same_context(outcome.confirmed, basis) ||
        outcome.confirmed.configuration_revision != (changed ? next_de_clock_revision(basis.configuration_revision) : basis.configuration_revision) ||
        (outcome.status == DeClockSettingsApplyStatus::Applied && !changed) ||
        (outcome.status == DeClockSettingsApplyStatus::Unchanged && changed))
        throw std::invalid_argument("Unconfirmed apply outcome");
}
QLabel* label(QWidget* parent, const char* name) {
    auto* item = new QLabel(parent); item->setObjectName(name); item->setTextFormat(Qt::PlainText);
    item->setWordWrap(true); return item;
}
void named_text(QLabel* item, const QString& text) { item->setText(text); item->setAccessibleName(text); }
class OffsetCombo final : public QComboBox {
public: using QComboBox::QComboBox;
protected:
    void paintEvent(QPaintEvent* event) override {
        QComboBox::paintEvent(event); QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
        const QRect area(width() - 23, 5, 18, height() - 10); const auto center = area.center();
        painter.fillRect(area, property("deClockFieldBase").value<QColor>());
        painter.setPen(Qt::NoPen); painter.setBrush(property("deClockFieldInk").value<QColor>());
        painter.drawPolygon(QPolygonF{QPointF(center.x()-5,center.y()-3),QPointF(center.x()+5,center.y()-3),QPointF(center.x(),center.y()+3)});
    }
};
class OffsetLineEdit final : public QLineEdit {
public: using QLineEdit::QLineEdit;
protected:
    void keyPressEvent(QKeyEvent* event) override {
        if(event->key()==Qt::Key_Return || event->key()==Qt::Key_Enter){Q_EMIT returnPressed();event->accept();return;}
        QLineEdit::keyPressEvent(event);
    }
};
class OffsetDelegate final : public QStyledItemDelegate {
public: using QStyledItemDelegate::QStyledItemDelegate;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        auto size = QStyledItemDelegate::sizeHint(option,index); if(size.height()<44)size.setHeight(44); return size;
    }
};
class OffsetAppearance final : public QObject {
    QDialog& dialog_;
    void paint() {
        const auto colors = dialog_.palette();
        const auto base = colors.color(QPalette::Base).name(), ink = colors.color(QPalette::Text).name();
        const auto canvas = colors.color(QPalette::Window).name(), accent = colors.color(QPalette::Highlight).name();
        const auto selected = colors.color(QPalette::HighlightedText).name();
        for (auto* field : dialog_.findChildren<QLineEdit*>()) {
            field->setPalette(colors);
            field->setStyleSheet(QString("QLineEdit {background:%1;color:%2;border:1px solid %2;border-radius:7px;padding:7px;selection-background-color:%3;selection-color:%4;} QLineEdit:focus {border:2px solid %3;}").arg(base,ink,accent,selected));
        }
        for (auto* field : dialog_.findChildren<QComboBox*>()) {
            field->setPalette(colors); field->view()->setPalette(colors);
            field->setProperty("deClockFieldBase",colors.color(QPalette::Base)); field->setProperty("deClockFieldInk",colors.color(QPalette::Text));
            field->setStyleSheet(QString("QComboBox {background:%1;color:%2;border:1px solid %2;border-radius:7px;padding:7px 24px 7px 7px;} QComboBox:focus {border:2px solid %3;} QComboBox::drop-down {width:22px;border:none;} QComboBox QAbstractItemView {background:%1;color:%2;selection-background-color:%3;selection-color:%4;}").arg(base,ink,accent,selected));
        }
        if(auto* scroll=dialog_.findChild<QScrollArea*>("deClockSettingsScroll")) {
            auto surface=colors; surface.setColor(QPalette::Base,colors.color(QPalette::Window));
            for(auto* item:{static_cast<QWidget*>(scroll),scroll->viewport(),scroll->widget()}) {
                item->setPalette(surface); item->setBackgroundRole(QPalette::Window); item->setAutoFillBackground(true);
            }
            scroll->setStyleSheet(QString("QScrollArea {background:%1;border:none;}").arg(canvas));
            scroll->viewport()->setStyleSheet(QString("QWidget#deClockSettingsViewport {background:%1;}").arg(canvas));
            scroll->widget()->setStyleSheet(QString("QWidget#deClockSettingsContent {background:%1;}").arg(canvas));
        }
    }
public:
    explicit OffsetAppearance(QDialog& dialog):QObject(&dialog),dialog_(dialog){dialog.installEventFilter(this);paint();}
protected:
    bool eventFilter(QObject*,QEvent* event)override{if(event->type()==QEvent::PaletteChange)paint();return false;}
};
} // namespace

DeClockSettingsDialog::DeClockSettingsDialog(Getter getter, Apply apply, const std::string& appearance, QWidget* parent)
    : QDialog(parent), getter_(std::move(getter)), apply_(std::move(apply)) {
    if(!getter_ || !apply_)throw std::invalid_argument("DE settings callbacks required");
    if(appearance!="system" && appearance!="light" && appearance!="dark")throw std::invalid_argument("Invalid appearance");
    const auto initial=getter_(); validate_snapshot(initial);
    const auto current=current_text(initial);
    const auto draft=initial.configuration.manual_offset?offset_text(initial.configuration.manual_offset->value):QString("+00:00");
    setObjectName("deClockSettingsDialog");setWindowTitle("DE time settings · HamClock Pro");setAccessibleName("DE time settings");resize(540,400);
    auto* outer=new QVBoxLayout(this);outer->setContentsMargins(16,12,16,12);
    auto* scroll=new QScrollArea(this);scroll->setObjectName("deClockSettingsScroll");scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);
    scroll->viewport()->setObjectName("deClockSettingsViewport");
    auto* body=new QWidget(scroll);body->setObjectName("deClockSettingsContent");auto* layout=new QVBoxLayout(body);layout->setContentsMargins(4,4,4,4);layout->setSpacing(12);
    current_=label(body,"deClockCurrent");layout->addWidget(current_);named_text(current_,current);
    scope_=label(body,"deClockScope");named_text(scope_,"Fixed offset; no automatic daylight-saving changes. Session only; lost after application restart. Automatic DE time is unresolved in this version.");layout->addWidget(scope_);
    auto* grid=new QGridLayout;auto* mode_label=label(body,"deClockModeLabel");mode_label->setText("Mode");
    mode_=new OffsetCombo(body);mode_->setObjectName("deClockMode");mode_->addItems({"Automatic (unresolved)","Manual fixed offset"});mode_->setMinimumHeight(44);mode_->setFocusPolicy(Qt::StrongFocus);mode_->setAccessibleName("DE time mode");mode_->setItemDelegate(new OffsetDelegate(mode_));mode_label->setBuddy(mode_);
    auto* offset_label=label(body,"deClockOffsetLabel");offset_label->setText("Fixed offset ±HH:MM[:SS]");
    offset_=new OffsetLineEdit(body);offset_->setObjectName("deClockOffsetDraft");offset_->setMaxLength(kDeClockOffsetDraftCharacters);offset_->setMinimumHeight(44);offset_->setFocusPolicy(Qt::StrongFocus);offset_->setAccessibleName("Unapplied DE fixed offset");offset_->setAccessibleDescription("Signed ASCII hours, minutes and optional seconds. Range minus24 hours to plus24 hours. Apply explicitly.");offset_label->setBuddy(offset_);
    grid->addWidget(mode_label,0,0);grid->addWidget(mode_,0,1);grid->addWidget(offset_label,1,0);grid->addWidget(offset_,1,1);layout->addLayout(grid);
    error_=label(body,"deClockActionError");layout->addWidget(error_);error_->hide();layout->addStretch();scroll->setWidget(body);outer->addWidget(scroll,1);
    auto* footer=new QHBoxLayout;
    const auto button=[&](const char* text,const char* name){auto* item=new QPushButton(text,this);item->setObjectName(name);item->setAccessibleName(text);item->setMinimumSize(88,44);item->setFocusPolicy(Qt::StrongFocus);item->setAutoDefault(false);footer->addWidget(item);return item;};
    apply_button_=button("Apply","deClockApply");reload_button_=button("Reload","deClockReload");cancel_button_=button("Cancel","deClockCancel");outer->addLayout(footer);
    setTabOrder(mode_,offset_);setTabOrder(offset_,apply_button_);setTabOrder(apply_button_,reload_button_);setTabOrder(reload_button_,cancel_button_);
    connect(mode_,&QComboBox::currentIndexChanged,this,[this](int index){offset_->setEnabled(index==1);});
    connect(apply_button_,&QPushButton::clicked,this,[this](){applyNow();});connect(reload_button_,&QPushButton::clicked,this,[this](){reloadNow();});connect(cancel_button_,&QPushButton::clicked,this,&QDialog::reject);
    connect(offset_,&QLineEdit::returnPressed,this,[this](){applyNow();});
    apply_dashboard_appearance(*this,appearance);new OffsetAppearance(*this);
    confirmed_=initial;draft_basis_=initial;mode_->setCurrentIndex(static_cast<int>(initial.configuration.mode));offset_->setText(draft);offset_->setEnabled(initial.configuration.mode==DeClockMode::ManualOffset);
}
const DeClockConfigurationSnapshot& DeClockSettingsDialog::lastConfirmed()const noexcept{return confirmed_;}
bool DeClockSettingsDialog::confirmationStale()const noexcept{return stale_;}
bool DeClockSettingsDialog::reloadNow() {
    if(updating_)return false;const QScopedValueRollback<bool> guard(updating_,true);
    DeClockConfigurationSnapshot snapshot{};QString current,draft;bool prepared=false;
    try{snapshot=getter_();validate_snapshot(snapshot);current=current_text(snapshot);draft=snapshot.configuration.manual_offset?offset_text(snapshot.configuration.manual_offset->value):QString("+00:00");prepared=true;}
    catch(...){stale_=true;named_text(current_,"Unconfirmed: retained previous setting; Reload failed.");named_text(error_,"Could not reload DE settings. Your draft is unchanged.");error_->show();}
    if(!prepared)return false;
    confirmed_=snapshot;draft_basis_=snapshot;stale_=false;reconciliation_required_=false;
    try{named_text(current_,current);const QSignalBlocker block(mode_);mode_->setCurrentIndex(static_cast<int>(snapshot.configuration.mode));offset_->setText(draft);offset_->setEnabled(snapshot.configuration.mode==DeClockMode::ManualOffset);error_->clear();error_->setAccessibleName({});error_->hide();}
    catch(...){stale_=true;reconciliation_required_=true;
        try{named_text(current_,"Unconfirmed: the current setting could not be rendered.");named_text(error_,"Reload to reconcile the owner state; no rollback is asserted.");error_->show();}catch(...){}
        throw;}
    return true;
}
bool DeClockSettingsDialog::applyNow() {
    if(updating_)return false;const QScopedValueRollback<bool> guard(updating_,true);
    if(reconciliation_required_){named_text(error_,"Apply outcome is unconfirmed. Reload before applying again.");error_->show();return false;}
    DeClockConfiguration desired{DeClockMode::Automatic,std::nullopt};QString requested_draft,requested_current;
    try{if(mode_->currentIndex()==1){desired.mode=DeClockMode::ManualOffset;desired.manual_offset=parse_offset(offset_->text());}
        else if(mode_->currentIndex()!=0)throw std::invalid_argument("Unknown mode draft");
        validate_de_clock_configuration(desired,draft_basis_.offset_policy);
        auto proposed=draft_basis_;proposed.configuration=desired;requested_current=current_text(proposed);
        requested_draft=desired.manual_offset?offset_text(desired.manual_offset->value):QString("+00:00");}
    catch(...){named_text(error_,"Invalid draft. Use signed HH:MM or HH:MM:SS within ±24:00; no setting was applied.");error_->show();return false;}
    DeClockSettingsApplyOutcome outcome{};
    try{outcome=apply_(desired,draft_basis_);}
    catch(...){named_text(error_,"Could not apply DE settings. This action was not committed.");error_->show();return false;}
    QString current,draft,message;bool valid=false;
    try{validate_outcome(outcome,desired,draft_basis_);message=outcome_message(outcome);current=outcome.status==DeClockSettingsApplyStatus::Rejected?current_text(outcome.confirmed):requested_current;draft=requested_draft;valid=true;}
    catch(...){stale_=true;reconciliation_required_=true;named_text(current_,"Unconfirmed: prior setting is not proof of the current owner state.");named_text(error_,"Apply outcome is unconfirmed. Reload to reconcile; no rollback is asserted.");error_->show();}
    if(!valid)return false;
    confirmed_=outcome.confirmed;stale_=false;
    const bool rejected=outcome.status==DeClockSettingsApplyStatus::Rejected;
    if(!rejected)draft_basis_=outcome.confirmed;
    try{
        named_text(current_,current);
        if(rejected){named_text(error_,"Setting was not applied: "+message+" Reload if the draft is based on older settings.");error_->show();return false;}
        const QSignalBlocker block(mode_);mode_->setCurrentIndex(static_cast<int>(desired.mode));offset_->setText(draft);offset_->setEnabled(desired.mode==DeClockMode::ManualOffset);
        if(outcome.status==DeClockSettingsApplyStatus::CommittedDisplayError){named_text(error_,"Setting WAS applied; clock display unavailable: "+message);error_->show();return false;}
        error_->clear();error_->setAccessibleName({});error_->hide();return true;
    }catch(...){stale_=true;reconciliation_required_=true;
        try{named_text(current_,"Unconfirmed: the committed result could not be rendered.");named_text(error_,"Reload to reconcile the owner state; no rollback is asserted.");error_->show();}catch(...){}
        throw;}
}
void DeClockSettingsDialog::keyPressEvent(QKeyEvent* event){
    if(event->key()==Qt::Key_Return || event->key()==Qt::Key_Enter){applyNow();event->accept();return;}
    QDialog::keyPressEvent(event);
}
} // namespace hamclock
