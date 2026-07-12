function getLanguage() {
  return localStorage.getItem('lang') || 'de';
}

function applyLanguage(lang) {
  const dict = lang === 'en' ? I18N_EN : I18N_DE;
  document.documentElement.lang = lang;

  const page = document.body.dataset.page || '';
  const titleKey = page + '.meta.title';
  if (dict[titleKey]) {
    document.title = dict[titleKey];
  }

  document.querySelectorAll('[data-i18n]').forEach(function (el) {
    const key = el.dataset.i18n;
    if (dict[key] !== undefined) {
      el.innerHTML = dict[key];
    }
  });

  document.querySelectorAll('.lang-switch button').forEach(function (btn) {
    btn.classList.toggle('active', btn.dataset.lang === lang);
  });
}

function setLanguage(lang) {
  localStorage.setItem('lang', lang);
  applyLanguage(lang);
}

document.addEventListener('DOMContentLoaded', function () {
  applyLanguage(getLanguage());
});
