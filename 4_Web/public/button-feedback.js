'use strict';
// Visual feedback only. Command/drive handlers retain ownership of actuation.
(() => {
    const selector = 'button.btn, button.connection-badge, .mobile-nav-btn';
    const pressed = new Map();
    const remove = id => { const button = pressed.get(id); if (button) button.classList.remove('is-pressed'); pressed.delete(id); };
    const clear = () => { for (const id of [...pressed.keys()]) remove(id); document.querySelectorAll('.is-pressed').forEach(b => b.classList.remove('is-pressed')); };
    document.addEventListener('pointerdown', event => {
        const button = event.target.closest(selector);
        if (!button || button.disabled || event.button > 0) return;
        pressed.set(event.pointerId, button); button.classList.add('is-pressed');
    }, true);
    for (const name of ['pointerup', 'pointercancel', 'lostpointercapture']) document.addEventListener(name, event => remove(event.pointerId), true);
    document.addEventListener('keydown', event => { if ([' ', 'Enter'].includes(event.key) && !event.repeat && event.target.matches(selector) && !event.target.disabled) event.target.classList.add('is-pressed'); });
    document.addEventListener('keyup', event => { if ([' ', 'Enter'].includes(event.key)) event.target.classList.remove('is-pressed'); });
    window.addEventListener('blur', clear);
    document.addEventListener('balancebot:release', clear);
    document.addEventListener('visibilitychange', () => { if (document.hidden) clear(); });
})();
