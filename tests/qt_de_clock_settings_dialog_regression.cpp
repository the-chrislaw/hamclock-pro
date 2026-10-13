#include "../src/qt/de_clock_settings_dialog.hpp"
#include <QAbstractItemView>
#include <QAccessible>
#include <QApplication>
#include <QComboBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyleFactory>
#include <QVariant>
#include <climits>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace hamclock;
int checks=0,failures=0;
void check(bool value,const char* label){++checks;if(!value){++failures;std::cerr<<"FAIL: "<<label<<'\n';}}
bool same_config(const DeClockConfiguration& a,const DeClockConfiguration& b){return a.mode==b.mode && a.manual_offset.has_value()==b.manual_offset.has_value() && (!a.manual_offset || a.manual_offset->value==b.manual_offset->value) && de_clock_zone_id(a)==de_clock_zone_id(b);}
bool same(const DeClockConfigurationSnapshot& a,const DeClockConfigurationSnapshot& b){return same_config(a.configuration,b.configuration) && a.offset_policy.minimum.value==b.offset_policy.minimum.value && a.offset_policy.maximum.value==b.offset_policy.maximum.value && a.location.latitude==b.location.latitude && a.location.longitude==b.location.longitude && a.configuration_revision==b.configuration_revision && a.location_revision==b.location_revision;}
DeClockConfigurationSnapshot initial(){return {{DeClockMode::Automatic,std::nullopt},{{-86400},{86400}},{40,-73},1,1};}
DeClockSettingsApplyOutcome outcome(DeClockSettingsApplyStatus status,const DeClockConfigurationSnapshot& snapshot,const char* reason=""){
    DeClockSettingsApplyOutcome result{status,snapshot,{}};std::size_t index=0;for(;reason[index] && index<240;++index)result.message[index]=reason[index];return result;
}
struct Owner {
    DeClockConfigurationSnapshot state=initial(),expected_seen=initial();
    int gets=0,applies=0;bool getter_throw=false,apply_throw=false,reject=false,display_error=false;
    std::function<void(DeClockSettingsApplyOutcome&)> corrupt;
    std::function<void()> reenter;
    DeClockConfigurationSnapshot get(){++gets;if(getter_throw)throw std::runtime_error("secret getter credentials");return state;}
    DeClockSettingsApplyOutcome apply(const DeClockConfiguration& desired,const DeClockConfigurationSnapshot& expected){
        ++applies;expected_seen=expected;if(reenter)reenter();if(apply_throw)throw std::runtime_error("secret apply credentials");
        if(reject || !same(expected,state))return outcome(DeClockSettingsApplyStatus::Rejected,state,"State changed; reload.");
        const bool changed=!same_config(desired,state.configuration);state.configuration=desired;if(changed)++state.configuration_revision;
        auto result=outcome(display_error?DeClockSettingsApplyStatus::CommittedDisplayError:changed?DeClockSettingsApplyStatus::Applied:DeClockSettingsApplyStatus::Unchanged,state,display_error?"Display update unavailable.":"");
        if(corrupt)corrupt(result);return result;
    }
};
template<class Action>bool invalid(Action action){try{action();}catch(const std::invalid_argument&){return true;}return false;}
QLineEdit* draft(DeClockSettingsDialog& dialog){return dialog.findChild<QLineEdit*>("deClockOffsetDraft");}
QComboBox* mode(DeClockSettingsDialog& dialog){return dialog.findChild<QComboBox*>("deClockMode");}
QLineEdit* zone(DeClockSettingsDialog& dialog){return dialog.findChild<QLineEdit*>("deClockZoneDraft");}
QLabel* error(DeClockSettingsDialog& dialog){return dialog.findChild<QLabel*>("deClockActionError");}
QLabel* current(DeClockSettingsDialog& dialog){return dialog.findChild<QLabel*>("deClockCurrent");}
} // namespace
int main(int argc,char** argv){
    QApplication app(argc,argv);using namespace hamclock;const auto original_palette=QApplication::palette();
    Owner owner;auto get=[&](){return owner.get();};auto apply=[&](const auto& desired,const auto& expected){return owner.apply(desired,expected);};
    check(invalid([&](){DeClockSettingsDialog bad({},apply,"light");}) && owner.gets==0,"empty getter rejected before sampling");
    check(invalid([&](){DeClockSettingsDialog bad(get,{},"light");}) && owner.gets==0,"empty Apply rejected before sampling");
    check(invalid([&](){DeClockSettingsDialog bad(get,apply,"bad");}) && owner.gets==0,"unknown appearance rejected before sampling");
    for(unsigned field=0;field<8;++field){owner.state=initial();
        if(field==0)owner.state.offset_policy.minimum.value=-86401;
        if(field==1)owner.state.location.longitude=180;
        if(field==2)owner.state.location.latitude=91;
        if(field==3)owner.state.location.longitude=std::numeric_limits<double>::quiet_NaN();
        if(field==4)owner.state.location_revision=0;
        if(field==5)owner.state.location_revision=2;
        if(field==6)owner.state.configuration.mode=static_cast<DeClockMode>(INT_MAX);
        if(field==7)owner.state.configuration.manual_offset=UtcOffsetSeconds{0};
        check(invalid([&](){DeClockSettingsDialog bad(get,apply,"light");}),"all opening policy/canonical/revision/configuration constraints checked");
    }
    owner.state=initial();owner.gets=0;
    DeClockSettingsDialog dialog(get,apply,"light");
    check(owner.gets==1 && owner.applies==0 && same(dialog.lastConfirmed(),owner.state),"one opening Getter and no Apply/default configuration");
    dialog.show();app.processEvents();mode(dialog)->setCurrentIndex(1);draft(dialog)->setText("+05:30:15");
    check(owner.applies==0 && draft(dialog)->validator()==nullptr,"editing is unapplied raw text with no validator/fixup");
    for(const QString& text:{QString(""),QString("+"),QString("05:30"),QString("+5:30"),QString(" +05:30"),QString("+05:30 "),QString("+00:60"),QString("+00:00:60"),QString("+24:00:01"),QString("-24:01"),QString("+25:00"),QString("+00:00junk"),QString("−05:30"),QString("+０５:３０"),QString("+01:02:030"),QString(100,'9'),QString("+01:02:03")+QString(100,'x')}){
        draft(dialog)->setText(text);const auto raw=draft(dialog)->text();const auto old=owner.state;const int before=owner.applies;
        check(!dialog.applyNow() && owner.applies==before && same(owner.state,old) && draft(dialog)->text()==raw,
              "invalid/raw/intermediate/Unicode/junk/long drafts reject without callback or clamping");
    }
    const struct{const char* text;std::int64_t seconds;} valid_drafts[]={{"+00:00",0},{"-00:00",0},{"+00:00:01",1},{"-00:00:01",-1},{"+05:30:15",19815},{"-03:45:59",-13559},{"+24:00",86400},{"-24:00:00",-86400}};
    for(const auto& fixture:valid_drafts){draft(dialog)->setText(fixture.text);const int before=owner.applies;check(dialog.applyNow() && owner.applies==before+1 && owner.state.configuration.manual_offset->value==fixture.seconds,"integer-second offsets and explicit endpoint policy applied exactly");check(same(dialog.lastConfirmed(),owner.state),"confirmed snapshot reflects actual serialized commit");}
    check(dialog.findChild<QLabel*>("deClockScope")->text().contains("Session only") && dialog.findChild<QLabel*>("deClockScope")->text().contains("unresolved"),"persistence and automatic-resolution limits visible");
    draft(dialog)->setText("+00:00:01");const int enter_before=owner.applies;QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);QApplication::sendEvent(draft(dialog),&enter);
    check(owner.applies==enter_before+1 && owner.state.configuration.manual_offset->value==1,"Enter on raw offset applies exactly once");
    draft(dialog)->setText("junk");mode(dialog)->setCurrentIndex(0);check(dialog.applyNow() && !owner.state.configuration.manual_offset && !draft(dialog)->isEnabled(),"Automatic applies absent offset, not invalid disabled draft");
    mode(dialog)->setCurrentIndex(1);draft(dialog)->setText("+01:00");
    const auto basis=dialog.lastConfirmed();owner.state.configuration={DeClockMode::ManualOffset,UtcOffsetSeconds{7200}};++owner.state.configuration_revision;
    check(!dialog.applyNow() && same(dialog.lastConfirmed(),owner.state) && draft(dialog)->text()=="+01:00","Rejected updates authoritative current but retains raw draft");
    dialog.applyNow();check(same(owner.expected_seen,basis),"Rejected NEVER silently rebases stale draft expected precondition");
    check(dialog.reloadNow() && draft(dialog)->text()=="+02:00", "successful explicit Reload rebases/discards draft");
    owner.getter_throw=true;draft(dialog)->setText("+03:00");const auto prior=dialog.lastConfirmed();
    check(!dialog.reloadNow() && same(dialog.lastConfirmed(),prior) && draft(dialog)->text()=="+03:00" && dialog.confirmationStale() && !error(dialog)->text().contains("secret"),"Getter failure preserves draft/snapshot and masks stale confirmation without exception leak");
    owner.getter_throw=false;const auto valid_owner=owner.state;owner.state.location.longitude=180;
    check(!dialog.reloadNow() && same(dialog.lastConfirmed(),prior) && draft(dialog)->text()=="+03:00" && dialog.confirmationStale(),"malformed Reload snapshot preserves raw draft/basis and masks old confirmation");
    owner.state=valid_owner;dialog.reloadNow();mode(dialog)->setCurrentIndex(1);draft(dialog)->setText("+03:00");owner.apply_throw=true;
    check(!dialog.applyNow() && same(dialog.lastConfirmed(),owner.state) && draft(dialog)->text()=="+03:00" && !error(dialog)->text().contains("secret"),"precommit callback exception generic, unapplied and draft preserved");
    owner.apply_throw=false;owner.display_error=true;check(!dialog.applyNow() && same(dialog.lastConfirmed(),owner.state) && error(dialog)->text().contains("WAS applied"),"committed display error never says rollback/unapplied");owner.display_error=false;check(dialog.applyNow() && error(dialog)->text().isEmpty(),"same-value successful explicit Apply clears action error");

    // Simulate a callback commit then return malformed/wrong-but-valid-shaped
    // outcomes: old confirmation MUST be masked and Apply blocked until Reload.
    for(unsigned corruption=0;corruption<15;++corruption){
        draft(dialog)->setText(corruption%2?"+04:00":"+05:00");const auto before=dialog.lastConfirmed();
        owner.corrupt=[corruption](DeClockSettingsApplyOutcome& result){
            if(corruption==0)result.status=static_cast<DeClockSettingsApplyStatus>(INT_MAX);
            if(corruption==1)result.confirmed.configuration.manual_offset=UtcOffsetSeconds{0};
            if(corruption==2)result.confirmed.offset_policy.maximum.value=86399;
            if(corruption==3)result.confirmed.location.longitude=100;
            if(corruption==4)++result.confirmed.location_revision;
            if(corruption==5)++result.confirmed.configuration_revision;
            if(corruption==6)result.status=DeClockSettingsApplyStatus::Unchanged;
            if(corruption==7)result.message[0]='x';
            if(corruption==8)result.message.fill('x');
            if(corruption==9)result.message[1]='x';
            if(corruption==10)result.confirmed.configuration_revision=0;
            if(corruption==11)result.status=DeClockSettingsApplyStatus::CommittedDisplayError;
            if(corruption==12){result.status=DeClockSettingsApplyStatus::Rejected;result.message[0]='\x1b';}
            if(corruption==13)result.confirmed.configuration.mode=static_cast<DeClockMode>(INT_MIN);
            if(corruption==14)result.confirmed.configuration.manual_offset.reset();
        };
        check(!dialog.applyNow() && dialog.confirmationStale() && same(dialog.lastConfirmed(),before) && current(dialog)->text().contains("Unconfirmed"),"postcallback unknown masks old confirmation, not false rollback or success");
        const int count=owner.applies;check(!dialog.applyNow() && owner.applies==count,"unknown outcome blocks another Apply before reconciliation");
        owner.corrupt={};check(dialog.reloadNow() && !dialog.confirmationStale() && same(dialog.lastConfirmed(),owner.state),"Reload reconciles actual committed owner after malformed outcome");
    }
    // Reentry guard preserves controls/errors/stale/current throughout callback.
    mode(dialog)->setCurrentIndex(1);draft(dialog)->setText("+06:00");int nested=0;
    owner.reenter=[&](){const auto raw=draft(dialog)->text(),message=error(dialog)->text(),current_label=current(dialog)->text();const auto saved=dialog.lastConfirmed();const bool stale=dialog.confirmationStale();check(!dialog.applyNow() && !dialog.reloadNow(),"nested Apply/Reload rejected before callbacks");check(draft(dialog)->text()==raw && error(dialog)->text()==message && current(dialog)->text()==current_label && same(dialog.lastConfirmed(),saved) && dialog.confirmationStale()==stale,"nested rejection preserves all UI/state/errors");++nested;};
    const int before_reentry=owner.applies;check(dialog.applyNow() && owner.applies==before_reentry+1 && nested==1,"outer Apply survives one reentrant callback without recursion");owner.reenter={};
    dialog.findChild<QPushButton*>("deClockCancel")->click();check(!dialog.isVisible(),"Cancel closes without another Apply");
    DeClockSettingsDialog reopened(get,apply,"light");check(same(reopened.lastConfirmed(),owner.state),"session reopening reads surviving owner configuration");
    reopened.show();app.processEvents();for(auto* label:reopened.findChildren<QLabel*>())check(label->textFormat()==Qt::PlainText,"all settings messages plain text");
    for(auto* control:{static_cast<QWidget*>(mode(reopened)),static_cast<QWidget*>(draft(reopened)),static_cast<QWidget*>(reopened.findChild<QPushButton*>("deClockApply")),static_cast<QWidget*>(reopened.findChild<QPushButton*>("deClockReload")),static_cast<QWidget*>(reopened.findChild<QPushButton*>("deClockCancel"))})check(control->height()>=44 && control->focusPolicy()==Qt::StrongFocus && !control->accessibleName().isEmpty(),"named focusable native touch targets");
    mode(reopened)->setFocus();QKeyEvent tab(QEvent::KeyPress,Qt::Key_Tab,Qt::NoModifier);QApplication::sendEvent(mode(reopened),&tab);check(draft(reopened)->hasFocus(),"keyboard Tab reaches enabled offset field");
    auto* ax=QAccessible::queryAccessibleInterface(draft(reopened));check(ax && ax->text(QAccessible::Value)==draft(reopened)->text(),"native text-field accessibility exposes actual draft");
    const int cancel_before=owner.applies;QKeyEvent escape(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);QApplication::sendEvent(&reopened,&escape);check(!reopened.isVisible() && owner.applies==cancel_before,"Escape closes without Apply or rollback");
    for(const auto& style:QStyleFactory::keys()){QApplication::setStyle(style);for(const auto& appearance:{std::string("light"),std::string("dark"),std::string("system")}){
        auto host=original_palette;host.setColor(QPalette::Window,appearance=="dark"?QColor("white"):QColor("#101010"));host.setColor(QPalette::Base,host.color(QPalette::Window));QApplication::setPalette(host);
        DeClockSettingsDialog themed(get,apply,appearance);themed.show();app.processEvents();auto* scroll=themed.findChild<QScrollArea*>("deClockSettingsScroll");
        check(scroll->widget()->grab().toImage().pixelColor(2,2)==themed.palette().color(QPalette::Window),"rendered local canvas correct under opposite host Base");
        check(draft(themed)->palette().color(QPalette::Text)==themed.palette().color(QPalette::Text) && draft(themed)->styleSheet().contains(themed.palette().color(QPalette::Base).name()),"offset text/base/selection palette explicitly local");
        const auto image=mode(themed)->grab().toImage();int ink=0;for(int y=mode(themed)->height()/2-5;y<=mode(themed)->height()/2+5;++y)for(int x=mode(themed)->width()-22;x<mode(themed)->width()-5;++x)if(image.pixelColor(x,y)==themed.palette().color(QPalette::Text))++ink;
        check(ink>=8 && mode(themed)->view()->sizeHintForRow(0)>=44,"rendered mode disclosure ink and popup touch row");
        mode(themed)->setCurrentIndex(1);app.processEvents();const auto enabled_fill=draft(themed)->grab().toImage().pixelColor(draft(themed)->width()-6,draft(themed)->height()/2);
        mode(themed)->setCurrentIndex(0);app.processEvents();const auto disabled_fill=draft(themed)->grab().toImage().pixelColor(draft(themed)->width()-6,draft(themed)->height()/2);
        check(!draft(themed)->isEnabled() && disabled_fill!=enabled_fill,"disabled Automatic offset field renders visibly different from an editable field");
        auto font=themed.font();font.setPointSize(20);themed.setFont(font);themed.resize(800,480);app.processEvents();check(scroll->horizontalScrollBar()->maximum()==0 && draft(themed)->height()>=draft(themed)->sizeHint().height(),"enlarged font fits/scrolls at seven-inch candidate geometry");
    }}
    QApplication::setPalette(original_palette);app.processEvents();
    {   // IANA zone id for Automatic mode (DST-aware DE civil time).
        Owner zone_owner;DeClockSettingsDialog zd([&](){return zone_owner.get();},[&](const auto& d,const auto& e){return zone_owner.apply(d,e);},"light");
        zd.show();app.processEvents();
        check(zone(zd)!=nullptr && zone(zd)->isEnabled() && zone(zd)->minimumHeight()>=44 && zone(zd)->text().isEmpty(),"zone field exists, enabled for Automatic, 44 px tall, empty by default");
        zone(zd)->setText("America/New_York");
        check(zd.applyNow() && de_clock_zone_id(zone_owner.state.configuration)=="America/New_York" && zone_owner.state.configuration.mode==DeClockMode::Automatic && !zone_owner.state.configuration.manual_offset && zone_owner.state.configuration_revision==2,"valid zone applies as Automatic with zone id");
        check(current(zd)->text().contains("America/New_York"),"current label names the applied zone");
        const int before=zone_owner.applies;zone(zd)->setText("Not/AZone");
        check(!zd.applyNow() && zone_owner.applies==before && error(zd)->isVisible() && zone(zd)->text()=="Not/AZone","unknown zone rejected without callback, raw draft kept");
        zone(zd)->setText("bad zone!");check(!zd.applyNow() && zone_owner.applies==before,"malformed zone rejected without callback");
        zone(zd)->setText("");check(zd.applyNow() && de_clock_zone_id(zone_owner.state.configuration).empty(),"empty zone clears the id");
        zone(zd)->setText("Europe/London");check(zd.applyNow(),"zone set again");mode(zd)->setCurrentIndex(1);
        check(!zone(zd)->isEnabled(),"zone field disabled in Manual mode");
        draft(zd)->setText("+01:00");check(zd.applyNow() && de_clock_zone_id(zone_owner.state.configuration).empty(),"Manual applies without a zone id");
        mode(zd)->setCurrentIndex(0);zone_owner.state.configuration=make_de_clock_zone_configuration("Asia/Tokyo");++zone_owner.state.configuration_revision;
        check(zd.reloadNow() && zone(zd)->text()=="Asia/Tokyo","Reload shows the owner's zone id");
    }
    std::cout<<"Qt DE settings: "<<checks<<" checks, "<<failures<<" failures\n";return failures?1:0;
}
