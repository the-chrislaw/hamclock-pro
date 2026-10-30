// Paging maths for a swipeable pane (PaneSlot.qml), kept free of QML so the
// regression test can run it in a plain QJSEngine (tests/pager_regression.cpp).
//
// Sign convention: offset is how far the current page has moved, in pixels;
// negative = dragged left, which reveals the next page (step +1).

// A slow drag commits once the page has moved this fraction of its width.
export const COMMIT_FRACTION = 1 / 3
// A release faster than this (logical px per second, times Theme.scale) is a flick.
export const FLICK_SPEED = 350
// Only finger samples this recent count towards the release velocity, so a
// finger that stopped before lifting does not flick.
export const VELOCITY_WINDOW_MS = 90
// Rubber band stiffness at an end that cannot page (UIScrollView uses 0.55).
export const RUBBER = 0.55
// Slide durations: a full page from rest, and the clamps for a flick.
export const SLIDE_MS = 300
export const MIN_SLIDE_MS = 170
export const MAX_SLIDE_MS = 340

// Page `direction` (+1 next, -1 previous) steps from `current` in a circular list.
// An id that is not in the list counts as the first entry.
export function neighbour(list, current, direction) {
    if (!list || list.length === 0) return ""
    const at = Math.max(0, list.indexOf(current))
    return list[(at + direction % list.length + list.length) % list.length]
}

// Direction a gesture or velocity points at: left (negative) = next (+1).
export function directionOf(value) {
    return value < 0 ? 1 : value > 0 ? -1 : 0
}

// Resistance past an end: follows the finger less and less, never beyond one span.
export function rubberBand(offset, span) {
    if (span <= 0 || offset === 0) return 0
    const pulled = Math.abs(offset)
    const eased = (1 - 1 / (pulled * RUBBER / span + 1)) * span
    return offset < 0 ? -eased : eased
}

// Where a released drag goes: +1 next, -1 previous, 0 spring back.
// A flick commits in its own direction unless it points back at the current
// page; otherwise the page must have travelled COMMIT_FRACTION of the span.
export function releaseStep(offset, velocity, span, flickSpeed, canPage) {
    if (!canPage || span <= 0) return 0
    if (Math.abs(velocity) >= flickSpeed) {
        const flick = directionOf(velocity)
        return directionOf(offset) === -flick ? 0 : flick
    }
    return Math.abs(offset) > span * COMMIT_FRACTION ? directionOf(offset) : 0
}

// Keeps only the samples ({t, x}) newer than `windowMs`, as a new array.
export function recentSamples(samples, now, windowMs) {
    return samples.filter(function (s) { return now - s.t <= windowMs })
}

// Release velocity in px/s from the recent samples; 0 when there are too few.
export function velocity(samples, now, windowMs) {
    const recent = recentSamples(samples, now, windowMs)
    if (recent.length < 2) return 0
    const first = recent[0]
    const last = recent[recent.length - 1]
    const dt = last.t - first.t
    return dt > 0 ? (last.x - first.x) * 1000 / dt : 0
}

// How long the slide to rest takes. From rest it scales with the distance left;
// after a flick it starts at the finger's speed (OutCubic starts at 3x average).
export function slideDuration(distance, span, speed) {
    if (span <= 0 || distance <= 0) return 0
    const fromRest = SLIDE_MS * Math.min(1, distance / span)
    const matched = speed > 0 ? 3 * distance * 1000 / speed : fromRest
    return Math.round(Math.max(MIN_SLIDE_MS, Math.min(MAX_SLIDE_MS, fromRest, matched)))
}

// How "current" page dot `index` is (0..1) while the strip sits at `progress`.
export function dotWeight(index, progress) {
    return Math.max(0, 1 - Math.abs(index - progress))
}
