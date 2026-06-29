(function () {
  let images = [];
  let current = 0;

  const box   = document.createElement('div');
  box.className = 'lightbox';
  box.innerHTML = `
    <button class="lightbox-close" aria-label="Schließen">&times;</button>
    <button class="lightbox-prev" aria-label="Vorheriges Bild">&#8592;</button>
    <img class="lightbox-img" src="" alt="">
    <button class="lightbox-next" aria-label="Nächstes Bild">&#8594;</button>`;
  document.body.appendChild(box);

  const img  = box.querySelector('.lightbox-img');
  const prev = box.querySelector('.lightbox-prev');
  const next = box.querySelector('.lightbox-next');

  function show(index) {
    current = (index + images.length) % images.length;
    img.src = images[current].src;
    img.alt = images[current].alt;
    box.classList.add('open');
    document.body.style.overflow = 'hidden';
  }

  function hide() {
    box.classList.remove('open');
    document.body.style.overflow = '';
  }

  prev.addEventListener('click', () => show(current - 1));
  next.addEventListener('click', () => show(current + 1));
  box.querySelector('.lightbox-close').addEventListener('click', hide);
  box.addEventListener('click', (e) => { if (e.target === box) hide(); });

  document.addEventListener('keydown', (e) => {
    if (!box.classList.contains('open')) return;
    if (e.key === 'Escape')     hide();
    if (e.key === 'ArrowLeft')  show(current - 1);
    if (e.key === 'ArrowRight') show(current + 1);
  });

  document.addEventListener('DOMContentLoaded', () => {
    images = Array.from(document.querySelectorAll('[data-lightbox]'));
    images.forEach((el, i) => {
      el.style.cursor = 'pointer';
      el.addEventListener('click', () => show(i));
    });
  });
})();
