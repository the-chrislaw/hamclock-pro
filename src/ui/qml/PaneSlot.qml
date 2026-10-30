import QtQuick
import HamClock
import "pager.mjs" as Pager

// One dashboard pane: shows the widget chosen for this slot (appSettings.panes),
// or a placeholder when the slot is empty. Tapping anywhere asks to change it.
// Swiping pages like an iOS carousel: the card follows the finger with the
// neighbouring widget sliding in beside it, and keys and rotation slide the same way.
Item {
    id: slot
    objectName: "paneSlot"
    property int index: 0
    readonly property string saved: appSettings.panes[index] || "none"
    readonly property int rotateSeconds: appSettings.demoMode ? 10 : appSettings.paneRotations[index] || 0
    // What is on screen: the saved choice, or the current stop of an automatic
    // rotation (never written to settings, so a rotation does not wear the SD card).
    property string rotated: ""
    readonly property string choice: rotateSeconds > 0 && rotated !== "" ? rotated : saved
    // Widgets the rotation cycles through (empty = every widget).
    readonly property var rotationSet: appSettings.demoMode ? [] : appSettings.paneRotationSets[index] || []
    // Pause rotation while the user is interacting or a menu is open.
    property bool paused: false
    onSavedChanged: rotated = ""
    onRotateSecondsChanged: rotated = ""
    onRotationSetChanged: rotated = ""
    signal chooseRequested(int slot)
    signal setupRequested()
    // Phone: the page dots get their own strip under the card instead of covering its footer.
    readonly property real dotsRoom: Theme.phone && pageList.length > 1 ? 12 * Theme.scale : 0
    signal optionsRequested(int slot)

    // ---- Paging state -------------------------------------------------------
    // Pages sit one slot width plus a gap apart; offset is how far the current
    // page has moved (negative = towards the next page).
    readonly property real span: width + Theme.gap
    property real offset: 0
    // The page sliding in, fixed once a slide is under way ("" = follow the drag).
    property string incoming: ""
    // The step a running slide lands on: {id, dir, persist}, or null for a spring back.
    property var pending: null
    readonly property bool moving: offset !== 0 || slide.running
    readonly property bool canPage: pageList.length > 1
    // Touch devices keep both neighbours loaded so a swipe never shows a blank card.
    // Models are shared by the registry, so a hidden card starts no extra polling.
    readonly property bool preload: Theme.phone || Qt.platform.os === "ios" || Qt.platform.os === "android"
    readonly property var pageList: ids()
    readonly property string prevId: canPage ? Pager.neighbour(pageList, choice, -1) : ""
    readonly property string nextId: canPage ? Pager.neighbour(pageList, choice, 1) : ""
    readonly property string rotationNext: rotateSeconds > 0 ? rotationTarget() : ""
    readonly property string shownIncoming: incoming !== "" ? incoming : offset < 0 ? nextId : offset > 0 ? prevId : ""
    readonly property int incomingSide: pending ? pending.dir : offset < 0 ? 1 : -1
    property real dragBase: 0
    property real dragStartX: 0
    property var samples: []

    // Step to the next (+1) or previous (-1) widget in registry order; "none" sits
    // between the last and first widget so a swipe can also empty the pane.
    function ids() {
        const list = ["none"]
        for (let i = 0; i < loaders.count; ++i) {
            const loader = loaders.itemAt(i)
            if (loader) list.push(loader.wid)
        }
        return list
    }
    // Keys, TV remote: slide one page. A slide still running lands first.
    function step(direction) {
        if (!canPage) return
        settleNow()
        slideTo(Pager.neighbour(pageList, choice, direction), direction, true, 0)
    }
    // Next stop of the automatic rotation (the rotation set, or every widget).
    function rotationTarget() {
        const list = pageList.filter(function (id) {
            return id !== "none" && (rotationSet.length === 0 || rotationSet.indexOf(id) >= 0)
        })
        return list.length === 0 ? "" : Pager.neighbour(list, choice, 1)
    }
    // Page role of a widget id: 0 on screen, -1/+1 sliding in from that side, 2 off stage.
    function roleOf(id) {
        if (id === choice) return 0
        return id === shownIncoming ? incomingSide : 2
    }
    function wanted(id) {
        if (id === choice || id === incoming || id === shownIncoming) return true
        return preload && (id === prevId || id === nextId || id === rotationNext)
    }

    function slideTo(id, dir, persist, speed) {
        if (id === "" || id === choice) return
        pending = { id: id, dir: dir, persist: persist }
        incoming = id
        animateOffset(-dir * span, speed)
    }
    function springBack(speed) {
        pending = null
        animateOffset(0, speed)
    }
    function animateOffset(to, speed) {
        slide.stop()
        const duration = Pager.slideDuration(Math.abs(to - offset), span, Math.abs(speed))
        if (duration <= 0) { offset = to; finishSlide(); return }
        slide.from = offset
        slide.to = to
        slide.duration = duration
        slide.start()
    }
    // Lands the slide: the incoming page becomes the choice in the same frame the
    // offset returns to 0, so nothing jumps. Keeps keyboard focus on the pane.
    function finishSlide() {
        const landing = pending
        const hadFocus = focusedPage() !== null
        if (landing) {
            if (landing.persist) appSettings.setPane(index, landing.id)
            else rotated = landing.id
        }
        pending = null
        incoming = ""
        offset = 0
        if (hadFocus) focusCurrent()
    }
    function settleNow() {
        if (!slide.running) return
        slide.stop()
        finishSlide()
    }
    function focusedPage() {
        if (emptyPage.activeFocus) return emptyPage
        for (let i = 0; i < loaders.count; ++i) {
            const loader = loaders.itemAt(i)
            if (loader && loader.item && loader.item.activeFocus) return loader.item
        }
        return null
    }
    function focusCurrent() {
        for (let i = 0; i < loaders.count; ++i) {
            const loader = loaders.itemAt(i)
            if (loader && loader.wid === choice && loader.item) { loader.item.forceActiveFocus(); return }
        }
        emptyPage.forceActiveFocus()
    }

    // ---- Finger tracking ----------------------------------------------------
    // A new touch catches a running slide where it is instead of jumping.
    function beginDrag(x) {
        if (slide.running) {
            slide.stop()
            if (pending) {
                const caught = offset + pending.dir * span
                finishSlide()
                offset = caught
            }
            pending = null
            incoming = ""
        }
        dragBase = offset
        dragStartX = x
        samples = []
    }
    function dragTo(x) {
        const raw = dragBase + x - dragStartX
        offset = canPage ? Math.max(-span, Math.min(span, raw)) : Pager.rubberBand(raw, span)
        const now = Date.now()
        samples = Pager.recentSamples(samples, now, Pager.VELOCITY_WINDOW_MS).concat([{ t: now, x: offset }])
    }
    function endDrag() {
        const speed = Pager.velocity(samples, Date.now(), Pager.VELOCITY_WINDOW_MS)
        samples = []
        const dir = Pager.releaseStep(offset, speed, span, Pager.FLICK_SPEED * Theme.scale, canPage)
        if (dir !== 0) slideTo(dir > 0 ? nextId : prevId, dir, true, speed)
        else springBack(speed)
    }

    NumberAnimation {
        id: slide
        target: slot
        property: "offset"
        easing.type: Easing.OutCubic
        onFinished: slot.finishSlide()
    }
    // A resize (phone rotation) mid-slide would leave the offset in old pixels.
    onWidthChanged: if (!drag.active) settleNow()
    clip: moving

    // Automatic rotation slides to the next stop like a swipe does.
    Timer {
        interval: slot.rotateSeconds * 1000
        repeat: true
        running: slot.rotateSeconds > 0 && !slot.paused && slot.visible
        onTriggered: if (!slot.moving && !drag.active) slot.slideTo(slot.rotationNext, 1, false, 0)
    }

    Repeater {
        id: loaders
        model: widgetRegistry
        delegate: Loader {
            readonly property string wid: widgetId
            readonly property int role: slot.roleOf(wid)
            x: role === 2 ? 0 : role * slot.span + slot.offset
            width: slot.width
            height: slot.height - slot.dotsRoom
            active: slot.wanted(wid)
            // Preloaded neighbours build over several frames; what is shown builds at once.
            asynchronous: role === 2
            visible: status === Loader.Ready && (role === 0 || (role !== 2 && slot.offset !== 0))
            source: card
            onLoaded: {
                item.widget = widgetModel
                item.chooser = true
                item.adaptive = true
                item.titleActivated.connect(function () { slot.chooseRequested(slot.index) })
                item.setupRequested.connect(slot.setupRequested)
            }
        }
    }

    // Keyboard / TV remote: the focused card bubbles unhandled keys up to the slot.
    // Left/Right step the pane, Menu (or Shift+F10 via Key_Menu) opens its options.
    // Any key counts as activity for screen blanking (does not consume it).
    Keys.onPressed: function (event) { blanking.touch(); event.accepted = false }
    Keys.onLeftPressed: function (event) { slot.step(-1); event.accepted = true }
    Keys.onRightPressed: function (event) { slot.step(1); event.accepted = true }
    // Up/Down move the focus ring to the previous/next focusable item (same order as Tab).
    Keys.onUpPressed: function (event) { slot.moveFocus(false); event.accepted = true }
    Keys.onDownPressed: function (event) { slot.moveFocus(true); event.accepted = true }
    function moveFocus(forward) {
        var next = slot.nextItemInFocusChain(forward)
        if (next && next !== slot) next.forceActiveFocus(forward ? Qt.TabFocusReason : Qt.BacktabFocusReason)
    }
    Keys.onMenuPressed: function (event) { slot.optionsRequested(slot.index); event.accepted = true }

    // Tap anywhere on the pane to change it; swipe sideways to page. Handlers are
    // passive so widgets and the title button keep working; a tap that stops a
    // slide does not also open the chooser.
    TapHandler { onTapped: if (!slot.moving) slot.chooseRequested(slot.index) }
    // Long-press opens the pane options (a long press does not also emit tapped).
    TapHandler { longPressThreshold: 0.6; onLongPressed: slot.optionsRequested(slot.index) }
    // Horizontal only: a vertical drag inside a widget list crosses its own
    // threshold first and keeps scrolling; once this takes over it stays sideways.
    DragHandler {
        id: drag
        target: null
        xAxis.enabled: true
        yAxis.enabled: false
        onActiveChanged: active ? slot.beginDrag(centroid.position.x) : slot.endDrag()
        onCentroidChanged: if (active) slot.dragTo(centroid.position.x)
    }

    PageDots {
        visible: Theme.phone && slot.canPage
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        pages: slot.pageList
        current: slot.choice
        // Moves with the finger: half way through a swipe the dots are half way too.
        progress: slot.span > 0 ? -slot.offset / slot.span : 0
    }

    // Empty slot, or a saved choice whose widget no longer exists. It is the
    // "none" page between the last and first widget, so it slides like one.
    Card {
        id: emptyPage
        readonly property bool showsChoice: slot.choice === "none" || !widgetRegistry.model(slot.choice)
        readonly property int role: showsChoice ? 0 : slot.roleOf("none")
        x: role === 2 ? 0 : role * slot.span + slot.offset
        width: slot.width
        height: slot.height - slot.dotsRoom
        title: "Empty pane"
        chooser: true
        visible: role === 0 || (role !== 2 && slot.offset !== 0)
        onTitleActivated: slot.chooseRequested(slot.index)
        Text {
            width: parent.width
            text: "Tap to choose a widget."
            color: Theme.secondary
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }
    }
}
