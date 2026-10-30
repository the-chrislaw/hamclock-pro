import QtQuick
import QtQuick.Layouts
import HamClock

// Standard widget card: title + live/stale badge, content, status line.
// Widgets put their content inside; `widget` is their WidgetModel (optional).
Rectangle {
    id: card
    property string title: widget ? widget.title : ""
    property QtObject widget: null
    property bool showStatus: widget !== null
    default property alias content: stage.data
    // True in a small dashboard slot: show key numbers only, drop details.
    readonly property bool compact: width < 420 * Theme.scale || height < 190 * Theme.scale

    // Adaptive fit (dashboard panes; the slot turns it on). The content is laid out
    // in a stage 1/fit the size of the body and drawn `fit` times larger, so a pane
    // bigger than its layout needs grows crisply to fill it instead of leaving space.
    // Two kinds of content:
    //  - Content that stretches itself (a stage child bound to the stage height: lists,
    //    plots) grows with the pane's width, a little past its design width (420 full,
    //    260 compact) up to maxGrow, and fills the stage at any fit.
    //  - Anything else (summaries, metrics, tables) grows past the design width (to
    //    maxGrow) until its natural height fills the body. Height still left over is
    //    shared above and below, so it sits centred.
    //  Either way no text may be cut short at the narrower stage (checked after each
    //  layout; the card backs off when one is).
    // A card can set scaleToFill itself to override the choice.
    property bool adaptive: false
    property bool scaleToFill: !stretches
    // A narrow pane with height to spare (desktop pane rows, the station column's
    // spill pane): widgets stack their full layout instead of the compact one.
    readonly property bool tall: compact && width >= 240 * Theme.scale && height >= width * 1.1
    // A compact pane with real height (the two-row grids at 1280x800 and 2560x1440, the
    // 7-inch panels' panes): room for a list or a stacked layout even when it is not taller than wide.
    readonly property bool roomy: compact && height >= 225 * Theme.scale && width >= 240 * Theme.scale
    // A widget that needs setting up shows why, in full, with a button to Settings.
    readonly property bool needsSetup: adaptive && widget !== null && widget.state === "unconfigured"
    // Nothing to show yet (first answer in flight) or the source failed before any data came:
    // the card shows a centred notice instead of the widget's empty skeleton ("—" in a big pane).
    // A model with `hasData` decides; one without it only counts its first load.
    readonly property bool waiting: adaptive && widget !== null && !needsSetup
        && ["loading", "offline", "error"].indexOf(widget.state) >= 0
        && (widget.hasData === false || (widget.hasData === undefined && widget.state === "loading"))
    signal setupRequested()
    property real designWidth: (compact ? 260 : 420) * Theme.scale
    // Neighbouring panes should read at about the same size: content is grown at most 1.6x
    // (stretching content, by width) or 1.4x (content grown to fill), so a summary never
    // towers over the list beside it.
    property real maxFit: 1.6
    property real fit: 1
    // Leftover body height (pixels) put above content that scales to fill.
    property real lift: 0
    // Highest fit at which no text was cut short (reset when the pane changes size).
    property real ceiling: maxFit
    // Natural content height in stage units: what the children ask for, not what they were given.
    readonly property real naturalHeight: {
        let h = 0
        for (let i = 0; i < stage.children.length; ++i) {
            const c = stage.children[i]
            if (c.visible) h = Math.max(h, c.y + Math.max(c.implicitHeight, 1))
        }
        return h
    }
    readonly property bool stretches: {
        for (let i = 0; i < stage.children.length; ++i) {
            const c = stage.children[i]
            // Bound to the stage height: taller than it asks (spread) or shorter (a list longer than the pane).
            if (c.visible && Math.abs(c.height - c.implicitHeight) > 1) return true
        }
        return false
    }
    // The fit that only reaches the design width (always safe), and the one that fills the height.
    // Stretching content fills any height, so only its width limits it (a long list's natural
    // height is every row, which would otherwise pin it at 1).
    function widthFit() {
        const byWidth = Math.min(maxFit, body.width / designWidth)
        return Math.max(1, scaleToFill ? Math.min(byWidth, body.height / naturalHeight) : byWidth)
    }
    // Content grown to fill keeps a small margin: text drawn a little past its row's budget is not cut by the body.
    readonly property real fillShare: 0.95
    // Growing past the design width stops at 1.4x for every kind of content, so a one-line summary
    // ("Nothing in force") stays a headline, not a poster, and neighbouring panes read alike.
    property real maxGrow: 1.4
    readonly property real minFit: 0.85
    // Stretching content (a list in a big pane) may also grow past its design width, to at most
    // maxGrow and a stage 0.8 of the design width: bigger rows instead of a long list of small ones.
    // Only a pane with height to spare (tall or roomy): in a short strip the plots would be squeezed.
    function stretchFit() {
        if (!tall && !roomy) return widthFit()
        return Math.max(widthFit(), Math.min(maxGrow, ceiling, body.width / (designWidth * 0.8)))
    }
    // Lowest fit the cut check may back off to: 1 for content grown to fill, the design-width fit otherwise.
    function cutFloor() { return scaleToFill ? 1 : widthFit() }
    function heightFit() { return Math.max(1, Math.min(maxGrow, ceiling, body.height * fillShare / naturalHeight)) }
    // `cut` (from checkCuts): text was cut short at this fit, so take the lower target even if it is close.
    function refit(cut) {
        if (!adaptive || fillBody || body.width <= 0 || body.height <= 0 || naturalHeight <= 0) { fit = 1; lift = 0; return }
        const overflows = naturalHeight * fit > body.height + 1
        if (scaleToFill) limitGrowth(overflows)
        // Content that scales to fill may also back off below the design-width fit when text is cut there.
        let target = scaleToFill ? Math.min(ceiling, Math.max(widthFit(), heightFit())) : stretchFit()
        // Content that does not fit even at 1 (a phone strip with a two-line footer) is drawn a
        // little smaller, down to minFit, rather than cut off at the bottom.
        if (scaleToFill && naturalHeight * target > body.height + 1) {
            target = Math.max(minFit, Math.floor(body.height / naturalHeight * 100) / 100)
            ceiling = Math.min(ceiling, target)  // unwrapping at the wider stage must not grow it back
        }
        // Content that no longer fits (text wrapped onto another line at the new width), or whose
        // text was cut, always shrinks; otherwise small changes (text ticking) do not re-layout.
        // Content that scales to fill only shrinks for those reasons or a lower ceiling: shrinking
        // because the fill share was missed by a little would unwrap a line and grow again, forever.
        const mustShrink = overflows || cut === true || (scaleToFill && ceiling < fit)
        const settle = scaleToFill ? target > fit * 1.03 : Math.abs(target - fit) > 0.03 * fit
        if (mustShrink ? target < fit : settle) {
            grownFrom = { fit: fit, height: naturalHeight }
            fit = Math.floor(target * 100) / 100
        }
        lift = scaleToFill ? Math.max(0, Math.floor((body.height - naturalHeight * fit) / 2)) : 0
        if (fit > cutFloor() + 0.01) cutCheck.restart()
    }
    // Growing past the design width narrows the stage, which can make the content taller
    // (lines wrap) or shorter (rows that no longer fit are dropped). Both lower the ceiling:
    // halfway back to the last fit that fitted, so it settles instead of flipping between two.
    function limitGrowth(overflows) {
        if (fit <= widthFit() + 0.01) return
        if (fit > grownFrom.fit + 0.005 && naturalHeight < grownFrom.height - 1)
            ceiling = Math.max(1, Math.min(ceiling, Math.floor(body.height / grownFrom.height * 100) / 100))
        if (!overflows) { fitted = Math.max(fitted, fit); return }
        // It fitted here before, with other content (new data): that reading no longer holds.
        if (fitted >= fit) fitted = 1
        const good = Math.max(widthFit(), fitted)
        ceiling = Math.min(ceiling, fit - good < 0.06 ? good : Math.floor((good + fit) / 2 * 100) / 100)
    }
    // Highest fit past the design width at which the content fitted the body (see limitGrowth).
    property real fitted: 1
    // The fit and natural height before the last change (see refit).
    property var grownFrom: ({ fit: 1, height: 0 })
    // How much narrower the stage may get before visible text in it is cut short: the smallest
    // width/needed ratio of an elided single line or one running out of its container (1 when nothing is cut;
    // a wrapped text over its line limit counts as a fifth too narrow).
    // `inClip`: below an item that clips on purpose (a list showing whole rows), where text past
    // the bottom is meant to be hidden.
    function cutRatio(item, inClip) {
        let ratio = 1
        for (let i = 0; i < item.children.length; ++i) {
            const c = item.children[i]
            if (!c.visible) continue
            // Text below the stage's bottom: a layout squeezed past its minimum (stretching
            // content grown too far in a short pane) overflows instead of shrinking.
            if (!inClip && c.truncated !== undefined && c.text !== "" && c.height > 0) {
                const bottom = c.mapToItem(stage, 0, 0).y + Math.min(c.height, c.contentHeight)
                if (bottom > stage.height + 1) ratio = Math.min(ratio, Math.max(0.5, stage.height / bottom))
            }
            const single = c.truncated !== undefined && c.wrapMode === Text.NoWrap && c.width > 0 && c.objectName !== "elideOk"
            if (single && c.implicitWidth > c.width + 1) ratio = Math.min(ratio, c.width / c.implicitWidth)
            else if (c.truncated === true && c.objectName !== "elideOk") ratio = Math.min(ratio, 0.8)
            // A line sized to its text that runs past the right edge of its container.
            if (single && item.width > 0 && c.x + c.implicitWidth > item.width + 1)
                ratio = Math.min(ratio, Math.max(0.5, (item.width - c.x) / c.implicitWidth))
            // ...or past the right edge of the stage (rows of fixed-width columns).
            if (single) {
                const right = c.mapToItem(stage, 0, 0).x + Math.max(c.width, c.implicitWidth)
                if (right > stage.width + 1) ratio = Math.min(ratio, Math.max(0.5, stage.width / right))
            }
            ratio = Math.min(ratio, cutRatio(c, inClip || c.clip))
        }
        return ratio
    }
    // After the layout settles at a fit above 1, and every few seconds while
    // it stays there (new data can lengthen a line): back off until no text is cut short.
    function checkCuts() {
        if (fit <= cutFloor() + 0.01) return
        const ratio = cutRatio(stage, false)
        if (ratio >= 1) return
        ceiling = Math.max(cutFloor(), Math.min(fit - 0.01, Math.floor(fit * ratio * 0.98 * 100) / 100))
        fitted = Math.min(fitted, ceiling)
        refit(true)
    }
    Timer { id: cutCheck; interval: 30; onTriggered: card.checkCuts() }
    Timer { interval: 3000; repeat: true; running: card.fit > 1.01 && card.visible; onTriggered: card.checkCuts() }
    // Deferred refit owned by the card (Qt.callLater would still run after a pane swap destroyed it).
    Timer { id: refitLater; interval: 0; onTriggered: card.refit() }
    function resetCeiling() { ceiling = maxFit; fitted = 1; grownFrom = { fit: 1, height: 0 }; refitLater.restart() }
    onNaturalHeightChanged: refitLater.restart()
    onWidthChanged: resetCeiling()
    onHeightChanged: resetCeiling()
    onAdaptiveChanged: refitLater.restart()
    onCompactChanged: resetCeiling()
    onDesignWidthChanged: refitLater.restart()
    onScaleToFillChanged: refitLater.restart()
    // When true (set by the pane slot) tapping the title or pressing Enter asks to change the widget.
    property bool chooser: false
    // Content sized by the card (map): its size must not feed back into the card's own.
    property bool fillBody: false
    // False hides the title row (dense screens, map card): the body gets every pixel.
    property bool showTitle: true
    signal titleActivated()

    radius: Theme.radius
    color: Theme.surface
    border.color: activeFocus ? Theme.focus : Theme.divider
    border.width: activeFocus ? 2 : 1
    activeFocusOnTab: true
    clip: true
    Keys.onReturnPressed: if (chooser) titleActivated()
    Keys.onEnterPressed: if (chooser) titleActivated()
    Keys.onSpacePressed: if (chooser) titleActivated()
    implicitHeight: column.implicitHeight + 2 * Theme.pad
    Accessible.role: Accessible.Grouping
    Accessible.name: title + (widget ? ", " + Theme.stateLabel(widget.state) + ", " + widget.statusText : "")

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap * 0.6

        RowLayout {
            visible: card.showTitle
            Layout.fillWidth: true
            spacing: Theme.gap * 0.6
            Text {
                id: titleText
                text: card.title + (card.chooser ? " ▾" : "")
                color: Theme.ink
                font.pixelSize: Theme.fontTitle
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                Layout.fillWidth: true
                Layout.minimumHeight: card.chooser && !Theme.dense ? Theme.touch * 0.7 : 0
                verticalAlignment: Text.AlignVCenter
                Accessible.role: card.chooser ? Accessible.Button : Accessible.StaticText
                Accessible.name: card.title + (card.chooser ? ", change widget" : "")
                MouseArea {
                    anchors.fill: parent
                    enabled: card.chooser
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { card.forceActiveFocus(); card.titleActivated() }
                }
            }
            // Title first: when the title plus the full badge does not fit on one line, the badge drops to a dot.
            TextMetrics { id: titleMetrics; text: titleText.text; font: titleText.font }
            StatusBadge {
                id: badge
                visible: card.showStatus
                status: widget ? widget.state : ""
                dotOnly: titleMetrics.advanceWidth + fullWidth + Theme.gap * 0.6 > column.width
            }
        }
        Item {
            id: body
            objectName: "cardBody"  // pane fill check (--check-fill)
            // The footer can take a second line when the status text arrives: refit to the new body.
            onHeightChanged: card.resetCeiling()
            clip: true
            Layout.fillWidth: true
            Layout.fillHeight: true
            implicitHeight: card.fillBody ? 0 : stage.childrenRect.height
            Item {
                id: stage
                y: card.lift
                width: body.width / card.fit
                // Not shortened by the lift: content that stretches to it never gets one.
                height: body.height / card.fit
                scale: card.fit
                transformOrigin: Item.TopLeft
                visible: !card.needsSetup && !card.waiting
            }
            ColumnLayout {
                visible: card.needsSetup || card.waiting
                anchors.centerIn: parent
                width: Math.min(parent.width, 360 * Theme.scale)
                spacing: Theme.gap
                Text {
                    visible: card.waiting
                    text: card.widget && card.widget.state === "loading" ? "Loading…" : "No data yet"
                    color: Theme.ink
                    font.pixelSize: Theme.fontTitle
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
                Text {
                    // While loading the heading says it all; a long error reason keeps to three lines.
                    visible: text.length > 0
                    // Request URLs in an error reason mean nothing on a dashboard: leave them out.
                    text: !card.widget || (card.waiting && card.widget.state === "loading") ? ""
                        : card.widget.statusText.replace(/\s*https?:\/\/\S+/g, "")
                    color: Theme.secondary
                    font.pixelSize: Theme.fontBody
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    maximumLineCount: card.waiting ? 3 : 6
                    elide: Text.ElideRight
                    objectName: "elideOk"
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
                AppButton {
                    visible: card.needsSetup
                    text: "Open Settings"
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: card.setupRequested()
                }
            }
        }
        RowLayout {
            visible: card.showStatus && !card.needsSetup && (statusLine.text.length > 0 || ageLine.text.length > 0)
            Layout.fillWidth: true
            spacing: Theme.gap * 0.6
            // The status reason may be shortened; the data age never is.
            Text {
                id: statusLine
                objectName: "elideOk"  // long error reasons are shortened on purpose
                // The notice in the body already says it while waiting.
                text: widget && !card.waiting ? widget.statusText : ""
                color: Theme.secondary
                font.pixelSize: Theme.fontCaption
                elide: Text.ElideRight
                wrapMode: Text.WordWrap
                maximumLineCount: card.compact ? 1 : 2
                Layout.fillWidth: true
            }
            Text {
                id: ageLine
                text: widget ? widget.ageText : ""
                visible: text.length > 0
                color: Theme.secondary
                font.pixelSize: Theme.fontCaption
                Layout.alignment: Qt.AlignTop
                Layout.minimumWidth: implicitWidth
            }
        }
    }
}
