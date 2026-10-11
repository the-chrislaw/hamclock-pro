#pragma once

#include "../app/de_clock_configuration.hpp"
#include <QDialog>
#include <array>
#include <cstddef>
#include <functional>
#include <string>

class QComboBox;
class QKeyEvent;
class QLabel;
class QLineEdit;
class QPushButton;

namespace hamclock {

// Explicit replacement UI policy, not an original numeric/legal-zone limit.
constexpr std::int64_t kDeClockEditorMinimumSeconds = -86400;
constexpr std::int64_t kDeClockEditorMaximumSeconds = 86400;
constexpr std::size_t kDeClockApplyMessageBytes = 241; // ASCII message + NUL.
constexpr int kDeClockOffsetDraftCharacters = 64; // Longer than valid6/9 formats.

enum class DeClockSettingsApplyStatus { Rejected, Applied, Unchanged, CommittedDisplayError };
struct DeClockSettingsApplyOutcome {
    DeClockSettingsApplyStatus status;
    // Authoritative owned snapshot from the SAME stable application session.
    // Required for every outcome. Rejected supplies the current owner state,
    // not a proposed draft. Committed states supply the actual committed state.
    DeClockConfigurationSnapshot confirmed;
    // Bounded ASCII plain explanation; all unused bytes zero. No HTML/data URLs.
    // Rejected and CommittedDisplayError require a nonempty reason. Applied and
    // Unchanged require empty message. The dialog never certifies caller claims.
    std::array<char, kDeClockApplyMessageBytes> message;
};

// Session-only editor, intended entry inside Big Clock Options. Owns only draft
// widgets and last-confirmed value, not configuration/clock/station/file state.
// GUI-thread use only. Both callbacks are copied, nonempty, and may not destroy
// the dialog or violate captured-owner lifetime; no event-loop/worker mutation
// inside the application transaction. No timer or independent OS clock reads.
class DeClockSettingsDialog final : public QDialog {
public:
    using Getter = std::function<DeClockConfigurationSnapshot()>;
    // Expected is the last snapshot on which the draft was based. Application
    // compares it to its SAME stable session before changing desire, rejecting
    // stale location/configuration/policy; it is not an authority/token or a
    // standalone asynchronous completion guard. No session identity invented.
    using Apply = std::function<DeClockSettingsApplyOutcome(
        const DeClockConfiguration& desired, const DeClockConfigurationSnapshot& expected)>;

    // Validate callbacks/appearance before one Getter call. Validate returned
    // configuration, policy exactly [-86400,+86400], latitude[-90,+90], canonical
    // longitude[-180,+180) and configuration_revision>=location_revision>0;
    // prepare complete owned draft/current/status text, then display. The same
    // complete metadata validation applies to Reload and every Apply outcome.
    // Invalid opening snapshot/constructor preparation throws; no Apply call.
    // Constructor/session reopening reflects supplied owner state; no default
    // actual offset or automatic resolution is manufactured.
    DeClockSettingsDialog(Getter, Apply, const std::string& appearance,
                          QWidget* parent = nullptr);

    const DeClockConfigurationSnapshot& lastConfirmed() const noexcept;
    bool confirmationStale() const noexcept;

    // Explicit Reload discards draft only after successful Getter/validation/
    // text preparation. Failure preserves all draft/confirmed state, marks old
    // current labels stale and shows a named error. A successful Reload clears
    // prior action error and refreshes the draft basis. No automatic polling.
    bool reloadNow();

    // Draft edits never invoke callbacks. Manual draft accepts ONLY ASCII signed
    // HH:MM or HH:MM:SS (mandatory +/-; exact two digits per field), minutes and
    // seconds 00..59; ±24:00 valid only with zero minutes/seconds. Reject spaces,
    // Unicode signs/digits, missing sign, intermediate text and out-of-range
    // values WITHOUT clamping, rounding, fixup or replacing the raw draft. The
    // bounded QLineEdit has no validator which silently restores cached text.
    // Its length bound is64, not6/9: truncating an overlong pasted string must
    // leave an invalid-length draft, never silently turn junk into a valid
    // prefix. Parse the widget's current raw text, not last acceptable text.
    // Automatic builds absent manual payload; its disabled offset draft is not
    // applied and may be retained for a subsequent Manual draft. Opening an
    // Automatic snapshot stages +00:00 as an explicitly unapplied draft only.
    // Canonical confirmed offset formatting preserves integer seconds, omitting
    // :00 only when seconds are zero; zero is displayed with + sign.
    //
    // Validate/prepare desired config and dialog text before ONE serialized Apply
    // call (no Getter). Invalid drafts/unknown mode do not call Apply. Callback
    // exceptions promise PRECOMMIT failure; caught as unapplied, preserving old
    // confirmed state and raw draft.
    // Show a bounded generic failure, never arbitrary exception.what() content
    // or secrets from the callback. Outcome reasons are explicitly reviewed
    // user-facing text; ASCII/bounds are structural checks, not a secrecy proof.
    // Caller must not throw a postcommit Qt error through this path and falsely
    // imply rollback. It returns Applied/Unchanged
    // with authoritative confirmation, or CommittedDisplayError when committed
    // configuration exists but its caller display update failed/unavailable.
    // Application conversion/publication must be prepared before checked config
    // commit; Qt postcommit rendering is separate, not a transactional guarantee.
    //
    // Rejected preserves raw draft/basis and reports unapplied reason; validate
    // its owned current snapshot before updating current labels. It never seeds
    // the owner's configuration from an arbitrary snapshot. Successful commit
    // updates confirmed/draft basis; CommittedDisplayError clearly says setting
    // WAS applied but display unavailable, never 'not applied'. Close/Cancel
    // cannot roll back an already applied setting. Result validation/text/Qt
    // failure AFTER callback commit cannot be treated as precommit rejection:
    // malformed/unknown result raises an explicit outcome-unknown error, and
    // postcommit rendering propagates rather than returning false/claiming old
    // owner state. Caller owns reconciliation; this view cannot reverse commit.
    // Malformed outcome or ANY postcallback result-validation/text-preparation
    // failure sets confirmationStale and reconciliation-required. Retained prior
    // current/confirmed labels are explicitly masked as UNCONFIRMED, never
    // presented as fresh or proof of rollback. Preserve raw draft/basis; require
    // successful explicit Reload to reconcile owner state before another Apply.
    // Successful Apply alone cannot clear this unknown-outcome condition.
    // A postcommit Qt render exception likewise sets stale/reconciliation flags
    // before propagating, with best-effort unconfirmed error labeling; process-
    // wide rendering allocation failure cannot promise reliable painted output.
    // Confirmed Applied/Unchanged/CommittedDisplayError must exactly match the
    // requested desire and the draft basis's immutable policy/canonical location.
    // Applied requires a different desire, config revision exactly one greater
    // than expected (checked exhaustion, no wrap), and unchanged location
    // revision. Unchanged requires equal desire and complete equal snapshot.
    // CommittedDisplayError must satisfy the corresponding changed/unchanged
    // relation based on desired-vs-basis equality. A valid-shaped wrong payload,
    // location/policy or revision is outcome-unknown, not confirmed success.
    // These checks enforce this synchronous replace-only API, not universal
    // proof that the callback/session or reported commit is authoritative.
    //
    // Return true only for valid Applied/Unchanged. Rejected and valid
    // CommittedDisplayError return false with DISTINCT truthful status; false
    // therefore is not an owner-rollback guarantee. Persistent action errors
    // clear only on next successful explicit Apply/Reload, not draft edits.
    // Per-dialog RAII in-flight guard spans callbacks/preparation/render; nested
    // Apply/Reload returns false before any callback/UI/draft/stale/error change.
    bool applyNow();

protected:
    void keyPressEvent(QKeyEvent*) override;

private:
    Getter getter_;
    Apply apply_;
    DeClockConfigurationSnapshot confirmed_{};
    // Separate owned last-loaded/successfully-applied draft precondition.
    // A Rejected outcome may update confirmed_ to a newer authoritative current
    // snapshot, but NEVER rebases this value or overwrites raw user draft.
    // Only successful explicit Reload or a validated committed outcome rebases.
    // Clearly mark an outdated draft basis and offer Reload, not implicit save.
    DeClockConfigurationSnapshot draft_basis_{};
    bool stale_ = false;
    bool reconciliation_required_ = false;
    bool updating_ = false;
    QComboBox* mode_ = nullptr;
    QLineEdit* offset_ = nullptr;
    QLabel *current_ = nullptr, *scope_ = nullptr, *error_ = nullptr;
    QPushButton *apply_button_ = nullptr, *reload_button_ = nullptr, *cancel_button_ = nullptr;
};

// Layout/acceptance proposal: compact standard layout/scrolling at800x480 and
// enlarged font, mode/offset plus Apply/Reload/Cancel, every actual hit target
// >=44 logical pixels, names/labels/buddies/tab/focus and keyboard equivalents.
// Enter explicit Apply, Escape Cancel; no implicit focus-loss apply. Current
// mode/offset is separate from unapplied draft. Explain 'Fixed offset; no
// automatic daylight-saving changes', 'Session only; lost after application
// restart', and 'Automatic DE time is unresolved in this version'. No accuracy,
// synchronization or civil availability inferred from configured desire.
// Existing same-theme helper, local Base/Text/background/arrow semantics; actual
// rendered opposite-host themes, checkbox/field/selection/focus/enlarged-font
// probes plus native computer QA required, no global palette/style/new assets.
// This contract does not add persistence, automatic mapper/backend, DST/alarm
// scheduling, radio/GPIO/audio or approval for dashboard/Big Clock source edits.

} // namespace hamclock
