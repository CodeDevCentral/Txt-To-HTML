(() => {
'use strict';

function renderMath() {
    if (!window.katex) {
        console.error('KaTeX failed to load. Check the KaTeX JavaScript request.');
        return;
    }

    document.querySelectorAll('.corp-math').forEach((element) => {
        if (element.dataset.mathRendered === 'true') return; 
        const text = element.textContent.trim();
        const match = text.match(/^\$\$([\s\S]*?)\$\$$/);
        const formula = (match ? match[1] : text).trim(); 
            
        try {
            window.katex.render(formula, element, {
                displayMode: true,
                throwOnError: false,
                strict: 'ignore'
            });
            element.dataset.mathRendered = 'true';
        } 
        catch (error) {
            console.error('KaTeX rendering failed:', error);
        }
    });
}

function connectLiveReload() {
    const scheme = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const socket = new WebSocket(`${scheme}//${window.location.hostname}:9003`);
    socket.addEventListener('message', (event) => {
        if (event.data === 'reload') window.location.reload();
    });
}

// JavaScript security overrides for disable right-click, text selection, saving. 
document.addEventListener('contextmenu', event => event.preventDefault());
document.addEventListener('copy', event => event.preventDefault());
document.addEventListener('dragstart', event => event.preventDefault());

document.addEventListener('DOMContentLoaded', () => {
    renderMath();
    try {
        connectLiveReload();
    } 
    catch (error) {
        console.warn('Live reload is unavailable:', error);
    }
});

})();

