# Maintainer: ArchLog Team <archlog@example.com>
pkgname=archlog
pkgver=1.0.0
pkgrel=1
pkgdesc="Lightweight systemd journal log analyzer with CLI and GUI interfaces"
arch=('x86_64' 'i686' 'aarch64')
url="https://github.com/archlog/archlog"
license=('MIT')
depends=('systemd')
makedepends=('gcc' 'make' 'gtk3')
optdepends=('gtk3: for GUI interface')
source=("$pkgname-$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
    cd "$srcdir/$pkgname-$pkgver"
    # Build both CLI and GUI
    make -f Makefile.gui all
}

package() {
    cd "$srcdir/$pkgname-$pkgver"
    make -f Makefile.gui DESTDIR="$pkgdir" install
    
    # Install desktop file
    install -Dm644 archlog-gui.desktop "$pkgdir/usr/share/applications/archlog-gui.desktop"
    
    # Install documentation
    install -Dm644 README.md "$pkgdir/usr/share/doc/$pkgname/README.md"
    install -Dm644 INSTALL_GUIDE.md "$pkgdir/usr/share/doc/$pkgname/INSTALL_GUIDE.md"
}