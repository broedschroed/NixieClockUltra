const NAV_PAGES = [
  { href: 'index.html',         label: 'Startseite',    key: 'index' },
  { href: 'features.html',      label: 'Features',      key: 'features' },
  { href: 'aufbau.html',        label: 'Aufbau',        key: 'aufbau' },
  { href: 'geschichte.html',    label: 'Geschichte',    key: 'geschichte' },
  { href: 'dokumentation.html', label: 'Dokumentation', key: 'dokumentation' },
  { href: 'galerie.html',       label: 'Galerie',       key: 'galerie' },
];

const GEAR_SVG = `<svg width="32" height="32" viewBox="0 0 100 100" fill="none" xmlns="http://www.w3.org/2000/svg" aria-hidden="true">
  <path d="M43 5h14l2 11a34 34 0 0 1 8 4.5l10-3.5 9.9 9.9-3.5 10A34 34 0 0 1 88 45l11 2v14l-11 2a34 34 0 0 1-4.5 8l3.5 10-9.9 9.9-10-3.5A34 34 0 0 1 59 91l-2 9H43l-2-9a34 34 0 0 1-8-4.5l-10 3.5-9.9-9.9 3.5-10A34 34 0 0 1 12 57L1 55V41l11-2a34 34 0 0 1 4.5-8l-3.5-10 9.9-9.9 10 3.5A34 34 0 0 1 41 10Z"
    fill="none" stroke="#6b4f12" stroke-width="5"/>
  <circle cx="50" cy="50" r="16" fill="none" stroke="#6b4f12" stroke-width="5"/>
</svg>`;

function injectNav() {
  const page = document.body.dataset.page || '';

  const header = document.getElementById('site-header');
  if (header) {
    header.innerHTML = `
      <div class="container">
        <div class="header-inner">
          ${GEAR_SVG}
          <div class="header-text">
            <div class="site-title">NIXIE CLOCK ULTRA</div>
            <div class="site-subtitle">broed digital media &middot; 2026</div>
          </div>
          ${GEAR_SVG}
        </div>
      </div>`;
  }

  const nav = document.getElementById('site-nav');
  if (nav) {
    const links = NAV_PAGES.map(p =>
      `<a href="${p.href}"${p.key === page ? ' class="active"' : ''}>${p.label}</a>`
    ).join('');
    nav.innerHTML = `
      <div class="container">
        <button class="nav-toggle" aria-label="Navigation" onclick="this.nextElementSibling.classList.toggle('open')">&#9776;</button>
        <div class="nav-inner">${links}</div>
      </div>`;
  }

  const footer = document.getElementById('site-footer');
  if (footer) {
    footer.innerHTML = `
      <div class="container">
        <p class="footer-text">&copy; 2026 broed digital media &middot; Nixie Clock Ultra</p>
      </div>`;
  }
}

document.addEventListener('DOMContentLoaded', injectNav);
