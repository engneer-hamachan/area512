// Open an application reference when it is addressed by a fragment link.
function openReference() {
  const target = document.getElementById(location.hash.slice(1));
  if (target instanceof HTMLDetailsElement) {
    target.open = true;
    target.scrollIntoView();
  }
}
window.addEventListener('hashchange', openReference);
openReference();
